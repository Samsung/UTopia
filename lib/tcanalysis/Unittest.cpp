#include "ftg/tcanalysis/Unittest.h"
#include "ftg/utils/FileUtil.h"
#include "ftg/utils/StringUtil.h"

#include "clang/AST/AST.h"
#include "clang/Basic/SourceManager.h"

using namespace ftg;

Unittest::Unittest(const clang::NamedDecl &Decl, std::string Type,
                   const std::vector<FunctionNode> &TestSequence)
    : Name(Decl.getNameAsString()), TestSequence(TestSequence), Type(Type) {
  const auto &SM = Decl.getASTContext().getSourceManager();
  auto DeclLocation = SM.getExpansionRange(Decl.getSourceRange()).getBegin();
  FilePath = util::getNormalizedPath(SM.getFilename(DeclLocation).str());
}

Unittest::Unittest(const Json::Value &Json) {
  assert(Json.isMember("APICalls") && Json.isMember("filepath") &&
         Json.isMember("name") && Json.isMember("Type") && "Unexpected Program State");

  for (auto &APICallsJson : Json["APICalls"])
    APICalls.emplace_back(APICallsJson);
  FilePath = Json["filepath"].asString();
  Name = Json["name"].asString();
  for (Json::Value TestFuncJson : Json["TestSequence"])
    TestSequence.emplace_back(TestFuncJson);
  Type = Json["Type"].asString();
  assert(!Type.empty() && "Unexpected Program State");
}

const std::vector<APICall> &Unittest::getAPICalls() const { return APICalls; }

std::set<std::string> Unittest::getEnvironmentClasses() const {
  std::set<std::string> EnvClasses;
  for (const auto &TestFunc : TestSequence) {
    if (TestFunc.isEnvironment() && !TestFunc.getClassName().empty())
      EnvClasses.insert(TestFunc.getClassName());
  }
  return EnvClasses;
}

const std::vector<FunctionNode> &Unittest::getTestSequence() const {
  return TestSequence;
}

std::string Unittest::getFilePath() const { return FilePath; }

std::string Unittest::getID() const {
  if (Type == "gtest") {
    std::vector<std::string> Tokens = util::split(Name, "::");
    return Tokens[0];
  }
  return Name;
}

const std::vector<llvm::Function *> Unittest::getLinks() const {
  std::vector<llvm::Function *> Links;
  for (const auto &TestFunc : TestSequence)
    Links.push_back(const_cast<llvm::Function *>(TestFunc.getIRFunction()));
  return Links;
}

std::string Unittest::getName() const { return Name; }

void Unittest::setAPICalls(const std::vector<APICall> &APICalls) {
  this->APICalls = APICalls;
}

const FunctionNode *Unittest::getTestBody() const {
  for (const auto &TestFunc : TestSequence) {
    if (TestFunc.isTestBody())
      return &TestFunc;
  }
  return nullptr;
}

std::string Unittest::getType() const { return Type; }

Json::Value Unittest::getJson() const {
  Json::Value Root;
  Root["name"] = Name;
  Root["filepath"] = FilePath;
  Root["Type"] = Type;

  Json::Value TestSequenceJson;
  for (const auto &Action : TestSequence) {
    TestSequenceJson.append(Action.getJson());
  }
  Root["TestSequence"] = TestSequenceJson;

  Json::Value APICallJsonArray = Json::Value(Json::arrayValue);
  for (auto &Call : APICalls)
    APICallJsonArray.append(Call.toJson());

  Root["APICalls"] = APICallJsonArray;
  return Root;
}
