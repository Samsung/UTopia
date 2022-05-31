#include "ftg/astirmap/LocIndex.h"
#include "ftg/utils/FileUtil.h"

using namespace clang;

namespace ftg {

LocIndex::LocIndex() : Path(""), Line(0), Column(0) {}

LocIndex::LocIndex(std::string Path, size_t Line, size_t Column)
    : Path(util::getNormalizedPath(Path)), Line(Line), Column(Column) {}

LocIndex::LocIndex(const SourceManager &SrcManager, const SourceLocation &Loc) {
  SourceLocation ELoc = SrcManager.getExpansionLoc(Loc);
  Path = util::getNormalizedPath(SrcManager.getFilename(ELoc));
  Line = SrcManager.getExpansionLineNumber(ELoc);
  Column = SrcManager.getExpansionColumnNumber(ELoc);
}

std::string LocIndex::getPath() const { return Path; }

size_t LocIndex::getLine() const { return Line; }

size_t LocIndex::getColumn() const { return Column; }

std::string LocIndex::getIDAsString() const {
  return Path + ":" + std::to_string(Line) + ":" + std::to_string(Column);
}

bool LocIndex::operator<(const LocIndex &Src) const {
  if (Path != Src.getPath())
    return Path < Src.getPath();
  if (Line != Src.getLine())
    return Line < Src.getLine();
  return Column < Src.getColumn();
}

bool LocIndex::operator!=(const LocIndex &RHS) const {

  return Path != RHS.getPath() || Line != RHS.getLine() ||
         Column != RHS.getColumn();
}

bool LocIndex::operator==(const LocIndex &Rhs) const {
  return Path == Rhs.getPath() && Line == Rhs.getLine() &&
         Column == Rhs.getColumn();
}

} // namespace ftg
