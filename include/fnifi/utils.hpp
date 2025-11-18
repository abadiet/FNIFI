#ifndef FNIFI_UTILS_HPP
#define FNIFI_UTILS_HPP

#include <vector>
#include <ostream>
#include <fstream>
#include <iostream>
#include <chrono>
#include <format>

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

#define S_TO_NS(x) (x * 1000000000)

#define EMPTY_EXPR_T std::numeric_limits<expr_t>::max()


namespace fnifi {

typedef std::vector<unsigned char> fileBuf_t;
typedef unsigned int fileId_t;
typedef long int expr_t;

template <typename T>
std::ostream& Serialize(std::ostream& os, const T& var);
template <typename T>
std::ostream& Serialize(std::ostream& os, const std::vector<T>& var);
template <typename T>
std::istream& Deserialize(std::istream& is, T& var);
template <typename T>
std::istream& Deserialize(std::istream& is, std::vector<T>& var);

bool operator>(const timespec& lhs, const timespec& rhs);

std::string Hash(const std::string& s);

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

inline std::string fnifi::Hash(const std::string& s) {
    auto res = s;
    for (char c : "/\\:*?\"<>|") {
        std::replace(res.begin(), res.end(), c, '_');
    }
    return res;
}

#endif  /* FNIFI_UTILS_HPP */
