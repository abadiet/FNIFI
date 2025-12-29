#include "fnifi/file/File.hpp"
#include <vector>


using namespace fnifi;
using namespace fnifi::file;

bool File::pCompare::operator()(const File* a, const File* b) const {
    return a->_sortScore < b->_sortScore;
}

File::File(fileId_t id, AFileHelper* helper)
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

void File::setHelper(AFileHelper* helper) {
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
