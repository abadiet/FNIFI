#ifndef FNIFI_EXPRESSION_KIND_HPP
#define FNIFI_EXPRESSION_KIND_HPP

namespace fnifi {
namespace expression {

enum Kind {
    CTIME,   /* creation time */
    XMP,     /* XMP generic metadata */
    EXIF,    /* EXIF generic metadata */
    IPTC,    /* IPTC generic metadata */
    UNKOWN,  /* unkown => throws an error */
};

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_EXPRESSION_KIND_HPP */
