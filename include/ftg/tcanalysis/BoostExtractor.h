#ifndef FTG_TCANALYSIS_BOOSTEXTRACTOR_H
#define FTG_TCANALYSIS_BOOSTEXTRACTOR_H

#include "ftg/sourceloader/SourceCollection.h"
#include "ftg/tcanalysis/TCExtractor.h"
#include "clang/Frontend/ASTUnit.h"

#include <string>
#include <vector>

namespace ftg {

class BoostExtractor : public TCExtractor {
public:
  BoostExtractor(const SourceCollection &SC);

private:
  const std::vector<Unittest> getTestcasesFromASTUnit(const llvm::Module &M,
                                                      clang::ASTUnit &U) const;
  const std::set<const clang::CXXRecordDecl *>
  getTestRecords(clang::ASTContext &Ctx) const;
  const clang::FunctionDecl *getTestBody(const clang::CXXRecordDecl &D,
                                         const clang::ASTContext &Ctx) const;
  std::vector<Unittest> extractTCs() override;

  const std::string TestBodyName = "test_method";
};
} // namespace ftg

#endif // FTG_TCANALYSIS_BOOSTEXTRACTOR_H
