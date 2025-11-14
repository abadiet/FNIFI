#ifndef FNIFI_UTILS_HPP
#define FNIFI_UTILS_HPP

#include <vector>
#include <ostream>
#include <istream>

#define UNUSED(x) (void)x
#define TODO throw std::runtime_error("Not yet implemented");


namespace fnifi {

typedef std::vector<char> fileBuf_t;
typedef unsigned int fileId_t;
typedef unsigned int expr_t;

template <typename T>
std::ostream& Serialize(std::ostream& os, const T& var) {
   os.write(reinterpret_cast<const char*>(&var), sizeof(var));
   return os;
}

template <typename T>
std::ostream& Serialize(std::ostream& os, const std::vector<T>& var) {
    size_t size = var.size();
    os.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& elem : var) {
        Serialize(elem, os);
    }
    return os;
}

template <typename T>
std::istream& Deserialize(std::istream& is, T& var) {
    is.read(reinterpret_cast<char*>(&var), sizeof(var));
    return is;
}

template <typename T>
std::istream& Deserialize(std::istream& is, std::vector<T>& var) {
    size_t size;
    is.read(reinterpret_cast<char*>(&size), sizeof(size));
    var.clear();
    var.resize(size);
    for (size_t i = 0; i < size; i++) {
        Deserialize(var[i], is);
    }
    return is;
}

}  /* namespace fnifi */

#endif  /* FNIFI_UTILS_HPP */
