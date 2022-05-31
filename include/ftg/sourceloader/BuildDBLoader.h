//===-- BuildDBLoader.h - source loader from build DB -----------*- C++ -*-===//
///
/// \file
/// Defines source loader that utilize build DB
///
//===----------------------------------------------------------------------===//

#ifndef FTG_SOURCELOADER_BUILDDBLOADER_H
#define FTG_SOURCELOADER_BUILDDBLOADER_H

#include "ftg/sourceloader/BuildDB.h"
#include "ftg/sourceloader/SourceCollection.h"
#include "ftg/sourceloader/SourceLoader.h"
#include <string>

namespace ftg {

/// Implement of ftg::sourceloader that uses saved build db.
class BuildDBLoader : public SourceLoader {
public:
  /// \param BuildDBPath Path of BuildDB directory.
  /// The directory should contain project_entry.json and compiles.json.
  /// \param BinaryName Target binary name to load build info.
  BuildDBLoader(std::string BuildDBPath, std::string BinaryName);
  std::unique_ptr<SourceCollection> load() override;

private:
  std::string BinaryName;
  std::unique_ptr<ProjectEntry> PE;
  std::unique_ptr<CompileDB> DB;
  /// load data from BuildDBPath
  void loadBuildDB(std::string BuildDBPath);
  /// get path of BC file from project_entry.json
  std::string getBCPath();
  /// get paths of AST files from compiles.json
  std::vector<std::string> getASTPaths();
};

} // namespace ftg

#endif // FTG_SOURCELOADER_BUILDDBLOADER_H
