#ifndef FNIFI_FILE_INFO_HPP
#define FNIFI_FILE_INFO_HPP

#include "fnifi/file/AFileHelper.hpp"
#include "fnifi/file/File.hpp"
#include "fnifi/file/InfoType.hpp"
#include "fnifi/expression/Kind.hpp"
#include "fnifi/utils/SyncDirectory.hpp"
#include "fnifi/utils/utils.hpp"
#ifdef ENABLE_EXIV2
#include <exiv2/exiv2.hpp>
#endif  /* ENABLE_EXIV2 */
#include <string>
#include <filesystem>
#include <unordered_map>
#include <memory>
#include <type_traits>
#include <string>

#define SEP std::string("\xFF")
#define INFO_DIRNAME "info"
#define EMPTY_INFO_VALUE std::numeric_limits<T>::max()
#define NOTFOUND_INFO_VALUE std::numeric_limits<T>::max() - 1


namespace fnifi {
namespace file {

template<fnifi::file::InfoType T>
class Info {
public:
    static void Uncache(fileId_t id);
    static void Free();
    static Info<T>* Build(const AFileHelper* helper,
                          expression::Kind kind, const std::string& key = "");

    bool get(const File* file, T& result);
    void disableSync(bool pull = true);
    void enableSync(bool push = true);

private:
    static std::string GetTypeName();
    Info(const AFileHelper* helper, expression::Kind kind,
         const std::string& key);
    bool getValue(const File* file, T& result) const;

#ifdef ENABLE_EXIV2
    static T ParseTimestamp(const std::string& date,
                                 const std::string& offset = "",
                                 const std::string& subsec = "");
    static T ParseTimestampISO8601(const std::string& date);
    static T ParseTimestampMov(const std::string& date);
    static T ParseLatLong(const std::vector<double>& coord,
                               const std::string& ref);
    static T ParseAltitude(double altitude, char ref);
    const Exiv2::Image::UniquePtr openExiv2Image(const File* file) const;
    template<typename U>
    static bool GetMetadata(const Exiv2::Image::UniquePtr& image,
                            expression::Kind kind, const std::string& key,
                            U& res);
    template<typename Iterator, typename U>
    static bool ProcessExiv2FindRes(Iterator it, U& res, size_t i = 0);
#endif  /* ENABLE_EXIV2 */

    static std::unordered_map<std::string, Info> _built;

    const expression::Kind _kind;
    const std::string _key;
    std::unique_ptr<utils::SyncDirectory::FileStream> _file;
    fileId_t _maxId;
    const size_t _typeSz;
};

}  /* namespace expression */
}  /* namespace fnifi */


/* IMPLEMENTATIONS */

template<fnifi::file::InfoType T>
bool fnifi::file::File::get(T& result, expression::Kind kind,
                            const std::string& key) const
{
    const auto info = Info<T>::Build(_helper, kind, key);
    return info->get(this, result);
}

template<fnifi::file::InfoType T>
std::unordered_map<std::string, fnifi::file::Info<T>>
fnifi::file::Info<T>::_built;

template<fnifi::file::InfoType T>
void fnifi::file::Info<T>::Uncache(fileId_t id) {
    DLOG("Info", "(static)", "Uncaching file id " << id)

    for (auto& info: _built) {
        /* write an empty results on the id position */
        info.second._file->seekp(std::streamoff(id * info.second._typeSz));
        utils::Serialize(*info.second._file, EMPTY_INFO_VALUE);
        info.second._file->push();
    }
}

template<fnifi::file::InfoType T>
void fnifi::file::Info<T>::Free() {
    DLOG("Info", "(static)", "Cleaning")

    for (auto& info: _built) {
        info.second._file->close();
    }
    _built.clear();
}

template<fnifi::file::InfoType T>
fnifi::file::Info<T>* fnifi::file::Info<T>::Build(
    const AFileHelper* helper, expression::Kind kind,
    const std::string& key)
{
    std::ostringstream kindStr;
    kindStr << kind;
    const auto hsh = helper->getName() + SEP + kindStr.str() + SEP + key + SEP
        + GetTypeName();

    const auto pos = _built.find(hsh);
    if (pos != _built.end()) {
        /* the object already exists */
        return &pos->second;
    }

    auto info = _built.insert(std::make_pair(hsh, Info<T>(helper, kind, key)));
    return &info.first->second;
}

