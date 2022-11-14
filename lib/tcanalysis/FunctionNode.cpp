#include "ftg/tcanalysis/FunctionNode.h"
#include "ftg/utils/ASTUtil.h"

using namespace ftg;

FunctionNode::FunctionNode(const llvm::Function &Func, bool IsTestBody,
                           bool Environment)
    : IRFunc(&Func), TestBody(IsTestBody), Environment(Environment) {
  Name = util::getDemangledName(Func.getName().str());
  ClassName = util::getClassNameWithNamespace(Name);
  FunctionName = util::getMethodName(Name);
}

FunctionNode::FunctionNode(Json::Value JsonValue) {
  assert(JsonValue.isMember("FullName") && JsonValue.isMember("ClassName") &&
         JsonValue.isMember("FunctionName") &&
         JsonValue.isMember("isTestBody") &&
         JsonValue.isMember("isEnvironment") && "Unexpected Program State");

  Name = JsonValue["FullName"].asString();
  ClassName = JsonValue["ClassName"].asString();
  FunctionName = JsonValue["FunctionName"].asString();
  TestBody = JsonValue["isTestBody"].asBool();
  Environment = JsonValue["isEnvironment"].asBool();
}

std::string FunctionNode::getClassName() const { return ClassName; }

std::string FunctionNode::getFunctionName() const { return FunctionName; }

const llvm::Function *FunctionNode::getIRFunction() const { return IRFunc; }

std::string FunctionNode::getName() const { return Name; }

bool FunctionNode::isEnvironment() const { return Environment; }

bool FunctionNode::isTestBody() const { return TestBody; }

Json::Value FunctionNode::getJson() const {
  Json::Value Root;
  Root["FullName"] = Name;
  Root["ClassName"] = ClassName;
  Root["FunctionName"] = FunctionName;
  Root["isTestBody"] = TestBody;
  Root["isEnvironment"] = Environment;
  return Root;
}
