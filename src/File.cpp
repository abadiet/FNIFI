#include "fnifi/file/File.hpp"
#include <exiv2/exiv2.hpp>


using namespace fnifi;
using namespace fnifi::file;

File::File(fileId_t id, Collection* coll)
: _id(id), _coll(coll)
{

}

std::string File::getPath() const {
    return _coll->getFilePath(_id);
}

std::ostream& File::getMetadata(std::ostream& os, MetadataType type,
                                const char* key) const {
    const auto data = _coll->read(_id);

    switch (type) {
        case XMP:
        case EXIF:
        case IPTC:
            {
                const auto image = Exiv2::ImageFactory::open(data.data(),
                                                             data.size());
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
    return _coll->preview(_id);
}

fileBuf_t File::read() const {
    return _coll->read(_id);
}