template<fnifi::file::InfoType T>
bool fnifi::file::Info<T>::get(const File* file, T& result) {
    const auto id = file->getId();
    const auto pos = id * _typeSz;

    if (_file->pull()) {
        /* update maxId */
        _file->seekg(0, std::ios::end);
        _maxId = static_cast<fileId_t>(static_cast<size_t>(
            _file->tellg()) / sizeof(T));
    }

    if (id <= _maxId) {
        /* the value may be saved */
        T res;
        _file->seekg(std::streamoff(pos));
        utils::Deserialize(*_file, res);

        if (res != EMPTY_INFO_VALUE) {
            /* the value was saved */
            result = res;
            return true;
        }

        _file->seekg(std::streamoff(pos));
    } else {
        /* filling the file up to the position of the value */
        _file->seekp(0, std::ios::end);
        for (auto i = _maxId; i < id; ++i) {
            /* TODO avoid multiples std::ofstream::write calls */
            utils::Serialize(*_file, EMPTY_INFO_VALUE);
        }
        _maxId = id;
    }

    DLOG("Info", this, "Results for File " << file << " was not cached")

    T res;
    const auto valid = getValue(file, res);
    if (!valid) {
        res = NOTFOUND_INFO_VALUE;
    }
    result = res;

    _file->seekp(std::streamoff(pos));
    utils::Serialize(*_file, res);

    _file->push();

    return valid;
}

template<fnifi::file::InfoType T>
void fnifi::file::Info<T>::disableSync(bool pull) {
    _file->disableSync(pull);
}

template<fnifi::file::InfoType T>
void fnifi::file::Info<T>::enableSync(bool pull) {
    _file->enableSync(pull);
}

template<fnifi::file::InfoType T>
std::string fnifi::file::Info<T>::GetTypeName() {
    std::ostringstream oss;
    if constexpr (std::is_integral_v<T>) {
        if constexpr (std::is_unsigned_v<T>) {
            oss << "u";
        }
        oss << "int" << sizeof(T) * 8;
    } else if constexpr (std::is_floating_point_v<T>) {
        if constexpr (std::is_unsigned_v<T>) {
            oss << "u";
        }
        oss << "float" << sizeof(T) * 8;
    } else if constexpr (std::is_same_v<T, std::string>) {
        TODO
        oss << "string";
    } else {
        throw std::runtime_error("Unkown type");
    }
    return oss.str();
}

template<fnifi::file::InfoType T>
fnifi::file::Info<T>::Info(const fnifi::file::AFileHelper* helper,
                         fnifi::expression::Kind kind, const std::string& key)
: _kind(kind), _key(key), _maxId(0), _typeSz(sizeof(T))
{
    DLOG("Info", this, "Instanciation for coll " << &helper << ", type "
         << typeid(T).name() << ", kind " << kind << " and key \"" << key
         << "\"")

    std::ostringstream kindName;
    kindName << kind << key;

    const auto filepath = helper->_storingPath / INFO_DIRNAME /
        utils::Hash(GetTypeName()) / utils::Hash(kindName.str());
    bool ate = false;
    if (helper->_storing.exists(filepath)) {
        ate = true;
    }

    _file = std::make_unique<utils::SyncDirectory::FileStream>(
        helper->_storing, filepath, true);

    if (ate) {
        /* the file already existed */
        _maxId = static_cast<fileId_t>(static_cast<size_t>(_file->tellg())
            / sizeof(T));
    }
}

template<fnifi::file::InfoType T>
bool fnifi::file::Info<T>::getValue(const File* file, T& result) const {
    DLOG("Info", this, "Retrieveing value")

    switch (_kind) {
        case expression::KIND:
        {
            result = static_cast<T>(file->getKind());
            return true;
        }
        case expression::SIZE:
        {
            const auto path = file->getLocalCopyPath();
            result = static_cast<T>(std::filesystem::file_size(path));
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
            const auto image = openExiv2Image(file);
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
                            result = val;
                            return true;
                        }
                    }
                }
            }
#endif  /* ENABLE_EXIV2 */
            const auto time = file->getStats().st_ctimespec;
            result = static_cast<T>(S_TO_NS(time.tv_sec) + time.tv_nsec);
            return true;
        }
        case expression::MTIME:
        {
#ifdef ENABLE_EXIV2
            /*
             * TODO: to consider
             * Xmp.tiff.DateTime
             */
            const auto image = openExiv2Image(file);
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
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
                        if (val != EMPTY_INFO_VALUE) {
                            result = val;
                            return true;
                        }
                    }
                }
            }
