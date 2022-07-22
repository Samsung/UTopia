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

  assert(Def.DataType && "Unexpected Program State");
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
  } else if (T->getKind() == Type::TypeID_Enum) {
    if (auto *GD = T->getGlobalDef()) {
      auto &Elems = GD->getElements();
      for (unsigned S = 0; S < Elems.size(); ++S) {
        if (std::to_string(Elems[S].getValue()) == Result) {
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
    PT = PT->getPointeeType();
  } while (PT && PT != PT->getPointeeType());

  if (!PT || &T == PT)
    return false;

  if (!PT->isIntegerType())
    return false;

  return PT->isAnyCharacter();
}

} // namespace ftg
