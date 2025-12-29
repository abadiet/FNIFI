#ifndef FNIFI_EXPRESSION_KIND_HPP
#define FNIFI_EXPRESSION_KIND_HPP

namespace fnifi {
namespace expression {

enum Kind {
    UNKNOWN = 0,  /* unknown => throws an error */
    XMP = 1,      /* XMP generic metadata */
    EXIF = 2,     /* EXIF generic metadata */
    IPTC = 3,     /* IPTC generic metadata */
    KIND = 4,  /* kind of file. See fnifi::file::Kind for more */
    SIZE = 5,  /* in bytes */
    CTIME = 6,    /* original creation time (for instance, when the photo/video has been shot),
                 fallback to system's ctime (e.g. when the file has been
                 created) */
    MTIME = 7,    /* last content modification time (for instance, when the photo has been edited)
              , fallback to system's mtime (e.g. when the file has been modified) */
    WIDTH = 8,
    HEIGHT = 9,
    DURATION = 10,  /* in nanoseconds, fallback to 0 */
    LATITUDE = 11,  /* latitude, in milliarcseconds ([-324000000, 32400000]), where the file has been created */
    LONGITUDE = 12,  /* longitude, in milliarcseconds ([-324000000, 32400000]), where the file has been created */
    ALTITUDE = 13,  /* altitude, in meters from the sea level, at the original location of the file creation */
    // ORIENTATION,  /* orientation where the file has been created */
    // SPEED,  /* speed when the file has been created */
    // EXPOSURE_TIME,
    // F_NUMBER,
    // ISO,
    // SHUTTER_SPEED,
    // APERTURE,
    // FOCAL_LENGHT,
    // FLASH,  /* boolean value indicating if the flash was active or not */
};

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_EXPRESSION_KIND_HPP */
