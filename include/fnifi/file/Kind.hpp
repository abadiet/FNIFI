#ifndef FNIFI_FILE_KIND_HPP
#define FNIFI_FILE_KIND_HPP


namespace fnifi {
namespace file {

enum Kind {
    UNKNOWN = 0,
    /* images */
    BMP = 1,
    JPEG2000 = 2,
    JPEG = 3,
    PNG = 4,
    WEBP = 5,
    AVIF = 6,
    PBM = 7,
    PGM = 8,
    PPM = 9,
    PXM = 10,
    PFM = 11,
    SR = 12,
    RAS = 13,
    TIFF = 14,
    EXR = 15,
    HDR = 16,
    PIC = 17,
    GIF = 18,
    /* videos: opencv use ffmpeg to handle videos so pretty much any file
     * formats can be handled. Here are just the most common ones. */
    AVI = 19,
    MTS = 20,
    MOV = 21,
    WMV = 22,
    YUV = 23,
    MP4 = 24,
    M4V = 25,
    MKV = 26, /* WebM as well */
    /* others */
};

}  /* namespace file */
}  /* namespace fnifi */

#endif  /* FNIFI_FILE_KIND_HPP */
