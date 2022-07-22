#ifndef FTG_TARGETANALYSIS_TARGETLIB_H
#define FTG_TARGETANALYSIS_TARGETLIB_H

#include "ftg/type/GlobalDef.h"
#include "clang/AST/Type.h"
#include "llvm/IR/Module.h"
#include <set>

namespace ftg {

class TargetLib {
public:
  TargetLib();
  TargetLib(const llvm::Module &M, std::set<std::string> APIs);
  const std::set<std::string> getAPIs() const;
  const std::map<std::string, std::shared_ptr<Enum>> &getEnumMap() const;
  const std::map<std::string, std::string> &getTypedefMap() const;
  Enum *getEnum(std::string name);
  const llvm::Module *getLLVMModule() const;
  void addEnum(std::shared_ptr<Enum> item);
  void addTypedef(std::pair<std::string, std::string> item);

private:
  const llvm::Module *M;
  std::set<std::string> APIs;
  std::map<std::string, std::shared_ptr<Enum>> EnumMap;
  std::map<std::string, std::string> TypedefMap;
};

} // namespace ftg

#endif // FTG_TARGETANALYSIS_TARGETLIB_H