#endif  /* ENABLE_EXIV2 */
            const auto time = file->getStats().st_mtimespec;
            result = static_cast<T>(S_TO_NS(time.tv_sec) + time.tv_nsec);
            return true;
        }
        case expression::WIDTH:
        {
#ifdef ENABLE_EXIV2
            const auto image = openExiv2Image(file);
            if (image != nullptr) {
                {
                    T val;
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
                                result = static_cast<T>(val);
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
                                result = static_cast<T>(val);
                                return true;
                            }
                        } catch (...) {
                        }
                    }
                }
            }
            WLOG("Info", this, "Cannot find width")
            return false;
#else  /* ENABLE_EXIV2 */
            WLOG("Info", this, "Cannot retrieve width as Exiv2 is disabled")
            return false;
#endif  /* ENABLE_EXIV2 */
        }
        case expression::HEIGHT:
        {
#ifdef ENABLE_EXIV2
            const auto image = openExiv2Image(file);
            if (image != nullptr) {
                {
                    T val;
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
                                result = static_cast<T>(val);
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
                                result = static_cast<T>(val);
                                return true;
                            }
                        } catch (...) {
                        }
                    }
                }
            }
            WLOG("Info", this, "Cannot find height")
            return false;
#else  /* ENABLE_EXIV2 */
            WLOG("Info", this, "Cannot retrieve height as Exiv2 is disabled")
            return false;
#endif  /* ENABLE_EXIV2 */
        }
        case expression::DURATION:
        {
#ifdef ENABLE_EXIV2
            const auto image = openExiv2Image(file);
            if (image != nullptr) {
                {
                    std::string valStr;
                    if (GetMetadata(image, expression::Kind::XMP,
                                    "Xmp.video.Duration", valStr))
                    {
                        try {
                            const auto val = std::stol(valStr);
                            if (val != 0) {
                                result = static_cast<T>(val * 1000000L);
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
                                result = static_cast<T>(val * 1000000000L);
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
                                result = static_cast<T>(val * 1000000000L);
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
                                result = static_cast<T>(val * 1000000000L);
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
                                result = static_cast<T>(val * 1000000000L);
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
                                    result = static_cast<T>(n * us * 1000L);
                                    return true;
                                }
                            } catch (...) {
                            }
                        }
                    }
                }
            }
            WLOG("Info", this, "Cannot find duration: assuming null")
            result = 0;
            return true;
#else  /* ENABLE_EXIV2 */
            WLOG("Info", this, "Cannot retrieve duration as Exiv2 is disabled")
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
            const auto image = openExiv2Image(file);
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
                        if (val != EMPTY_INFO_VALUE) {
                            result = val;
                            return true;
                        }
                    }
                }
            }
            WLOG("Info", this, "Cannot find latitude")
            return false;
#else  /* ENABLE_EXIV2 */
            WLOG("Info", this, "Cannot retrieve latitude as Exiv2 is disabled")
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
            const auto image = openExiv2Image(file);
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
                        if (val != EMPTY_INFO_VALUE) {
                            result = val;
                            return true;
                        }
                    }
                }
            }
            WLOG("Info", this, "Cannot find longitude")
            return false;
#else  /* ENABLE_EXIV2 */
            WLOG("Info", this, "Cannot retrieve longitude as Exiv2 is "
                 "disabled")
            return false;
#endif  /* ENABLE_EXIV2 */
        }
        case expression::ALTITUDE:
        {
#ifdef ENABLE_EXIV2
            const auto image = openExiv2Image(file);
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
            WLOG("Info", this, "Cannot find altitude")
            return false;
#else  /* ENABLE_EXIV2 */
            WLOG("Info", this, "Cannot retrieve altitude as Exiv2 is disabled")
            return false;
#endif  /* ENABLE_EXIV2 */
        }

        case expression::EXIF:
        case expression::XMP:
        case expression::IPTC:
        {
#ifdef ENABLE_EXIV2
            auto image = openExiv2Image(file);
            if (image == nullptr) {
                return false;
            }
            T val;
            if (GetMetadata(image, _kind, _key, val)) {
                result = val;
                return val;
            }
            WLOG("Info", this, "Failed to retrieve metadata for kind "
                 << _kind << " and key \"" << _key << "\": val=" << val)
            return false;
#else  /* ENABLE_EXIV2 */
            WLOG("Info", this, "Cannot retrieve metadata for kind " << _kind
                 << " and key \"" << _key << "\" as Exiv2 is disabled")
            return false;
#endif  /* ENABLE_EXIV2 */
        }
        case expression::UNKNOWN:
            throw std::runtime_error("Bad metadata's kind");
    }
}

