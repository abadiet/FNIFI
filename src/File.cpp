#include "fnifi/file/File.hpp"
#include <vector>


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
expr_t File::ParseTimestamp(const std::string& date, const std::string& offset,
    const std::string& subsec)
{
    std::istringstream iss(date);
    try {
        std::tm tm = {};
        iss >> std::get_time(&tm, "%Y:%m:%d %H:%M:%S");
        if (iss.fail()) {
            return EMPTY_EXPR_T;
        }

        const auto time = std::mktime(&tm) - timezone;
        if (time == -1) {
            return EMPTY_EXPR_T;
        }

        const auto tp = std::chrono::system_clock::from_time_t(time);
        auto nstime = static_cast<expr_t>(std::chrono::duration_cast<
            std::chrono::nanoseconds>(tp.time_since_epoch()).count());

        if (offset != "") {
            const auto sign = (offset[0] == '+') ? 1LL : -1LL;
            const auto hours = std::stol(offset.substr(1, 2));
            const auto minutes = std::stol(offset.substr(4, 2));
            nstime -= sign * (hours * 3600000000000L + minutes * 60000000000L);
        }

        if (subsec != "") {
            nstime += std::stol(subsec);
        }

        return nstime;
    } catch (...) {
        return EMPTY_EXPR_T;
    }
}

expr_t File::ParseTimestampISO8601(const std::string& date) {
    const auto offsetPos = date.find_last_of("+-");
    auto newDate = date.substr(0, offsetPos);
    std::replace(newDate.begin(), newDate.end(), 'T', ' ');
    std::replace(newDate.begin(), newDate.end(), '-', ':');
    const auto offset = date.substr(offsetPos);
    return ParseTimestamp(newDate, offset, "");
}

expr_t File::ParseTimestampMov(const std::string& date) {
    try {
        return std::stol(date) - 2082844800L;
    } catch (...) {
        return EMPTY_EXPR_T;
    }
}

expr_t File::ParseLatLong(const std::vector<double>& coord,
                          const std::string& ref)
{
    /*
     * input:
     *     coord = [degrees, minutes, seconds]
     *     ref = (N|S|W|E)
     * output: value in [-324000000, 324000000] milliarcseconds
     */
    if (coord.size() < 3) {
        return EMPTY_EXPR_T;
    }

    auto mas = static_cast<expr_t>(coord[0] * 3600000.0 + (coord[1] * 60000.0)
                                   + coord[2] * 1000.0);

    if (ref == "S" || ref == "W") {
        mas *= -1;
    }

    return mas;
}

const Exiv2::Image::UniquePtr File::openExiv2Image() const {
    const auto path = _helper->getLocalCopyFilePath(_id);
    Exiv2::Image::UniquePtr image;
    try {
        image = Exiv2::ImageFactory::open(path);
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

expr_t File::ParseAltitude(double altitude, char ref) {
    /*
     * input:
     *    altitude: altitude in meters
     *    ref: 0 = below sea level, 1 = above sea level
     */
    auto res = static_cast<expr_t>(altitude * 1000.0);
    if (ref == 0 && res > 0) {
        res *= -1;
    }
    return res;
}
#endif  /* ENABLE_EXIV2 */
