#include "fnifi/file/File.hpp"
#include <exiv2/exiv2.hpp>


using namespace fnifi;
using namespace fnifi::file;

File::File(fileId_t id, IFileHelper* helper)
: _id(id), _helper(helper)
{

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

struct stat File::getStats() const {
    return _helper->getStats(_id);
}

std::ostream& File::getMetadata(std::ostream& os, MetadataType type,
                                const char* key) const {
    const auto data = _helper->read(_id);

    switch (type) {
        case XMP:
        case EXIF:
        case IPTC:
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
                    case XMP:
                        {
                            const auto& meta = image->xmpData();
                            const auto pos = meta.findKey(Exiv2::XmpKey(key));
                            if (pos != meta.end()) {
                                os << pos->value();
                            }
                        }
                        break;
                    case EXIF:
                        {
                            const auto& meta = image->exifData();
                            const auto pos = meta.findKey(Exiv2::ExifKey(key));
                            if (pos != meta.end()) {
                                os << pos->value();
                            }
                        }
                        break;
                    case IPTC:
                        {
                            const auto& meta = image->iptcData();
                            const auto pos = meta.findKey(Exiv2::IptcKey(key));
                            if (pos != meta.end()) {
                                os << pos->value();
                            }
                        }
                        break;
                }
            }
            break;
    }

    return os;
}

fileBuf_t File::preview() const {
    return _helper->preview(_id);
}

fileBuf_t File::read() const {
    return _helper->read(_id);
}
