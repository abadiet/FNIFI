#include "fnifi/file/AFileHelper.hpp"


using namespace fnifi;
using namespace fnifi::file;

AFileHelper::AFileHelper(const utils::SyncDirectory& storing,
                         const std::filesystem::path& storingPath)
: _storing(storing), _storingPath(storingPath)
{}

AFileHelper::~AFileHelper() {}
