#ifndef FNIFI_UTILS_HPP
#define FNIFI_UTILS_HPP

#include <vector>

#define UNUSED(x) (void)x
#define TODO throw std::runtime_error("Not yet implemented");


namespace fnifi {

typedef std::vector<char> fileBuf_t;
typedef unsigned int fileId_t;
typedef unsigned int expr_t;

}  /* namespace fnifi */

#endif  /* FNIFI_UTILS_HPP */
