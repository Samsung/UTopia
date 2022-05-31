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

struct BCInfo {
  std::string BCCommand;
  std::string BCFile;
  std::string Directory;
  std::string OutCommand;
  std::string OutFile;
};

class ProjectEntry {
private:
  std::map<std::string, BCInfo> BinaryInfo;

public:
  std::string ProjectName;
  std::string ProjectDir;
  ProjectEntry(std::string ProjectName, std::string ProjectDir,
               std::map<std::string, BCInfo> BinaryInfo)
      : BinaryInfo(BinaryInfo), ProjectName(ProjectName),
        ProjectDir(ProjectDir) {}

  /// \param[in] BinaryName
  /// \return BCInfo of a binary if a binary is exist, else null
  const BCInfo *getBCInfo(std::string BinaryName) const;
  /// Loads ProjectEntry from json file.
  /// Does not guarantee all fields are filled.
  /// \param[in] JsonPath
  /// \return Loaded ProjectEntry if given file is exist and invalid, else null
  static std::unique_ptr<ProjectEntry> fromJson(std::string JsonPath);
};

struct CompileInfo {
  std::string ASTCommand;
  std::string ASTFile;
  std::string BCCommand;
  std::string BCFile;
  std::string Directory;
  std::string OutCommand;
  std::string OutFile;
  std::string SrcFile;
};

class CompileDB {
public:
  std::vector<CompileInfo> Compiles;
  /// Loads CompileDB from json file.
  /// Does not guarantee all fields are filled.
  /// \param[in] JsonPath
  /// \return Loaded CompileDB if given file is exist and valid, else null
  static std::unique_ptr<CompileDB> fromJson(std::string JsonPath);
  CompileDB(std::vector<CompileInfo> Compiles) : Compiles(Compiles) {}
};

} // namespace ftg

#endif // FTG_SOURCELOADER_BUILDDB_H
