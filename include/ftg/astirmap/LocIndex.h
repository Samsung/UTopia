#ifndef FTG_ASTIRMAP_LOCINDEX_H
#define FTG_ASTIRMAP_LOCINDEX_H

#include "clang/Basic/SourceManager.h"
#include <string>

namespace ftg {

class LocIndex {
public:
  LocIndex();
  LocIndex(std::string Path, size_t Line, size_t Column);
  LocIndex(const clang::SourceManager &, const clang::SourceLocation &);

  std::string getPath() const;
  size_t getLine() const;
  size_t getColumn() const;
  std::string getIDAsString() const;
  bool operator<(const LocIndex &) const;
  bool operator!=(const LocIndex &) const;
  bool operator==(const LocIndex &Index) const;

protected:
  std::string Path;
  size_t Line;
  size_t Column;
};

} // namespace ftg

#endif // FTG_ASTIRMAP_LOCINDEX_H
