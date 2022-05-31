#ifndef FTG_GENERATION_CORPUSGENERATOR_H
#define FTG_GENERATION_CORPUSGENERATOR_H

#include "ftg/generation/Fuzzer.h"

namespace ftg {

class CorpusGenerator {

public:
  CorpusGenerator() = default;
  std::string generate(const Fuzzer &F) const;

private:
  std::string generate(const Definition &Def) const;
  std::string generate(const ASTValue &Value, const Type *T = nullptr) const;
  std::string generate(const ASTValueData &Data, const Type *T = nullptr) const;

  bool isByteType(const Type &T) const;
};

} // namespace ftg

#endif // FTG_GENERATION_CORPUSGENERATOR_H
