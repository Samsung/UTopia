#ifndef FTG_TARGETANALYSIS_TARGETLIB_H
#define FTG_TARGETANALYSIS_TARGETLIB_H

#include "ftg/targetanalysis/FunctionReport.h"
#include "ftg/type/GlobalDef.h"
#include "clang/AST/Type.h"
#include "llvm/IR/Module.h"

namespace ftg {

class TargetLib {
public:
  TargetLib();
  TargetLib(const llvm::Module &M, std::set<std::string> APIs);
  const std::set<std::string> getAPIs() const;
  const std::map<std::string, std::shared_ptr<Function>> &
  getFunctionMap() const;
  const std::map<std::string, std::shared_ptr<FunctionReport>> &
  getFunctionReportMap() const;
  const std::map<std::string, std::shared_ptr<Enum>> &getEnumMap() const;
  const std::map<std::string, std::shared_ptr<Struct>> &getStructMap() const;
  const std::map<std::string, std::string> &getTypedefMap() const;
  Function *getFunction(std::string FunctionName) const;
  const FunctionReport *getFunctionReport(std::string FunctionName) const;
  Struct *getStruct(std::string name);
  Enum *getEnum(std::string name);
  const llvm::Module *getLLVMModule() const;
  void addFunction(std::shared_ptr<Function> item);
  void addFunctionReport(std::string FunctionName,
                         std::shared_ptr<FunctionReport> Report);
  void addStruct(std::shared_ptr<Struct> item);
  void addEnum(std::shared_ptr<Enum> item);
  void addTypedef(std::pair<std::string, std::string> item);

private:
  const llvm::Module *M;
  std::set<std::string> APIs;
  std::map<std::string, std::shared_ptr<Function>> FunctionMap;
  std::map<std::string, std::shared_ptr<FunctionReport>> FunctionReportMap;
  std::map<std::string, std::shared_ptr<Struct>> StructMap;
  std::map<std::string, std::shared_ptr<Enum>> EnumMap;
  std::map<std::string, std::string> TypedefMap;
};

} // namespace ftg

#endif // FTG_TARGETANALYSIS_TARGETLIB_H
