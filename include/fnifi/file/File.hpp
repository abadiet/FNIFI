#ifndef FNIFI_FILE_FILE_HPP
#define FNIFI_FILE_FILE_HPP

#include "fnifi/file/IFileHelper.hpp"
#include "fnifi/expression/Kind.hpp"
#include "fnifi/file/Kind.hpp"
#include "fnifi/utils/utils.hpp"
#ifdef ENABLE_EXIV2
#include <exiv2/exiv2.hpp>
#endif  /* ENABLE_EXIV2 */
#include <sys/stat.h>
#include <string>
#include <ostream>


namespace fnifi {
namespace file {

class File {
public:
    struct pCompare {
        bool operator()(const File* a, const File* b) const;
    };

    static Kind GetKind(const fileBuf_t& buf);

    File(fileId_t id, IFileHelper* helper);
    bool operator==(const File& other) const;
    fileId_t getId() const;
    std::string getPath() const;
    std::string getLocalPreviewPath() const;
    std::string getLocalCopyPath() const;
    struct stat getStats() const;
    Kind getKind() const;
    template<typename T>
    bool get(T& res, expression::Kind kind, const std::string& key = "") const;
    fileBuf_t read(bool nocache = false) const;
    void setSortingScore(expr_t score);
    expr_t getSortingScore() const;
    void setIsFilteredOut(bool isFilteredOut);
    bool isFilteredOut() const;
    std::string getCollectionName() const;
    void setHelper(IFileHelper* helper);

private:
    static bool StartWith(const fileBuf_t& buf, const char* chars, size_t n,
                          fileBuf_t::iterator::difference_type offset = 0);
#ifdef ENABLE_EXIV2
    static expr_t ParseTimestamp(const std::string& date,
                                 const std::string& offset = "",
                                 const std::string& subsec = "");
    static expr_t ParseTimestampISO8601(const std::string& date);
    static expr_t ParseTimestampMov(const std::string& date);
    static expr_t ParseLatLong(const std::vector<double>& coord,
                               const std::string& ref);
    const Exiv2::Image::UniquePtr openExiv2Image() const;
    template<typename T>
    static bool GetMetadata(const Exiv2::Image::UniquePtr& image,
                            expression::Kind kind, const std::string& key,
                            T& res);
    template<typename T, typename Iterator>
    static bool ProcessExiv2FindRes(Iterator it, T& res, size_t i = 0);
    static expr_t ParseAltitude(double altitude, char ref);
#endif  /* ENABLE_EXIV2 */

    const fileId_t _id;
    expr_t _sortScore;
    bool _filteredOut;
    IFileHelper* _helper;
};

}  /* namespace file */
}  /* namespace fnifi */

namespace std {

template<>
struct hash<fnifi::file::File> {
    size_t operator()(const fnifi::file::File& obj) const {
        return std::hash<fnifi::fileId_t>()(obj.getId());
    }
};

template<>
struct hash<std::pair<const fnifi::file::File*, fnifi::fileId_t>> {
    size_t operator()(const std::pair<const fnifi::file::File*,
                      fnifi::fileId_t>& obj) const
    {
        return std::hash<fnifi::fileId_t>()(obj.second);
    }
};

}  /* namespace std */


/* IMPLEMENTATIONS */

