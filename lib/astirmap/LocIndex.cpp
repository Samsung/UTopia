#include "ftg/astirmap/LocIndex.h"
#include "ftg/utils/FileUtil.h"
#include "ftg/utils/LLVMUtil.h"
#include "llvm/IR/DebugLoc.h"

using namespace clang;

namespace ftg {

LocIndex::LocIndex() : Path(""), Line(0), Column(0) {}

LocIndex::LocIndex(std::string Path, size_t Line, size_t Column)
    : Path(util::getNormalizedPath(Path)), Line(Line), Column(Column) {}

LocIndex::LocIndex(const SourceManager &SrcManager, const SourceLocation &Loc) {
  SourceLocation ELoc = SrcManager.getExpansionLoc(Loc);
  Path = util::getNormalizedPath(SrcManager.getFilename(ELoc).str());
  Line = SrcManager.getExpansionLineNumber(ELoc);
  Column = SrcManager.getExpansionColumnNumber(ELoc);
}

LocIndex LocIndex::of(const llvm::AllocaInst &AI) {
  const auto *F = AI.getFunction();
  assert(F && "Unexpected Program State");

  for (auto &IterB : *F)
    for (auto &IterI : IterB) {
      const auto *CB = llvm::dyn_cast<llvm::CallBase>(&IterI);
      if (!CB)
        continue;

      const auto *CF = CB->getCalledFunction();
      if (!CF || !CF->isIntrinsic() || CF->getName() != "llvm.dbg.declare")
        continue;

      assert(CB->getNumArgOperands() > 0 && "Unexpected Program State");

      const auto *M1 =
          llvm::dyn_cast_or_null<llvm::MetadataAsValue>(CB->getArgOperand(0));
      if (!M1)
        continue;

      const auto *M2 =
          llvm::dyn_cast_or_null<llvm::LocalAsMetadata>(M1->getMetadata());
      if (!M2 || M2->getValue() != &AI)
        continue;

      const auto &Loc = CB->getDebugLoc();
      if (Loc.get())
        return LocIndex(getFullPath(CB->getDebugLoc()), Loc.getLine(),
                        Loc.getCol());
    }
  throw std::runtime_error("DebugLoc Not Found");
}

LocIndex LocIndex::of(const llvm::Instruction &I) {
  if (auto *AI = llvm::dyn_cast<llvm::AllocaInst>(&I))
    return LocIndex::of(*AI);

  const auto &Loc = I.getDebugLoc();
  if (!Loc)
    throw std::runtime_error("DebugLoc Not Found");
  return LocIndex(getFullPath(Loc), Loc.getLine(), Loc.getCol());
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

std::string LocIndex::getFullPath(const llvm::DebugLoc &Loc) {
  auto *DILoc = Loc.get();
  if (!DILoc)
    return "";

  auto *DIF = DILoc->getFile();
  if (!DIF)
    return "";

  return util::getFullPath(*DIF);
}

} // namespace ftg
