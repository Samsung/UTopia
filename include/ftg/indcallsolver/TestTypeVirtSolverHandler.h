#ifndef FTG_INDCALLSOLVER_TESTTYPEVIRTSOLVERHANDLER_H
#define FTG_INDCALLSOLVER_TESTTYPEVIRTSOLVERHANDLER_H

#include "ftg/indcallsolver/LLVMWalkHandler.h"
#include <set>
#include <map>

namespace ftg {

class TestTypeVirtSolverHandler : public LLVMWalkHandler<llvm::GlobalVariable> {
public:
  TestTypeVirtSolverHandler() = default;
  TestTypeVirtSolverHandler(TestTypeVirtSolverHandler &&Handler);
  void collect(const llvm::Module &M);
  std::set<const llvm::Function *> get(const llvm::Value *V) const;
  void handle(const llvm::GlobalVariable &GV) override;

private:
  struct TypeMetaInfo {
    // GlobalVariable what has type metadata
    const llvm::GlobalVariable *GV;
    // Offset at initializer of GlobalVariable
    uint64_t GVOffset;

    TypeMetaInfo(const llvm::GlobalVariable *GV, uint64_t Offset)
        : GV(GV), GVOffset(Offset) {}

    bool operator<(const TypeMetaInfo &Other) const {
      return GV < Other.GV || (GV == Other.GV && GVOffset < Other.GVOffset);
    }
  };
  const llvm::Function *TestTypeFunc = nullptr;
  std::map<const llvm::Metadata *, std::set<TypeMetaInfo>> TypeIDMap;
  std::map<const llvm::Value *, std::set<const llvm::Function *>> Map;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_TESTTYPEVIRTSOLVERHANDLER_H
