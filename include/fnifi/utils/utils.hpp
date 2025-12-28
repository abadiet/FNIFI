#ifndef FNIFI_UTILS_UTILS_HPP
#define FNIFI_UTILS_UTILS_HPP

#include <vector>
#include <ostream>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <format>
#include <thread>
#include <condition_variable>
#include <mutex>

#define UNUSED(x) (void)x;
#define TODO throw std::runtime_error("Not yet implemented");

#ifdef FNIFI_DEBUG
/* TODO: using mutex for logs can lead to some serious blocking situations */
#define LOG                                                                   \
    {                                                                         \
    std::unique_lock logLk(fnifi::utils::logMtx);                             \
    fnifi::utils::logCv.wait(logLk, []{ return true; });                      \
    const auto logTm = std::chrono::system_clock::to_time_t(                  \
        std::chrono::system_clock::now());                                    \
    std::clog << "["                                                          \
        << std::put_time(std::localtime(&logTm), "%Y-%m-%d %H:%M:%S")         \
        << "] [" << std::this_thread::get_id() << "] "
#define DLOG(obj, id, x) LOG << "[DEBUG] " << "[" << obj << " " << id << "] " \
    << x << std::endl;                                                        \
    logLk.unlock();                                                           \
    fnifi::utils::logCv.notify_one();                                         \
    }
#define ILOG(obj, id, x) LOG << "[INFO]  " << "[" << obj << " " << id << "] " \
    << x << std::endl;                                                        \
    logLk.unlock();                                                           \
    fnifi::utils::logCv.notify_one();                                         \
    }
#define WLOG(obj, id, x) LOG << "[WARN]  " << "[" << obj << " " << id << "] " \
    << x << std::endl;                                                        \
    logLk.unlock();                                                           \
    fnifi::utils::logCv.notify_one();                                         \
    }
#define ELOG(obj, id, x) LOG << "[ERROR] " << "[" << obj << " " << id << "] " \
    << x << std::endl;                                                        \
    logLk.unlock();                                                           \
    fnifi::utils::logCv.notify_one();                                         \
    }
#else  /* FNIFI_DEBUG */
#define DLOG(obj, id, x)
#define ILOG(obj, id, x)
#define WLOG(obj, id, x)
#define ELOG(obj, id, x)
#endif  /* FNIFI_DEBUG */

#define S_TO_NS(x) (x * 1000000000)

#define EMPTY_EXPR_T std::numeric_limits<expr_t>::max()


namespace fnifi {

typedef std::vector<unsigned char> fileBuf_t;
typedef unsigned int fileId_t;
typedef long int expr_t;

bool operator>(const timespec& lhs, const timespec& rhs);


namespace utils {

static std::mutex logMtx;
static std::condition_variable logCv;

template <typename T>
std::ostream& Serialize(std::ostream& os, const T& var);
template <typename T>
std::ostream& Serialize(std::ostream& os, const std::vector<T>& var);
template <typename T>
std::istream& Deserialize(std::istream& is, T& var);
template <typename T>
std::istream& Deserialize(std::istream& is, std::vector<T>& var);

uint32_t fnv1a(const std::string& s);
std::string Hash(const std::string& s);

}  /* namespace utils */
}  /* namespace fnifi */

/* IMPLEMENTATIONS */


template <typename T>
std::ostream& fnifi::utils::Serialize(std::ostream& os, const T& var) {
   return os.write(reinterpret_cast<const char*>(&var), sizeof(var));
}

template <typename T>
std::ostream& fnifi::utils::Serialize(std::ostream& os,
                                      const std::vector<T>& var)
{
    size_t size = var.size();
    os.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& elem : var) {
        Serialize(elem, os);
    }
    return os;
}

template <typename T>
std::istream& fnifi::utils::Deserialize(std::istream& is, T& var) {
    return is.read(reinterpret_cast<char*>(&var), sizeof(var));
}

template <typename T>
std::istream& fnifi::utils::Deserialize(std::istream& is, std::vector<T>& var)
{
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

inline uint32_t fnifi::utils::fnv1a(const std::string& s) {
    const uint32_t FNV_PRIME = 16777619;
    const uint32_t FNV_OFFSET = 2166136261;
    uint32_t hash = FNV_OFFSET;
    for (char c : s) {
        hash ^= static_cast<uint8_t>(c);
        hash *= FNV_PRIME;
    }
    return hash;
}

inline std::string fnifi::utils::Hash(const std::string& s) {
    auto res = s;
    for (char c : "/\\:*?\"<>|") {
        std::replace(res.begin(), res.end(), c, '_');
    }
    res += "_" + std::to_string(fnv1a(s));
    return res;
}

#endif  /* FNIFI_UTILS_UTILS_HPP */
