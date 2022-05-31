#ifndef FTG_TCANALYSIS_TCTEXTRACTOR_H
#define FTG_TCANALYSIS_TCTEXTRACTOR_H

#include "ftg/sourceloader/SourceCollection.h"
#include "ftg/tcanalysis/TCExtractor.h"
#include "ftg/tcanalysis/UTInfo.h"

#include <vector>

namespace ftg {

class TCTExtractor : public TCExtractor {
public:
  TCTExtractor(const SourceCollection &SC);

private:
  std::vector<FunctionNode> getTestStepsFromUTFuncBody(const UTFuncBody &U);
  std::vector<Unittest> extractTCs() override;

  void setTCArray(clang::ASTContext &Context);
  void parseAST(clang::ASTContext &Context);

  Project TestProject;
};

} // namespace ftg

#endif // FTG_TCANALYSIS_TCTEXTRACTOR_H