#ifdef ENABLE_EXIV2
template<fnifi::file::InfoType T>
T fnifi::file::Info<T>::ParseTimestamp(const std::string& date,
    const std::string& offset, const std::string& subsec)
{
    std::istringstream iss(date);
    try {
        std::tm tm = {};
        iss >> std::get_time(&tm, "%Y:%m:%d %H:%M:%S");
        if (iss.fail()) {
            return EMPTY_INFO_VALUE;
        }

        const auto time = std::mktime(&tm) - timezone;
        if (time == -1) {
            return EMPTY_INFO_VALUE;
        }

        const auto tp = std::chrono::system_clock::from_time_t(time);
        auto nstime = static_cast<T>(std::chrono::duration_cast<
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

        return static_cast<T>(nstime);
    } catch (...) {
        return EMPTY_INFO_VALUE;
    }
}

template<fnifi::file::InfoType T>
T fnifi::file::Info<T>::ParseTimestampISO8601(const std::string& date) {
    const auto offsetPos = date.find_last_of("+-");
    auto newDate = date.substr(0, offsetPos);
    std::replace(newDate.begin(), newDate.end(), 'T', ' ');
    std::replace(newDate.begin(), newDate.end(), '-', ':');
    const auto offset = date.substr(offsetPos);
    return ParseTimestamp(newDate, offset, "");
}

template<fnifi::file::InfoType T>
T fnifi::file::Info<T>::ParseTimestampMov(const std::string& date) {
    try {
        return static_cast<T>(std::stol(date) - 2082844800L);
    } catch (...) {
        return EMPTY_INFO_VALUE;
    }
}

template<fnifi::file::InfoType T>
T fnifi::file::Info<T>::ParseLatLong(const std::vector<double>& coord,
                          const std::string& ref)
{
    /*
     * input:
     *     coord = [degrees, minutes, seconds]
     *     ref = (N|S|W|E)
     * output: value in [-324000000, 324000000] milliarcseconds
     */
    if (coord.size() < 3) {
        return EMPTY_INFO_VALUE;
    }

    auto mas = static_cast<T>(coord[0] * 3600000.0 + (coord[1] * 60000.0)
                                   + coord[2] * 1000.0);

    if (ref == "S" || ref == "W") {
        mas *= -1;
    }

    return mas;
}

template<fnifi::file::InfoType T>
const Exiv2::Image::UniquePtr fnifi::file::Info<T>::openExiv2Image(
    const File* file) const
{
    const auto path = file->getLocalCopyPath();
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

template<fnifi::file::InfoType T>
T fnifi::file::Info<T>::ParseAltitude(double altitude, char ref) {
    /*
     * input:
     *    altitude: altitude in meters
     *    ref: 0 = below sea level, 1 = above sea level
     */
    auto res = static_cast<T>(altitude * 1000.0);
    if (ref == 0 && res > 0) {
        res *= -1;
    }
    return res;
}

template<fnifi::file::InfoType T>
template<typename U>
bool fnifi::file::Info<T>::GetMetadata(const Exiv2::Image::UniquePtr& image,
    expression::Kind kind, const std::string& key, U& res)
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
        throw std::runtime_error("fnifi::file::Info<T>GetMetadata should be "
                                 "called with kind EXIF, XMP and IPTC only");
    }

    /* key not found */
    return false;
}

template<fnifi::file::InfoType T>
template<typename Iterator, typename U>
bool fnifi::file::Info<T>::ProcessExiv2FindRes(Iterator it, U& res, size_t i) {
    if constexpr (std::is_integral_v<U>) {
        res = static_cast<U>(it->toInt64(i));
        return true;
    } else if constexpr (std::is_floating_point_v<U>) {
        const auto rational = it->toRational(i);
        res = static_cast<U>(rational.first) / static_cast<U>(rational.second);
        return true;
    } else if constexpr (
        std::is_same_v<U, std::vector<typename U::value_type>>)
    {
        res.resize(it->count());
        for (size_t j = 0; j < it->count(); ++j) {
            if (!ProcessExiv2FindRes(it, res[j], j)) {
                return false;
            }
        }
        return true;
    } else if constexpr (std::is_same_v<U, std::string>) {
        res = it->toString(i);
        return true;
    }
    return false;
}
#endif  /* ENABLE_EXIV2 */

#endif  /* FNIFI_FILE_INFO_HPP */
