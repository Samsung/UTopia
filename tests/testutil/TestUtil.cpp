#include "TestUtil.h"
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace ftg {

std::string getClangVersion() {
  std::array<char, 128> Buffer;
  std::string Result;
  const std::string CMD =
      "clang -dM -E -x c /dev/null | grep __clang_version__";
  std::unique_ptr<FILE, decltype(&pclose)> Pipe(popen(CMD.c_str(), "r"),
                                                pclose);
  if (!Pipe)
    return Result;
  while (fgets(Buffer.data(), Buffer.size(), Pipe.get()) != nullptr)
    Result += Buffer.data();

  auto SIdx = Result.find("\"");
  auto EIdx = Result.rfind("\"");
  if (SIdx == std::string::npos || EIdx == std::string::npos ||
      SIdx + 2 >= EIdx)
    return "";

  Result = Result.substr(SIdx + 1, EIdx - SIdx - 2);
  return Result;
}

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
