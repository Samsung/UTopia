#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/utils/ASTUtil.h"

namespace ftg {

TargetLib::TargetLib() : M(nullptr) {}

TargetLib::TargetLib(const llvm::Module &M, std::set<std::string> APIs)
    : M(&M), APIs(APIs) {}

const std::set<std::string> TargetLib::getAPIs() const { return APIs; }

const std::map<std::string, std::shared_ptr<Enum>> &
TargetLib::getEnumMap() const {
  return EnumMap;
}
const std::map<std::string, std::string> &TargetLib::getTypedefMap() const {
  return TypedefMap;
}

Enum *TargetLib::getEnum(std::string name) {
  size_t enumExprOffset = name.find("enum ");
  if (enumExprOffset != std::string::npos) {
    name.replace(enumExprOffset, 5, "");
    name = util::trim(name);
  }

  auto ret = EnumMap.find(name);
  if (ret == EnumMap.end()) {
    auto retFromTypedef = TypedefMap.find(name);
    if (retFromTypedef == TypedefMap.end()) {
      return nullptr;
    }
    ret = EnumMap.find(retFromTypedef->second);
  }
  return (ret == EnumMap.end()) ? nullptr : ret->second.get();
}

const llvm::Module *TargetLib::getLLVMModule() const { return M; }

void TargetLib::addEnum(std::shared_ptr<Enum> item) {
  EnumMap.insert(std::make_pair(item->getName(), item));
}

void TargetLib::addTypedef(std::pair<std::string, std::string> item) {
  TypedefMap.insert(std::make_pair(item.first, item.second));
}

}; // namespace ftg
