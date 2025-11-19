#ifndef FNIFI_UTILS_TEMPFILE_HPP
#define FNIFI_UTILS_TEMPFILE_HPP

#include <fstream>
#include <filesystem>
#include <cstdlib>


namespace fnifi {
namespace utils {

class TempFile : public std::fstream {
public:
    TempFile();
    ~TempFile() override;
    std::filesystem::path getPath() const;

private:
    static char RandomChar();
    static std::string RandomString(unsigned char size);
    static std::filesystem::path RandomFilepath(unsigned char size);

    static const char _charset[63];
    const std::filesystem::path _path;
};

}  /* namespace utils */
}  /* namespace fnifi */

#endif  /* FNIFI_UTILS_TEMPFILE_HPP */
