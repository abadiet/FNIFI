#ifndef FNIFI_UTILS_HPP
#define FNIFI_UTILS_HPP

#include <vector>
#include <ostream>
#include <fstream>
#include <iostream>
#include <chrono>
#include <format>

#ifdef _WIN32
#include <winnt.h>
#else  /* _WIN32 */
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#define FILE_ATTRIBUTE_REPARSE_POINT 0x400
#endif  /* _WIN32 */
#define DOS_ISDIR(dos) ((dos & FILE_ATTRIBUTE_DIRECTORY) != 0)
#define DOS_ISREG(dos)                                                        \
    ((dos & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) == 0)

#define UNUSED(x) (void)x
#define TODO throw std::runtime_error("Not yet implemented");

#ifdef FNIFI_DEBUG
#define LOG                                                                   \
    std::clog << "["                                                          \
        << std::format("{:%Y-%m-%d %H:%M:%S}",                                \
                       std::chrono::system_clock::now())                      \
        << "] "
#define DLOG(x) LOG << "[DEBUG] " << x << std::endl;
#define ILOG(x) LOG << "[INFO]  " << x << std::endl;
#define WLOG(x) LOG << "[WARN]  " << x << std::endl;
#define ELOG(x) LOG << "[ERROR] " << x << std::endl;
#else  /* FNIFI_DEBUG */
#define DLOG(x)
#define ILOG(x)
#define WLOG(x)
#define ELOG(x)
#endif  /* FNIFI_DEBUG */


namespace fnifi {

typedef std::vector<unsigned char> fileBuf_t;
typedef unsigned int fileId_t;
typedef unsigned int expr_t;

template <typename T>
std::ostream& Serialize(std::ostream& os, const T& var);
template <typename T>
std::ostream& Serialize(std::ostream& os, const std::vector<T>& var);
template <typename T>
std::istream& Deserialize(std::istream& is, T& var);
template <typename T>
std::istream& Deserialize(std::istream& is, std::vector<T>& var);

bool operator>(const timespec& lhs, const timespec& rhs);

}  /* namespace fnifi */


/* IMPLEMENTATIONS */


template <typename T>
std::ostream& fnifi::Serialize(std::ostream& os, const T& var) {
   return os.write(reinterpret_cast<const char*>(&var), sizeof(var));
}

template <typename T>
std::ostream& fnifi::Serialize(std::ostream& os, const std::vector<T>& var) {
    size_t size = var.size();
    os.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& elem : var) {
        Serialize(elem, os);
    }
    return os;
}

template <typename T>
std::istream& fnifi::Deserialize(std::istream& is, T& var) {
    return is.read(reinterpret_cast<char*>(&var), sizeof(var));
}

template <typename T>
std::istream& fnifi::Deserialize(std::istream& is, std::vector<T>& var) {
    size_t size;
    is.read(reinterpret_cast<char*>(&size), sizeof(size));
    var.clear();
    var.resize(size);
    for (size_t i = 0; i < size; i++) {
        Deserialize(var[i], is);
    }
    return is;
}

inline bool fnifi::operator>(const timespec& lhs, const timespec& rhs) {
    if (lhs.tv_sec == rhs.tv_sec) {
        return lhs.tv_nsec > rhs.tv_nsec;
    }
    return lhs.tv_sec > rhs.tv_sec;
}

#endif  /* FNIFI_UTILS_HPP */
