#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/utils/ASTUtil.h"

namespace ftg {

TargetLib::TargetLib() : M(nullptr) {}

TargetLib::TargetLib(const llvm::Module &M, std::set<std::string> APIs)
    : M(&M), APIs(APIs) {}

const std::set<std::string> TargetLib::getAPIs() const { return APIs; }

const std::map<std::string, std::shared_ptr<Function>> &
TargetLib::getFunctionMap() const {
  return FunctionMap;
}

const std::map<std::string, std::shared_ptr<FunctionReport>> &
TargetLib::getFunctionReportMap() const {
  return FunctionReportMap;
}

const std::map<std::string, std::shared_ptr<Enum>> &
TargetLib::getEnumMap() const {
  return EnumMap;
}

const std::map<std::string, std::shared_ptr<Struct>> &
TargetLib::getStructMap() const {
  return StructMap;
}

const std::map<std::string, std::string> &TargetLib::getTypedefMap() const {
  return TypedefMap;
}

Function *TargetLib::getFunction(std::string name) const {
  auto ret = this->FunctionMap.find(name);
  return (ret == this->FunctionMap.end()) ? nullptr : ret->second.get();
}

const FunctionReport *
TargetLib::getFunctionReport(std::string FunctionName) const {
  auto Ret = FunctionReportMap.find(FunctionName);
  return (Ret == FunctionReportMap.end()) ? nullptr : Ret->second.get();
}

Struct *TargetLib::getStruct(std::string name) {
  auto ret = StructMap.find(name);
  if (ret == StructMap.end()) {
    auto retFromTypedef = TypedefMap.find(name);
    if (retFromTypedef == TypedefMap.end()) {
      return nullptr;
    }
    ret = StructMap.find(retFromTypedef->second);
  }
  return (ret == StructMap.end()) ? nullptr : ret->second.get();
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

void TargetLib::addFunction(std::shared_ptr<Function> item) {
  FunctionMap.insert(std::make_pair(item->getName(), item));
}

void TargetLib::addFunctionReport(std::string FunctionName,
                                  std::shared_ptr<FunctionReport> Report) {
  FunctionReportMap.insert(std::make_pair(FunctionName, Report));
}

void TargetLib::addStruct(std::shared_ptr<Struct> item) {
  StructMap.insert(std::make_pair(item->getName(), item));
}

void TargetLib::addEnum(std::shared_ptr<Enum> item) {
  EnumMap.insert(std::make_pair(item->getName(), item));
}

void TargetLib::addTypedef(std::pair<std::string, std::string> item) {
  TypedefMap.insert(std::make_pair(item.first, item.second));
}

}; // namespace ftg
