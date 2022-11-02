#include "TestUtil.h"
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace ftg {

std::string getTmpDirPath() { return fs::temp_directory_path().string(); }

std::string getUniqueFilePath(std::string Dir, std::string Name,
                              std::string Ext) {
  unsigned Counter = 0;
  fs::path Result = fs::path(Dir) / fs::path(Name);
  if (!Ext.empty())
    Result += "." + Ext;

  while (fs::exists(Result)) {
    Result = fs::path(Dir) / fs::path(Name + '-' + std::to_string(Counter++));
    if (!Ext.empty())
      Result += "." + Ext;
  }

  return Result;
}

} // namespace ftg
