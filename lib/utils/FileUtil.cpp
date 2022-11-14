#include "ftg/utils/FileUtil.h"

#include "ftg/utils/StringUtil.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <dirent.h>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sys/stat.h>
#include <vector>

namespace fs = std::experimental::filesystem;

namespace ftg {

namespace util {

static inline std::string getCanonicalPath(fs::path &Path) {
  return fs::canonical(Path).string();
}

std::vector<std::string> findFilePaths(std::string Dir, std::string Extension) {
  std::vector<std::string> Ret;
  std::vector<fs::path> PathList;
  std::set<std::string> VisitedPathSet;
  fs::path EntryPath = fs::path(Dir);
  if (fs::exists(EntryPath) && fs::is_directory(EntryPath)) {
    PathList.push_back(EntryPath);
  }
  while (!PathList.empty()) {
    fs::path Path = PathList.back();
    PathList.pop_back();
    std::string CurCanonicalPath = getCanonicalPath(Path);
    if (VisitedPathSet.find(CurCanonicalPath) != VisitedPathSet.end()) {
      continue;
    }
    VisitedPathSet.insert(CurCanonicalPath);
    for (auto &Iter : fs::directory_iterator(Path)) {
      auto InPath = Iter.path();
      if (fs::is_directory(InPath)) {
        PathList.push_back(InPath);
        continue;
      }

      if (InPath.extension().string() == Extension) {
        Ret.push_back(getCanonicalPath(InPath));
      }
    }
  }
  std::cout << "Found '*" << Extension << "' File Cnt: " << Ret.size() << "\n";
  return Ret;
}

std::vector<std::string> findASTFilePaths(std::string Dir) {
  return findFilePaths(Dir, ".ast");
}

std::vector<std::string> findJsonFilePaths(std::string Dir) {
  return findFilePaths(Dir, ".json");
}

bool saveFile(const char *filePath, const char *buffer) {
  std::ofstream outputFile(filePath);
  if (outputFile.is_open()) {
    outputFile << buffer;
    std::cout << "Saved File : " << filePath
              << ", Size : " << outputFile.tellp() << std::endl;
    return true;
  } else {
    std::cerr << "Failed Save File" << std::endl;
    return false;
  }
}

std::string readFile(const char *filePath) {
  std::ifstream inputFile(filePath);
  std::string fileContext;
  if (inputFile.is_open()) {
    inputFile.seekg(0, std::ios::end);
    int fileSize = inputFile.tellg();
    fileContext.resize(fileSize);
    inputFile.seekg(0, std::ios::beg);
    inputFile.read(&fileContext[0], fileSize);
    inputFile >> fileContext;

    std::cerr << "file opened: " << filePath << std::endl;
    return fileContext;
  } else {
    std::cerr << "Failed Open File: " << filePath << std::endl;
    return "";
  }
}

std::vector<std::string> readDirectory(const std::string &dirPath) {
  std::vector<std::string> fileList;

  if (!fs::is_directory(fs::status(dirPath))) {
    throw std::invalid_argument("Not Directory");
  }

  DIR *dirPtr = opendir(dirPath.c_str());
  assert(dirPtr && "Unexpected Program State");

  struct dirent *direntPtr;
  while ((direntPtr = readdir(dirPtr)) != nullptr) {
    std::string fileName = direntPtr->d_name;
    if (fileName == "." || fileName == "..") {
      continue;
    }

    std::string fullPath = dirPath + PATH_SEPARATOR + direntPtr->d_name;
    fileList.push_back(fullPath);
  }
  closedir(dirPtr);
  return fileList;
}

int makeDir(std::string path) {
  std::cout << "[COMMAND] mkdir " << path << '\n';
  return mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
}

void copy(std::string src, std::string dest, bool isRecursive) {
  std::string command = "cp ";
  command += (isRecursive ? "-r " : "") + src + " " + dest;
  std::cout << "[COMMAND] " << command << "\n";
  auto Ret = system(command.c_str());
  assert(Ret == 0 && "copy failed");
}

Json::Value parseJsonFileToJsonValue(const char *jsonPath) {
  std::string configDoc = readFile(jsonPath);
  Json::CharReaderBuilder rbuilder;
  std::istringstream input(configDoc);

  Json::Value root;
  std::string errs;
  Json::parseFromStream(rbuilder, input, &root, &errs);
  std::cout << "parsing json: " << errs << jsonPath << std::endl;
  return root;
}

std::string getNormalizedPath(std::string Path) {

  std::vector<fs::path> Tokens;

  fs::path FSPath(Path);
  for (auto S = FSPath.begin(), E = FSPath.end(); S != E; ++S) {
    if (*S == ".")
      continue;

    if (*S == ".." && Tokens.size() > 0) {
      Tokens.pop_back();
      continue;
    }

    Tokens.push_back(*S);
  }

  fs::path Result;
  for (auto &Token : Tokens)
    Result /= Token;

  return Result.string();
}

std::string getParentPath(std::string Path) {

  return fs::path(Path).parent_path().string();
}

std::string rebasePath(const std::string &Path, const std::string &OrgBase,
                       const std::string &NewBase) {
  auto LeafPath = getRelativePath(Path, OrgBase);
  assert(!LeafPath.empty() && "Invalid base path");
  return fs::path(NewBase) / LeafPath;
}

} // namespace util

} // namespace ftg
