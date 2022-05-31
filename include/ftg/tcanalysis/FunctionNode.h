#ifndef FTG_TCANALYSIS_FUNCTIONNODE_H
#define FTG_TCANALYSIS_FUNCTIONNODE_H

#include "ftg/utils/json/json.h"
#include "llvm/IR/Function.h"

namespace ftg {
/**
 * @brief File of NamedDecl
 * @details
 */
class FunctionNode {
public:
  FunctionNode(const llvm::Function &Func, bool IsTestBody,
               bool Environment = false);
  FunctionNode(Json::Value JsonValue);

  std::string getClassName() const;
  std::string getFunctionName() const;
  const llvm::Function *getIRFunction() const;
  std::string getName() const;
  bool isEnvironment() const;
  bool isTestBody() const;
  Json::Value getJson() const;

private:
  const llvm::Function *IRFunc = nullptr;
  std::string Name;
  std::string ClassName;
  std::string FunctionName;
  bool TestBody;
  bool Environment;
};
} // namespace ftg

#endif // FTG_TCANALYSIS_FUNCTIONNODE_H
