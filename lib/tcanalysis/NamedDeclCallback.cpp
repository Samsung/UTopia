#include "ftg/tcanalysis/NamedDeclCallback.h"

using namespace ftg;
using namespace clang;
using namespace ast_matchers;

void NamedDeclCallback::run(const MatchFinder::MatchResult &Result) {
  for (auto MatcherTag : MatcherTags) {
    const NamedDecl *FoundNamedDecl =
        Result.Nodes.getNodeAs<NamedDecl>(MatcherTag);

    if (FoundNamedDecl) {
      FoundNamedDecls[MatcherTag].emplace_back(
          llvm::cast<NamedDecl>(FoundNamedDecl));
    }
  }
}

const std::map<std::string, std::vector<const NamedDecl *>>
NamedDeclCallback::getFoundNamedDecls() const {
  return FoundNamedDecls;
}

const std::vector<const NamedDecl *>
NamedDeclCallback::getFoundNamedDecls(std::string MatcherTag) const {
  if (FoundNamedDecls.find(MatcherTag) != FoundNamedDecls.end()) {
    return FoundNamedDecls.at(MatcherTag);
  } else {
    return std::vector<const NamedDecl *>();
  }
}
