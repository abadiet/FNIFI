#include "fnifi/file/File.hpp"
#include <exiv2/exiv2.hpp>


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

struct stat File::getStats() const {
    return _helper->getStats(_id);
}

Kind File::getKind() {
    return _helper->getKind(_id);
}

std::ostream& File::getMetadata(std::ostream& os, expression::Kind type,
                                const std::string& key) const {
    DLOG("File", this, "Receive request for metadata of type " << type
         << " and key \"" << key << "\"")

    const auto data = _helper->read(_id);

    switch (type) {
        case expression::CTIME:
            {
                const auto time = _helper->getStats(_id).st_ctimespec;
                os << (S_TO_NS(time.tv_sec) + time.tv_nsec);
            }
            break;
        case expression::XMP:
        case expression::EXIF:
        case expression::IPTC:
            {
                Exiv2::Image::UniquePtr image;
                try {
                    image = Exiv2::ImageFactory::open(
                        static_cast<const Exiv2::byte*>(data.data()),
                        data.size());
                } catch (Exiv2::Error&) {
                    /* This is not an image */
                    return os;
                }
                if (image.get() == nullptr) {
                    /* This is not an image */
                    return os;
                }
                image->readMetadata();

                switch (type) {
                    case expression::XMP:
                        {
                            const auto& meta = image->xmpData();
                            const auto pos = meta.findKey(Exiv2::XmpKey(key));
                            if (pos != meta.end()) {
                                os << pos->value();
                            }
                        }
                        break;
                    case expression::EXIF:
                        {
                            const auto& meta = image->exifData();
                            const auto pos = meta.findKey(Exiv2::ExifKey(key));
                            if (pos != meta.end()) {
                                os << pos->value();
                            }
                        }
                        break;
                    case expression::IPTC:
                        {
                            const auto& meta = image->iptcData();
                            const auto pos = meta.findKey(Exiv2::IptcKey(key));
                            if (pos != meta.end()) {
                                os << pos->value();
                            }
                        }
                        break;
                    case expression::CTIME:
                    case expression::UNKOWN:
                        /* impossible */
                        break;
                }
            }
            break;
        case expression::UNKOWN:
            const auto msg("Bad Metadata's type");
            ELOG("File", this, msg)
            throw std::runtime_error(msg);
    }

    return os;
}

fileBuf_t File::read() const {
    return _helper->read(_id);
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
