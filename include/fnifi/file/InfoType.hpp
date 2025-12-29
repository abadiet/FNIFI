#ifndef FNIFI_FILE_INFOTYPE_HPP
#define FNIFI_FILE_INFOTYPE_HPP
#include <string>
#include <concepts>


namespace fnifi {
namespace file {

template<typename T>
concept InfoType =
    std::integral<T> || std::floating_point<T>;
    /* TODO: || std::same_as<T, std::string>; */

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_FILE_INFOTYPE_HPP */
