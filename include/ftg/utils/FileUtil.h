#ifndef FTG_UTILS_FILEUTIL_H
#define FTG_UTILS_FILEUTIL_H

#include "json/json.h"
#include <string>

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR std::string("\\")
#else
#define PATH_SEPARATOR std::string("/")
#endif

namespace ftg {

#define REPORT_FILENAME std::string("fuzzGen_Report.json")
#define PROTO_HEADER_FILENAME std::string("FuzzArgsProfile.pb.h")
#define GENERATOR_FUZZ_ENTRY_FILENAME std::string("fuzz_entry.cc")

namespace util {

std::vector<std::string> findFilePaths(std::string Dir, std::string Extension);
std::vector<std::string> findASTFilePaths(std::string Dir);
std::vector<std::string> findJsonFilePaths(std::string Dir);

// Read a file and return the buffer of the file.
std::string readFile(const char *filePath);

// save a buffer as a file
bool saveFile(const char *filePath, const char *buffer);

int makeDir(std::string path);

void copy(std::string src, std::string dest, bool isRecursive);

std::vector<std::string> readDirectory(const std::string &name);

Json::Value parseJsonFileToJsonValue(const char *jsonPath);

std::string getNormalizedPath(std::string Path);
std::string getParentPath(std::string Path);
std::string rebasePath(const std::string &Path, const std::string &OrgBase,
                       const std::string &NewBase);

} // namespace util

} // namespace ftg

#endif // FTG_UTILS_FILEUTIL_H
