#ifndef FTG_ASTIRMAP_CALLEDFUNCTIONMACROMAPPER_H
#define FTG_ASTIRMAP_CALLEDFUNCTIONMACROMAPPER_H

#include "MacroMapper.h"
#include <set>

namespace ftg {

class CalledFunctionMacroMapper : public MacroMapper {
public:
  CalledFunctionMacroMapper() = default;
  void insertMacroNode(clang::Stmt &E, clang::ASTUnit &Unit) override;
  ASTNode *getASTNode(llvm::CallBase &CB) override;
  ASTDefNode *getASTDefNode(llvm::Instruction &I) override;

private:
  struct DBKey {
    std::string FilePath;
    unsigned Line;
    unsigned Column;
    std::string FunctionName;

    DBKey(std::string FilePath, unsigned Line, unsigned Column,
          std::string FunctionName);
    bool operator<(const DBKey &RHS) const;
  };

  std::map<DBKey, std::unique_ptr<ASTDefNode>> DefMap;
  std::map<DBKey, std::unique_ptr<ASTNode>> CallMap;
  std::set<DBKey> DupKey;
};

} // namespace ftg

#endif // FTG_ASTIRMAP_CALLEDFUNCTIONMACROMAPPER_H
