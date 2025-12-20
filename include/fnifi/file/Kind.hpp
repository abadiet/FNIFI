#ifndef FNIFI_FILE_KIND_HPP
#define FNIFI_FILE_KIND_HPP


namespace fnifi {
namespace file {

enum Kind {
    /* images */
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
    /* videos: opencv use ffmpeg to handle videos so pretty much any file
     * formats can be handled. Here are just the most common ones. */
    MKV, /* WebM as well */
    AVI,
    MTS,
    MOV,
    WMV,
    YUV,
    MP4,
    M4V,
    /* others */
    UNKNOWN,
};

}  /* namespace file */
}  /* namespace fnifi */

#endif  /* FNIFI_FILE_KIND_HPP */
