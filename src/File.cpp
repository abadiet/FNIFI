#include "fnifi/file/File.hpp"


using namespace fnifi;
using namespace fnifi::file;

bool File::pCompare::operator()(const File* a, const File* b) const {
    return a->_sortScore < b->_sortScore;
}

File::File(fileId_t id, IFileHelper* helper)
: _id(id), _filteredOut(false), _helper(helper)
{
    DLOG("File", this, "Instanciation for id " << id << " and IFileHelper "
         << helper)
}

bool File::operator==(const File& other) const {
    return _id == other.getId();
}

fileId_t File::getId() const {
    return _id;
}

std::string File::getPath() const {
    return _helper->getFilePath(_id);
}

std::string File::getLocalPreviewPath() const {
    return _helper->getLocalPreviewFilePath(_id);
}

std::string File::getLocalCopyPath() const {
    return _helper->getLocalCopyFilePath(_id);
}

struct stat File::getStats() const {
    return _helper->getStats(_id);
}

Kind File::getKind() const {
    const auto content = _helper->read(_id, false);
    if (content.size() > 0) {
        return GetKind(content);
    }
    WLOG("File", this, "Unable to read the file: returning Kind::UNKNOWN")
    return Kind::UNKNOWN;
}

expr_t File::getValue(expression::Kind kind, const std::string& key) const {
    DLOG("File", this, "Receive request for metadata of kind " << kind
         << " and key \"" << key << "\"")

    switch (kind) {
        case expression::KIND:
            return static_cast<expr_t>(getKind());
        case expression::SIZE:
        {
            const auto path = _helper->getLocalCopyFilePath(_id);
            return static_cast<expr_t>(std::filesystem::file_size(path));
        }
        case expression::CTIME:
        {
#ifdef ENABLE_EXIV2
            /*
             * TODO: to consider
             * Xmp.video.DateTimeOriginal
             * Xmp.video.CreateDate
             * Xmp.video.CreationDate
             * Xmp.audio.DateTimeOriginal
             * Xmp.audio.CreateDate
             * Xmp.audio.CreationDate
             * Xmp.exif.DateTimeOriginal
             * Xmp.exif.DateTimeDigitized
             * Iptc.Application2.DateCreated
             * Iptc.Application2.TimeCreated
             */
            const auto image = openExiv2Image();
            if (image != nullptr) {
                {
                    const auto date = GetMetadata(image,
                        expression::Kind::EXIF, "Exif.Photo.DateTimeOriginal");
                    if (date != "") {
                        const auto offset = GetMetadata(image,
                            expression::Kind::EXIF,
                            "Exif.Photo.OffsetTimeOriginal");
                        const auto subsec = GetMetadata(image,
                            expression::Kind::EXIF,
                            "Exif.Photo.SubSecTimeOriginal");
                        const auto res = utils::ParseTimestamp(date,
                            offset, subsec);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image, expression::Kind::XMP,
                                                  "Xmp.xmp.CreateDate");
                    if (date != "") {
                        const auto res = utils::ParseTimestampISO8601(
                            date);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image, expression::Kind::XMP,
                                                  "Xmp.video.DateUTC");
                    if (date != "") {
                        const auto res = utils::ParseTimestampMov(date);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image, expression::Kind::XMP,
                                                  "Xmp.video.TrackCreateDate");
                    if (date != "") {
                        const auto res = utils::ParseTimestampMov(date);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image, expression::Kind::XMP,
                                                  "Xmp.video.MediaCreateDate");
                    if (date != "") {
                        const auto res = utils::ParseTimestampMov(date);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image, expression::Kind::XMP,
                                                  "Xmp.audio.TrackCreateDate");
                    if (date != "") {
                        const auto res = utils::ParseTimestampMov(date);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image, expression::Kind::XMP,
                                                  "Xmp.audio.MediaCreateDate");
                    if (date != "") {
                        const auto res = utils::ParseTimestampMov(date);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image,
                        expression::Kind::EXIF,
                        "Exif.Photo.DateTimeDigitized");
                    if (date != "") {
                        const auto offset = GetMetadata(image,
                            expression::Kind::EXIF,
                            "Exif.Photo.OffsetTimeDigitized");
                        const auto subsec = GetMetadata(image,
                            expression::Kind::EXIF,
                            "Exif.Photo.SubSecTimeDigitized");
                        const auto res = utils::ParseTimestamp(date,
                            offset, subsec);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image, expression::Kind::XMP,
                                                  "Xmp.photoshop.DateCreated");
                    if (date != "") {
                        const auto res = utils::ParseTimestampISO8601(
                            date);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
            }
#endif  /* ENABLE_EXIV2 */
            const auto time = _helper->getStats(_id).st_ctimespec;
            return static_cast<expr_t>(S_TO_NS(time.tv_sec) + time.tv_nsec);
        }
        case expression::MTIME:
        {
#ifdef ENABLE_EXIV2
            /*
             * TODO: to consider
             * Xmp.tiff.DateTime
             */
            const auto image = openExiv2Image();
            if (image != nullptr) {
                {
                    const auto date = GetMetadata(image,
                        expression::Kind::EXIF, "Exif.Photo.DateTime");
                    if (date != "") {
                        const auto offset = GetMetadata(image,
                            expression::Kind::EXIF, "Exif.Photo.OffsetTime");
                        const auto subsec = GetMetadata(image,
                            expression::Kind::EXIF, "Exif.Photo.SubSecTime");
                        const auto res = utils::ParseTimestamp(date, offset,
                                                               subsec);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image, expression::Kind::XMP,
                                                  "Xmp.xmp.ModifyDate");
                    if (date != "") {
                        const auto res = utils::ParseTimestampISO8601(date);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image, expression::Kind::XMP,
                        "Xmp.video.ModificationDate");
                    if (date != "") {
                        const auto res = utils::ParseTimestampMov(date);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image, expression::Kind::XMP,
                                                  "Xmp.video.TrackModifyDate");
                    if (date != "") {
                        const auto res = utils::ParseTimestampMov(date);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image, expression::Kind::XMP,
                                                  "Xmp.video.MediaModifyDate");
                    if (date != "") {
                        const auto res = utils::ParseTimestampMov(date);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image, expression::Kind::XMP,
                                                  "Xmp.audio.TrackModifyDate");
                    if (date != "") {
                        const auto res = utils::ParseTimestampMov(date);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
                {
                    const auto date = GetMetadata(image, expression::Kind::XMP,
                                                  "Xmp.audio.MediaModifyDate");
                    if (date != "") {
                        const auto res = utils::ParseTimestampMov(date);
                        if (res.count() != 0) {
                            return static_cast<expr_t>(res.count());
                        }
                    }
                }
            }
#endif  /* ENABLE_EXIV2 */
            const auto time = _helper->getStats(_id).st_mtimespec;
            return static_cast<expr_t>(S_TO_NS(time.tv_sec) + time.tv_nsec);
        }
        case expression::WIDTH:
        {
#ifdef ENABLE_EXIV2
            const auto image = openExiv2Image();
            if (image != nullptr) {
                {
                    const auto valStr = GetMetadata(image,
                        expression::Kind::EXIF, "Exif.Photo.PixelXDimension");
                    if (!valStr.empty()) {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                return static_cast<expr_t>(val);
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    const auto valStr = GetMetadata(image,
                        expression::Kind::XMP, "Xmp.video.SourceImageWidth");
                    if (!valStr.empty()) {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                return static_cast<expr_t>(val);
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    const auto valStr = GetMetadata(image,
                        expression::Kind::XMP, "Xmp.video.Width");
                    if (!valStr.empty()) {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                return static_cast<expr_t>(val);
                            }
                        } catch (...) {
                        }
                    }
                }
            }
            WLOG("File", this, "Cannot find width: returning EMPTY_EXPR_T")
            return EMPTY_EXPR_T;
#else  /* ENABLE_EXIV2 */
            WLOG("File", this, "Cannot retrieve width as Exiv2 is disabled: "
                 "returning EMPTY_EXPR_T")
            return EMPTY_EXPR_T;
#endif  /* ENABLE_EXIV2 */
        }
        case expression::HEIGHT:
        {
#ifdef ENABLE_EXIV2
            const auto image = openExiv2Image();
            if (image != nullptr) {
                {
                    const auto valStr = GetMetadata(image,
                        expression::Kind::EXIF, "Exif.Photo.PixelYDimension");
                    if (!valStr.empty()) {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                return static_cast<expr_t>(val);
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    const auto valStr = GetMetadata(image,
                        expression::Kind::XMP, "Xmp.video.SourceImageHeight");
                        if (!valStr.empty()) {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                return static_cast<expr_t>(val);
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    const auto valStr = GetMetadata(image,
                        expression::Kind::XMP, "Xmp.video.Height");
                    if (!valStr.empty()) {
                    try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                return static_cast<expr_t>(val);
                            }
                        } catch (...) {
                        }
                    }
                }
            }
            WLOG("File", this, "Cannot find height: returning EMPTY_EXPR_T")
            return EMPTY_EXPR_T;
#else  /* ENABLE_EXIV2 */
            WLOG("File", this, "Cannot retrieve height as Exiv2 is disabled: "
                 "returning EMPTY_EXPR_T")
            return EMPTY_EXPR_T;
#endif  /* ENABLE_EXIV2 */
        }
        case expression::DURATION:
        {
#ifdef ENABLE_EXIV2
            const auto image = openExiv2Image();
            if (image != nullptr) {
                {
                    const auto valStr = GetMetadata(image,
                        expression::Kind::XMP, "Xmp.video.Duration");
                    if (!valStr.empty()) {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                return static_cast<expr_t>(val * 1000000L);
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    const auto valStr = GetMetadata(image,
                        expression::Kind::XMP, "Xmp.video.MediaDuration");
                    if (!valStr.empty()) {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                return static_cast<expr_t>(val * 1000000000L);
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    const auto valStr = GetMetadata(image,
                        expression::Kind::XMP, "Xmp.video.TrackDuration");
                    if (!valStr.empty()) {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                return static_cast<expr_t>(val * 1000000000L);
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    const auto valStr = GetMetadata(image,
                        expression::Kind::XMP, "Xmp.audio.MediaDuration");
                    if (!valStr.empty()) {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                return static_cast<expr_t>(val * 1000000000L);
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    const auto valStr = GetMetadata(image,
                        expression::Kind::XMP, "Xmp.audio.TrackDuration");
                    if (!valStr.empty()) {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                return static_cast<expr_t>(val * 1000000000L);
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    const auto nStr = GetMetadata(image,
                        expression::Kind::XMP, "Xmp.video.FrameCount");
                    if (!nStr.empty()) {
                        const auto usStr = GetMetadata(image,
                            expression::Kind::XMP,
                            "Xmp.video.MicroSecPerFrame");
                        if (!usStr.empty()) {
                            try {
                                const auto n = std::stol(nStr);
                                const auto us = std::stol(usStr);
                                if (n != 0 && us != 0) {
                                    return static_cast<expr_t>(n * us * 1000L);
                                }
                            } catch (...) {
                            }
                        }
                    }
                }
            }
            WLOG("File", this, "Cannot find duration: returning 0")
            return 0;
#else  /* ENABLE_EXIV2 */
            WLOG("File", this, "Cannot retrieve duration as Exiv2 is disabled:"
                 " returning 0")
            return 0;
#endif  /* ENABLE_EXIV2 */
        }
        case expression::EXIF:
        case expression::XMP:
        case expression::IPTC:
        {
#ifdef ENABLE_EXIV2
            auto image = openExiv2Image();
            if (image == nullptr) {
                return EMPTY_EXPR_T;
            }
            const auto valueStr = GetMetadata(image, kind, key);
            try {
                return std::stol(valueStr);
            } catch(...) {
                WLOG("File", this, "Failed to retrieve metadata for kind "
                     << kind << " and key \"" << key << "\": cannot convert \""
                     << valueStr
                     << "\" to type expr_t: returning EMPTY_EXPR_T")
                return EMPTY_EXPR_T;
            }
#else  /* ENABLE_EXIV2 */
            WLOG("File", this, "Cannot retrieve metadata for kind " << kind
                 << " and key \"" << key
                 << "\" as Exiv2 is disabled: returning EMPTY_EXPR_T")
            return EMPTY_EXPR_T;
#endif  /* ENABLE_EXIV2 */
            }
        case expression::UNKNOWN:
            throw std::runtime_error("Bad metadata's kind");
    }
}

fileBuf_t File::read(bool nocache) const {
    return _helper->read(_id, nocache);
}

void File::setSortingScore(expr_t score) {
    _sortScore = score;
}

expr_t File::getSortingScore() const {
    return _sortScore;
}

void File::setIsFilteredOut(bool isFilteredOut) {
    _filteredOut = isFilteredOut;
}

bool File::isFilteredOut() const {
    return _filteredOut;
}

std::string File::getCollectionName() const {
    return _helper->getName();
}

void File::setHelper(IFileHelper* helper) {
    _helper = helper;
}

Kind File::GetKind(const fileBuf_t& buf) {
    if (StartWith(buf, "BM", 2))
        return Kind::BMP;
    if (StartWith(buf, "GIF87a", 6) || StartWith(buf, "GIF89a", 6))
        return Kind::GIF;
    if (StartWith(buf, "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a", 12))
        return Kind::JPEG2000;
    if (StartWith(buf, "\xff\xd8\xff", 3))
        return Kind::JPEG;
    if (StartWith(buf, "\x89\x50\x4E\x47", 4))
        return Kind::PNG;
    if (StartWith(buf, "RIFF", 4) && StartWith(buf, "WEBP", 4, 8))
        return Kind::WEBP;
    if (StartWith(buf, "P1", 2) || StartWith(buf, "P4", 2))
        return Kind::PBM;
    if (StartWith(buf, "P2", 2) || StartWith(buf, "P5", 2))
        return Kind::PGM;
    if (StartWith(buf, "P3", 2) || StartWith(buf, "P6", 2))
        return Kind::PPM;
    if (StartWith(buf,  "PF", 2) || StartWith(buf, "Pf", 2))
        return Kind::PFM;
    if (StartWith(buf, "\x49\x49\x2A\x00", 4) ||
        StartWith(buf, "\x49\x49\x00\x2A", 4))
        return Kind::TIFF;
    if (StartWith(buf, "\x76\x2F\x31\x01", 4))
        return Kind::EXR;
    if (StartWith(buf, "#?", 2))
        return Kind::HDR;  /* not normalized */
    if (StartWith(buf, "\x1A\x45\xDF\xA3", 4))
        return Kind::MKV;
    if (StartWith(buf, "RIFF", 4) && StartWith(buf, "AVI ", 4, 8))
        return Kind::AVI;
    if (StartWith(buf, "ftypqt  ", 8, 4))
        return Kind::MOV;
    if (StartWith(buf,
            "\x30\x26\xB2\x75\x8E\x66\xCF\x11\xA6\xD9\x00\xAA\x00\x62\xCE\x6C",
            16))
        return Kind::WMV;
    if (StartWith(buf, "RIFF", 4) && StartWith(buf, "YUVN", 4, 8))
        return Kind::YUV;
    if (StartWith(buf, "ftypisom", 8, 4) || StartWith(buf, "ftypMSNV", 8, 4))
        return Kind::MP4;
    if (StartWith(buf, "ftypmp42", 8, 4) || StartWith(buf, "ftypM4V ", 8, 4))
        return Kind::M4V;
    /* TODO: AVIF, PXM, SR, RAS, PIC, MTS */
    return Kind::UNKNOWN;
}

bool File::StartWith(const fileBuf_t& buf, const char* chars, size_t n,
                     fileBuf_t::iterator::difference_type offset)
{
    if (buf.size() < n + static_cast<size_t>(offset)) {
        return false;
    }
    return std::memcmp(buf.data() + offset, chars, n) == 0;
}

#ifdef ENABLE_EXIV2
const Exiv2::Image::UniquePtr File::openExiv2Image() const {
    const auto data = _helper->read(_id, false);
    Exiv2::Image::UniquePtr image;
    try {
        image = Exiv2::ImageFactory::open(
            static_cast<const Exiv2::byte*>(data.data()),
            data.size());
    } catch (Exiv2::Error&) {
        return nullptr;
    }
    if (image == nullptr || image.get() == nullptr) {
        return nullptr;
    }
    try {
        image->readMetadata();
    } catch (Exiv2::Error&) {
        return nullptr;
    }
    return image;
}

std::string File::GetMetadata(const Exiv2::Image::UniquePtr& image,
                              expression::Kind kind,
                              const std::string& key)
{
    try {
        switch (kind) {
            case expression::XMP:
                {
                    const auto& meta = image->xmpData();
                    const auto pos = meta.findKey(
                        Exiv2::XmpKey(key));
                    if (pos != meta.end()) {
                        std::ostringstream valOss;
                        valOss << pos->value();
                        return valOss.str();
                    }
                }
                break;
            case expression::EXIF:
                {
                    const auto& meta = image->exifData();
                    const auto pos = meta.findKey(
                        Exiv2::ExifKey(key));
                    if (pos != meta.end()) {
                        std::ostringstream valOss;
                        valOss << pos->value();
                        return valOss.str();
                    }
                }
                break;
            case expression::IPTC:
                {
                    const auto& meta = image->iptcData();
                    const auto pos = meta.findKey(
                        Exiv2::IptcKey(key));
                    if (pos != meta.end()) {
                        std::ostringstream valOss;
                        valOss << pos->value();
                        return valOss.str();
                    }
                }
                break;
            case expression::KIND:
            case expression::SIZE:
            case expression::CTIME:
            case expression::MTIME:
            case expression::WIDTH:
            case expression::HEIGHT:
            case expression::DURATION:
            case expression::UNKNOWN:
                throw std::runtime_error("File::GetMetadata should be called "
                                         "with kind EXIF, XMP and IPTC only");
        }
    } catch (Exiv2::Error&) {
        return "";
    }
    /* key not found */
    return "";
}
#endif  /* ENABLE_EXIV2 */