template<typename T>
bool fnifi::file::File::get(T& result, expression::Kind kind,
                            const std::string& key) const
{
    DLOG("File", this, "Receive request for metadata of kind " << kind
         << " and key \"" << key << "\"")

    switch (kind) {
        case expression::KIND:
        {
            result = static_cast<expr_t>(getKind());
            return true;
        }
        case expression::SIZE:
        {
            const auto path = _helper->getLocalCopyFilePath(_id);
            result = static_cast<expr_t>(std::filesystem::file_size(path));
            return true;
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
                    std::string date;
                    if (GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.Photo.DateTimeOriginal", date))
                    {
                        std::string offset, subsec;
                        GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.Photo.OffsetTimeOriginal", offset);
                        GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.Photo.SubSecTimeOriginal", subsec);
                        const auto val = ParseTimestamp(date, offset, subsec);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.xmp.CreateDate", date))
                    {
                        const auto val = ParseTimestampISO8601(date);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.DateUTC", date))
                    {
                        const auto val = ParseTimestampMov(date);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.TrackCreateDate", date))
                    {
                        const auto val = ParseTimestampMov(date);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.MediaCreateDate", date))
                    {
                        const auto val = ParseTimestampMov(date);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.audio.TrackCreateDate", date))
                    {
                        const auto val = ParseTimestampMov(date);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.audio.MediaCreateDate", date))
                    {
                        const auto val = ParseTimestampMov(date);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.Photo.DateTimeDigitized", date))
                    {
                        std::string offset, subsec;
                        GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.Photo.OffsetTimeDigitized", offset);
                        GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.Photo.SubSecTimeDigitized", subsec);
                        const auto val = ParseTimestamp(date, offset, subsec);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.photoshop.DateCreated", date))
                    {
                        const auto val = ParseTimestampISO8601(date);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
            }
#endif  /* ENABLE_EXIV2 */
            const auto time = _helper->getStats(_id).st_ctimespec;
            result = static_cast<expr_t>(S_TO_NS(time.tv_sec) + time.tv_nsec);
            return true;
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
                    std::string date;
                    if (GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.Photo.DateTime", date))
                    {
                        std::string offset, subsec;
                        GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.Photo.OffsetTime", offset);
                        GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.Photo.SubSecTime", subsec);
                        const auto val = ParseTimestamp(date, offset, subsec);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.xmp.ModifyDate", date))
                    {
                        const auto val = ParseTimestampISO8601(date);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.ModificationDate", date))
                    {
                        const auto val = ParseTimestampMov(date);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.TrackModifyDate", date))
                    {
                        const auto val = ParseTimestampMov(date);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.MediaModifyDate", date))
                    {
                        const auto val = ParseTimestampMov(date);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.audio.TrackModifyDate", date))
                    {
                        const auto val = ParseTimestampMov(date);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
                {
                    std::string date;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.audio.MediaModifyDate", date))
                    {
                        const auto val = ParseTimestampMov(date);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
            }
#endif  /* ENABLE_EXIV2 */
            const auto time = _helper->getStats(_id).st_mtimespec;
            result = static_cast<expr_t>(S_TO_NS(time.tv_sec) + time.tv_nsec);
            return true;
        }
        case expression::WIDTH:
        {
#ifdef ENABLE_EXIV2
            const auto image = openExiv2Image();
            if (image != nullptr) {
                {
                    expr_t val;
                    if (GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.Photo.PixelXDimension", val))
                    {
                        result = val;
                        return true;
                    }
                }
                {
                    std::string valStr;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.SourceImageWidth", valStr))
                    {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                result = static_cast<expr_t>(val);
                                return true;
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    std::string valStr;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.Width", valStr))
                    {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                result = static_cast<expr_t>(val);
                                return true;
                            }
                        } catch (...) {
                        }
                    }
                }
            }
            WLOG("File", this, "Cannot find width")
            return false;
#else  /* ENABLE_EXIV2 */
            WLOG("File", this, "Cannot retrieve width as Exiv2 is disabled")
            return false;
#endif  /* ENABLE_EXIV2 */
        }
        case expression::HEIGHT:
        {
#ifdef ENABLE_EXIV2
            const auto image = openExiv2Image();
            if (image != nullptr) {
                {
                    expr_t val;
                    if (GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.Photo.PixelYDimension", val))
                    {
                        return val;
                    }
                }
                {
                    std::string valStr;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.SourceImageHeight", valStr))
                    {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                result = static_cast<expr_t>(val);
                                return true;
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    std::string valStr;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.Height", valStr))
                    {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                result = static_cast<expr_t>(val);
                                return true;
                            }
                        } catch (...) {
                        }
                    }
                }
            }
            WLOG("File", this, "Cannot find height")
            return false;
#else  /* ENABLE_EXIV2 */
            WLOG("File", this, "Cannot retrieve height as Exiv2 is disabled")
            return false;
#endif  /* ENABLE_EXIV2 */
        }
        case expression::DURATION:
        {
#ifdef ENABLE_EXIV2
            const auto image = openExiv2Image();
            if (image != nullptr) {
                {
                    std::string valStr;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.Duration", valStr))
                    {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                result = static_cast<expr_t>(val * 1000000L);
                                return true;
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    std::string valStr;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.MediaDuration", valStr))
                    {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                result = static_cast<expr_t>(val * 1000000000L);
                                return true;
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    std::string valStr;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.TrackDuration", valStr))
                    {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                result = static_cast<expr_t>(val * 1000000000L);
                                return true;
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    std::string valStr;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.audio.MediaDuration", valStr))
                    {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                result = static_cast<expr_t>(val * 1000000000L);
                                return true;
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    std::string valStr;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.audio.TrackDuration", valStr))
                    {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                result = static_cast<expr_t>(val * 1000000000L);
                                return true;
                            }
                        } catch (...) {
                        }
                    }
                }
                {
                    std::string nStr;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.FrameCount", nStr))
                    {
                        std::string usStr;
                        if (GetMetadata(image, expression::Kind::XMP,
                                        "Xmp.video.MicroSecPerFrame", usStr))
                        {
                            try {
                                const auto n = std::stol(nStr);
                                const auto us = std::stol(usStr);
                                if (n != 0 && us != 0) {
                                    result = static_cast<expr_t>(n * us * 1000L);
                                    return true;
                                }
                            } catch (...) {
                            }
                        }
                    }
                }
            }
            WLOG("File", this, "Cannot find duration: assuming null")
            result = 0;
            return true;
#else  /* ENABLE_EXIV2 */
            WLOG("File", this, "Cannot retrieve duration as Exiv2 is disabled")
            return false;
#endif  /* ENABLE_EXIV2 */
        }
        case expression::LATITUDE:
        {
#ifdef ENABLE_EXIV2
            /*
             * TODO: to consider
             * Xmp.exif.GPSLatitude
             */
            const auto image = openExiv2Image();
            if (image != nullptr) {
                {
                    std::vector<double> coord;
                    if (GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.GPSInfo.GPSLatitude", coord))
                    {
                        std::string ref;
                        GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.GPSInfo.GPSLatitudeRef", ref);
                        const auto val = ParseLatLong(coord, ref);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
            }
            WLOG("File", this, "Cannot find latitude")
            return false;
#else  /* ENABLE_EXIV2 */
            WLOG("File", this, "Cannot retrieve latitude as Exiv2 is disabled")
            return false;
#endif  /* ENABLE_EXIV2 */
        }
        case expression::LONGITUDE:
        {
#ifdef ENABLE_EXIV2
            /*
             * TODO: to consider
             * Xmp.exif.GPSLongitude
             */
            const auto image = openExiv2Image();
            if (image != nullptr) {
                {
                    std::vector<double> coord;
                    if (GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.GPSInfo.GPSLongitude", coord))
                    {
                        std::string ref;
                        GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.GPSInfo.GPSLongitudeRef", ref);
                        const auto val = ParseLatLong(coord, ref);
                        if (val != EMPTY_EXPR_T) {
                            result = val;
                            return true;
                        }
                    }
                }
            }
            WLOG("File", this, "Cannot find longitude")
            return false;
#else  /* ENABLE_EXIV2 */
            WLOG("File", this, "Cannot retrieve longitude as Exiv2 is "
                 "disabled")
            return false;
#endif  /* ENABLE_EXIV2 */
        }
        case expression::ALTITUDE:
        {
#ifdef ENABLE_EXIV2
            const auto image = openExiv2Image();
            if (image != nullptr) {
                {
                    double altitude;
                    if (GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.GPSInfo.GPSAltitude", altitude))
                    {
                        char ref;
                        GetMetadata(image, expression::Kind::EXIF,
                                    "Exif.GPSInfo.GPSAltitudeRef", ref);
                        result = ParseAltitude(altitude, ref);
                        return true;
                    }
                }
            }
            WLOG("File", this, "Cannot find altitude")
            return false;
#else  /* ENABLE_EXIV2 */
            WLOG("File", this, "Cannot retrieve altitude as Exiv2 is disabled")
            return false;
#endif  /* ENABLE_EXIV2 */
        }

        case expression::EXIF:
        case expression::XMP:
        case expression::IPTC:
        {
#ifdef ENABLE_EXIV2
            auto image = openExiv2Image();
            if (image == nullptr) {
                return false;
            }
            T val;
            if (GetMetadata(image, kind, key, val)) {
                result = val;
                return val;
            }
            WLOG("File", this, "Failed to retrieve metadata for kind "
                 << kind << " and key \"" << key << "\": val=" << val)
            return false;
#else  /* ENABLE_EXIV2 */
            WLOG("File", this, "Cannot retrieve metadata for kind " << kind
                 << " and key \"" << key << "\" as Exiv2 is disabled")
            return false;
            UNUSED(key)
#endif  /* ENABLE_EXIV2 */
        }
        case expression::UNKNOWN:
            throw std::runtime_error("Bad metadata's kind");
    }
}

#ifdef ENABLE_EXIV2
template<typename T>
bool fnifi::file::File::GetMetadata(const Exiv2::Image::UniquePtr& image,
                                    expression::Kind kind,
                                    const std::string& key, T& res)
{
    if (kind == expression::EXIF) {
        try {
            const auto& meta = image->exifData();
            const auto pos = meta.findKey(Exiv2::ExifKey(key));
            if (pos != meta.end()) {
                return ProcessExiv2FindRes(pos, res);
            }
        } catch (Exiv2::Error&) {
            return false;
        }
    } else if (kind == expression::XMP) {
        try {
            const auto& meta = image->xmpData();
            const auto pos = meta.findKey(Exiv2::XmpKey(key));
            if (pos != meta.end()) {
                return ProcessExiv2FindRes(pos, res);
            }
        } catch (Exiv2::Error&) {
            return false;
        }
    } else if (kind == expression::IPTC) {
        try {
            const auto& meta = image->iptcData();
            const auto pos = meta.findKey(Exiv2::IptcKey(key));
            if (pos != meta.end()) {
                return ProcessExiv2FindRes(pos, res);
            }
        } catch (Exiv2::Error&) {
            return false;
        }
    } else {
        throw std::runtime_error("File::GetMetadata should be called with kind"
                                 " EXIF, XMP and IPTC only");
    }

    /* key not found */
    return false;
}

template<typename T, typename Iterator>
bool fnifi::file::File::ProcessExiv2FindRes(Iterator it, T& res, size_t i) {
    if constexpr (std::is_integral_v<T>) {
        res = static_cast<T>(it->toInt64(i));
        return true;
    } else if constexpr (std::is_floating_point_v<T>) {
        const auto rational = it->toRational(i);
        res = static_cast<T>(rational.first) / static_cast<T>(rational.second);
        return true;
    } else if constexpr (
        std::is_same_v<T, std::vector<typename T::value_type>>)
    {
        res.resize(it->count());
        for (size_t j = 0; j < it->count(); ++j) {
            if (!ProcessExiv2FindRes(it, res[j], j)) {
                return false;
            }
        }
        return true;
    } else if constexpr (std::is_same_v<T, std::string>) {
        res = it->toString(i);
        return true;
    }

    return false;
}
#endif  /* ENABLE_EXIV2 */

#endif  /* FNIFI_FILE_FILE_HPP */
