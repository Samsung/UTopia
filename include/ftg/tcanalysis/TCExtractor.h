#ifndef FTG_TCANALYSIS_TCEXTRACTOR_H
#define FTG_TCANALYSIS_TCEXTRACTOR_H

#include "ftg/sourceloader/SourceCollection.h"
#include "ftg/tcanalysis/Unittest.h"

#include "clang/AST/Decl.h"
#include "clang/Frontend/ASTUnit.h"

#include <string>
#include <vector>

namespace ftg {

class TCExtractor {
public:
  virtual ~TCExtractor() = default;
  TCExtractor(const SourceCollection &SC) : SrcCollection(SC){};

  const std::vector<std::string> getTCNames() const;
  const Unittest *getTC(std::string TCName) const;

  void load();

protected:
  virtual std::vector<Unittest> extractTCs() = 0;

  std::map<std::string, Unittest> TCs;
  const SourceCollection &SrcCollection;
};

} // namespace ftg

#endif // FTG_TCANALYSIS_TCEXTRACTOR_H
