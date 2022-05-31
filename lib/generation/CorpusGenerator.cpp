#include "ftg/generation/CorpusGenerator.h"
#include "ftg/constantanalysis/ASTValue.h"
#include "ftg/utils/AssignUtil.h"

namespace ftg {

std::string CorpusGenerator::generate(const Fuzzer &F) const {

  std::string Result;
  for (auto Iter : F.getFuzzInputMap()) {
    assert(Iter.second && "Unexpected Program State");

    auto &Def = Iter.second->getDef();
    if (Def.ArrayLen)
      continue;

    auto CorpusValue = generate(Def);
    if (CorpusValue.empty())
      continue;

    Result += CorpusValue + "\n";
  }

  return Result;
}

std::string CorpusGenerator::generate(const Definition &Def) const {

  std::string Result;

  auto CorpusValue = generate(Def.Value, Def.DataType.get());
  if (CorpusValue.empty())
    return Result;

  return util::getProtoVarName(Def.ID) + ": " + CorpusValue;
}

std::string CorpusGenerator::generate(const ASTValue &Value,
                                      const Type *T) const {

  std::string Result;

  if (Value.getValues().size() == 0) {
    return Result;
  }

  if (Value.isArray()) {
    if (T && isByteType(*T)) {
      for (auto &Value : Value.getValues()) {
        auto G = generate(Value, T);
        if (Value.Type == ASTValueData::VType_INT) {
          if (G.size() != 1)
            return "";
          if (G[0] < '0' || G[0] > '9')
            return "";
          Result += "\\" + G.substr(0, 1);
        } else {
          Result += generate(Value, T);
        }
      }
      Result = "\"" + Result + "\"";
    } else {
      Result += "[ ";

      for (int S = 0; S < (int)Value.getValues().size(); ++S) {
        Result += generate(Value.getValues()[S], T);
        if (S != (int)Value.getValues().size() - 1) {
          Result += ", ";
        }
      }
      Result += "]";
    }
    return Result;
  }

  return generate(Value.getValues()[0], T);
}

std::string CorpusGenerator::generate(const ASTValueData &Data,
                                      const Type *T) const {

  std::string Result = Data.Value;

  if (Data.Type == ASTValueData::VType_STRING) {
    Result = "\"" + Result + "\"";
  } else if (auto *ET =
                 const_cast<EnumType *>(llvm::dyn_cast_or_null<EnumType>(T))) {
    if (auto *GD = ET->getGlobalDef()) {
      auto &Elems = GD->getElements();
      for (unsigned S = 0; S < Elems.size(); ++S) {
        if (!Elems[S]) {
          continue;
        }

        if (std::to_string(Elems[S]->getType()) == Result) {
          Result = std::to_string(S);
          break;
        }
      }
    }
  }

  return Result;
}

bool CorpusGenerator::isByteType(const Type &T) const {
  const auto *PT = &T;
  do {
    PT = &(PT->getPointeeType());
  } while (PT != &(PT->getPointeeType()));

  if (&T == PT)
    return false;

  auto *IT = llvm::dyn_cast_or_null<IntegerType>(PT);
  if (!IT)
    return false;

  return IT->isAnyCharacter();
}

} // namespace ftg
