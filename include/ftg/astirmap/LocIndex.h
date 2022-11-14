#ifndef FTG_ASTIRMAP_LOCINDEX_H
#define FTG_ASTIRMAP_LOCINDEX_H

#include "clang/Basic/SourceManager.h"
#include "llvm/IR/Instructions.h"
#include <string>

namespace ftg {

class LocIndex {
public:
  LocIndex();
  LocIndex(std::string Path, size_t Line, size_t Column,
           bool FromMacro = false);
  LocIndex(const clang::SourceManager &, const clang::SourceLocation &);

  static LocIndex of(const llvm::AllocaInst &AI);
  static LocIndex of(const llvm::Instruction &I);

  std::string getPath() const;
  size_t getLine() const;
  size_t getColumn() const;
  std::string getIDAsString() const;
  bool isExpandedFromMacro() const;

  bool operator<(const LocIndex &) const;
  bool operator!=(const LocIndex &) const;
  bool operator==(const LocIndex &Index) const;

private:
  std::string Path;
  size_t Line;
  size_t Column;
  bool FromMacro;

  static std::string getFullPath(const llvm::DebugLoc &Loc);
};

} // namespace ftg

#endif // FTG_ASTIRMAP_LOCINDEX_H
