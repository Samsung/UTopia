#include "SourceFileManager.h"
#include "TestUtil.h"
#include "ftg/utils/FileUtil.h"
#include <experimental/filesystem>

using namespace ftg;
namespace fs = std::experimental::filesystem;

SourceFileManager::SourceFileManager(bool Debug)
    : Debug(Debug),
      BaseDir(getUniqueFilePath(getTmpDirPath(), "FTGTestSource")) {
  fs::create_directories(BaseDir);
  fs::create_directories(getSrcDirPath());
}

SourceFileManager::~SourceFileManager() {
  if (!Debug && fs::exists(BaseDir))
    fs::remove_all(BaseDir);
}

bool SourceFileManager::addFile(std::string Path) {
  auto AbsolutePath = fs::absolute(Path);
  if (AbsolutePath.string().find(BaseDir) != 0)
    return false;
  auto FileName = AbsolutePath.filename();
  auto Result = ManagedFiles.emplace(FileName, AbsolutePath);
  return Result.second;
}

bool SourceFileManager::createFile(std::string Name, std::string Content) {
  auto Path = fs::path(getSrcDirPath()) / Name;
  if (!util::saveFile(Path.c_str(), Content.c_str()))
    return false;
  ManagedFiles.emplace(Name, Path);
  return true;
}

std::string SourceFileManager::getFilePath(std::string Name) const {
  auto Iter = ManagedFiles.find(Name);
  if (Iter == ManagedFiles.end())
    return "";
  return Iter->second;
}

std::string SourceFileManager::getOutDirPath() const {
  return (fs::path(BaseDir) / fs::path("out")).string();
}

std::string SourceFileManager::getSrcDirPath() const {
  return (fs::path(BaseDir) / fs::path("src")).string();
}
