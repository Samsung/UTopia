//===-- BuildDB.h - data classes that represents build DB -------*- C++ -*-===//
///
/// \file
/// Defines data classes for build DB
///
//===----------------------------------------------------------------------===//

#ifndef FTG_SOURCELOADER_BUILDDB_H
#define FTG_SOURCELOADER_BUILDDB_H

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ftg {

class BuildDB {
public:
  BuildDB(std::string BCFile, std::vector<std::string> ASTFiles,
          std::string ProjectDir);

  /// Path of linked BC
  std::string getBCPath() const;
  /// Paths of AST files
  std::vector<std::string> getASTPaths() const;
  /// Path of build root directory
  std::string getProjectDir() const;
  static std::unique_ptr<BuildDB> fromJson(const std::string &Path);

private:
  std::string BCPath;
  std::vector<std::string> ASTPaths;
  std::string ProjectDir;
};

} // namespace ftg

#endif // FTG_SOURCELOADER_BUILDDB_H
