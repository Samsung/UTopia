#include "ftg/utils/ManualAllocLoader.h"
#include "ftg/utils/StringUtil.h"

using namespace ftg;

bool ManualAllocLoader::isAllocFunction(const llvm::Function &F) const {
  auto AllocSizeArgs = getAllocSizeArgNo(F);
  return !AllocSizeArgs.empty();
}

std::set<unsigned>
ManualAllocLoader::getAllocSizeArgNo(const llvm::Function &F) const {
  auto FuncName = F.getName();
  return getAllocSizeArgNo(FuncName);
}

std::set<unsigned>
ManualAllocLoader::getAllocSizeArgNo(std::string Name) const {
  Name = util::getDemangledName(Name);
  for (auto FuncIter : FuncList) {
    if (Name.find(FuncIter.Name) == std::string::npos)
      continue;
    if (FuncIter.Name != "::basic_string(")
      return FuncIter.AllocSizeArgNo;

    for (auto StringIter : StringList) {
      if (Name.find(StringIter.Name) == std::string::npos)
        continue;
      return StringIter.AllocSizeArgNo;
    }
  }
  return {};
}
