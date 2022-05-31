#ifndef FTG_TCANALYSIS_UNITTEST_H
#define FTG_TCANALYSIS_UNITTEST_H

#include "ftg/tcanalysis/APICall.h"
#include "ftg/tcanalysis/FunctionNode.h"
#include "ftg/utanalysis/Location.h"

#include "clang/AST/Decl.h"
#include "llvm/IR/Function.h"

namespace ftg {

/**
 * @brief File of unittest
 * @details
 */
class Unittest {
public:
  Unittest(const clang::NamedDecl &Decl, Location Loc, std::string Type,
           const std::vector<FunctionNode> &TestSequence);
  Unittest(const clang::NamedDecl &Decl, std::string Type,
           const std::vector<FunctionNode> &TestSequence);
  Unittest(const Json::Value &Json);

  const std::vector<APICall> &getAPICalls() const;
  std::set<std::string> getEnvironmentClasses() const;
  const std::vector<FunctionNode> &getTestSequence() const;
  std::string getFilePath() const;
  std::string getID() const;
  const std::vector<llvm::Function *> getLinks() const;
  std::string getName() const;
  void setAPICalls(const std::vector<APICall> &APICalls);
  const FunctionNode *getTestBody() const;
  std::string getType() const;
  Json::Value getJson() const;

private:
  std::vector<APICall> APICalls;
  std::string FilePath;
  std::string Name;
  size_t Offset;
  std::vector<FunctionNode> TestSequence;
  std::string Type;
};
} // namespace ftg

#endif // FTG_TCANALYSIS_UNITTEST_H
