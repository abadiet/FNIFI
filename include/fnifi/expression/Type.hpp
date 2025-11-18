#ifndef FNIFI_EXPRESSION_TYPE_HPP
#define FNIFI_EXPRESSION_TYPE_HPP

namespace fnifi {
namespace expression {

enum Type {
    CTIME,   /* creation time */
    XMP,     /* XMP generic metadata */
    EXIF,    /* EXIF generic metadata */
    IPTC,    /* IPTC generic metadata */
    UNKOWN,  /* unkown => throws an error */
};

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_EXPRESSION_TYPE_HPP */
