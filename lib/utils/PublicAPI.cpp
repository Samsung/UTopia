#include "ftg/utils/PublicAPI.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/FileUtil.h"
#include "ftg/utils/StringUtil.h"
#include "ftg/utils/json/json.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "llvm/Support/raw_os_ostream.h"
#include <assert.h>

using namespace clang;
using namespace ast_matchers;

namespace ftg {

bool PublicAPI::isPublicAPI(std::string funcName) const {
  return PublicAPIs.find(funcName) != PublicAPIs.end();
}

bool PublicAPI::isDeprecated(std::string funcName) const {
  return DeprecatedAPIs.find(funcName) != DeprecatedAPIs.end();
}

const std::set<std::string> &PublicAPI::getPublicAPIList() const {
  return PublicAPIs;
}

size_t PublicAPI::getPublicAPISize() const { return PublicAPIs.size(); }

void PublicAPI::loadJson(std::string jsonFilePath) {
  try {
    // 1. Parse PublicAPI.json
    Json::Value root = util::parseJsonFileToJsonValue(jsonFilePath.c_str());

    // 2. Add PublicAPIs and DeprecatedAPIs from PublicAPI.json
    for (std::string &libName : root.getMemberNames()) {
      if (util::regex(libName, ".+_deprecated").empty()) {
        // Note: There can be DeprecatedAPIs in PublicAPIs.
        for (Json::Value &funcName : root[libName]) {
          PublicAPIs.insert(funcName.asString());
        }
      } else {
        // Add DeprecatedAPIs if libName is "*_deprecated".
        for (Json::Value &funcName : root[libName]) {
          DeprecatedAPIs.insert(funcName.asString());
        }
      }
    }
  } catch (Json::Exception &E) {
    llvm::errs() << "[ERROR] Json Exception : " << E.what() << '\n';
  }
}

void PublicAPI::loadExportsNDepsJson(std::string JsonPath,
                                     std::string LibName) {
  try {
    Json::Value Root = util::parseJsonFileToJsonValue(JsonPath.c_str());
    assert(Root.isMember("exported_funcs") &&
           Root["exported_funcs"].isMember(LibName) &&
           "Json Format is invalid");
    for (Json::Value FuncName : Root["exported_funcs"][LibName]) {
      PublicAPIs.insert(FuncName.asString());
    }
  } catch (Json::Exception &E) {
    llvm::errs() << "[ERROR] Json Exception : " << E.what() << '\n';
  }
}

// TODO: Unify the method in TargetAnalysis and UTAnalysis
class FuncMatchCallback : public MatchFinder::MatchCallback {
public:
  FuncMatchCallback(const PublicAPI &MyPublicAPI,
                    std::set<clang::FunctionDecl *> &ClangPublicAPIs,
                    std::string MatchTag)
      : MyPublicAPI(MyPublicAPI), ClangPublicAPIs(ClangPublicAPIs),
        MatchTag(MatchTag) {}

  void run(const MatchFinder::MatchResult &Result) override {
    FunctionDecl *FD = const_cast<FunctionDecl *>(
        Result.Nodes.getNodeAs<FunctionDecl>(MatchTag));
    if (MyPublicAPI.getPublicAPISize()) {
      if (MyPublicAPI.isPublicAPI(util::getMangledName(FD))) {
        ClangPublicAPIs.insert(FD);
      }
    } else if (FD->hasExternalFormalLinkage()) {
      ClangPublicAPIs.insert(FD);
    }
  }

private:
  const PublicAPI &MyPublicAPI;
  std::set<clang::FunctionDecl *> &ClangPublicAPIs;
  std::string MatchTag;
};

const std::set<clang::FunctionDecl *>
PublicAPI::findClangFunctionDecls(clang::ASTContext &Ctx) const {
  std::set<clang::FunctionDecl *> ClangPublicAPIs;
  std::string MatchTag = "FunctionTag";
  DeclarationMatcher FunctionMatcher =
      functionDecl(isDefinition()).bind(MatchTag);

  MatchFinder MatchFinder;
  FuncMatchCallback MatchCallback(*this, ClangPublicAPIs, MatchTag);
  MatchFinder.addMatcher(FunctionMatcher, &MatchCallback);
  MatchFinder.matchAST(Ctx);
  return ClangPublicAPIs;
}

} // namespace ftg
