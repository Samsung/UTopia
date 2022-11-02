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

#include <memory>
#include <string>

namespace ftg {

/// Implement of ftg::sourceloader that uses saved build db.
class BuildDBLoader : public SourceLoader {
public:
  /// \param BuildDBPath Path of BuildDB file.
  BuildDBLoader(std::string BuildDBPath);
  std::unique_ptr<SourceCollection> load() override;

private:
  std::string BuildDBPath;
};

} // namespace ftg

#endif // FTG_SOURCELOADER_BUILDDBLOADER_H
