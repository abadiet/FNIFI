#ifndef FNIFI_FILE_KIND_HPP
#define FNIFI_FILE_KIND_HPP


namespace fnifi {
namespace file {

enum Kind {
    BMP,
    GIF,
    JPEG2000,
    JPEG,
    PNG,
    WEBP,
    AVIF,
    PBM,
    PGM,
    PPM,
    PXM,
    PFM,
    SR,
    RAS,
    TIFF,
    EXR,
    HDR,
    PIC,
    UNKNOWN,
};

}  /* namespace file */
}  /* namespace fnifi */

#endif  /* FNIFI_FILE_KIND_HPP */
