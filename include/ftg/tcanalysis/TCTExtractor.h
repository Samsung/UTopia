#ifndef FTG_TCANALYSIS_TCTEXTRACTOR_H
#define FTG_TCANALYSIS_TCTEXTRACTOR_H

#include "ftg/sourceloader/SourceCollection.h"
#include "ftg/tcanalysis/TCExtractor.h"

#include <vector>

namespace ftg {

class TCTExtractor : public TCExtractor {
public:
  TCTExtractor(const SourceCollection &SC);

private:
  std::vector<Unittest> extractTCs() override;
  std::vector<Unittest> extractTCs(const clang::ASTContext &Ctx) const;
  const llvm::Function *getFunc(const clang::FunctionDecl *D) const;
  const clang::FunctionDecl *getDefinedFuncDecl(const clang::Expr *E) const;
  const clang::VarDecl *getTCArray(const clang::ASTContext &Ctx) const;
  std::vector<Unittest> parseTCArray(const clang::VarDecl &D) const;
  std::unique_ptr<Unittest>
  parseTCArrayElement(const clang::InitListExpr &E) const;
};

} // namespace ftg

#endif // FTG_TCANALYSIS_TCTEXTRACTOR_H
