#ifndef FNIFI_EXPRESSION_KIND_HPP
#define FNIFI_EXPRESSION_KIND_HPP

namespace fnifi {
namespace expression {

enum Kind {
    KIND,  /* kind of file. See fnifi::file::Kind for more */
    SIZE,  /* in bytes */
    CTIME,    /* original creation time (for instance, when the photo/video has been shot),
                 fallback to system's ctime (e.g. when the file has been
                 created) */
    MTIME,    /* last content modification time (for instance, when the photo has been edited)
              , fallback to system's mtime (e.g. when the file has been modified) */
    WIDTH,
    HEIGHT,
    DURATION,  /* in nanoseconds, fallback to 0 */
   // LATITUDE,  /* latitude where the file has been created */
   // LONGITUDE,  /* longitude where the file has been created */
   // ALTITUDE,  /* altitude where the file has been created */
   // ORIENTATION,  /* orientation where the file has been created */
   // SPEED,  /* speed when the file has been created */
   // EXPOSURE_TIME,
   // F_NUMBER,
   // ISO,
   // SHUTTER_SPEED,
   // APERTURE,
   // FOCAL_LENGHT,
   // FLASH,  /* boolean value indicating if the flash was active or not */
    XMP,      /* XMP generic metadata */
    EXIF,     /* EXIF generic metadata */
    IPTC,     /* IPTC generic metadata */
    UNKNOWN,  /* unknown => throws an error */
};

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_EXPRESSION_KIND_HPP */
