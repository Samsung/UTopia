
#ifndef FTG_TCANALYSIS_NAMEDDECLCALLBACK_H
#define FTG_TCANALYSIS_NAMEDDECLCALLBACK_H

#include "clang/ASTMatchers/ASTMatchFinder.h"

namespace ftg {
/**
 * @brief Callback class for AST matcher
 * @details
 */
class NamedDeclCallback
    : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  NamedDeclCallback(std::vector<std::string> MatcherTags)
      : MatcherTags(MatcherTags),
        FoundNamedDecls(
            std::map<std::string, std::vector<const clang::NamedDecl *>>()) {}
  virtual void
  run(const clang::ast_matchers::MatchFinder::MatchResult &Result) final;
  const std::map<std::string, std::vector<const clang::NamedDecl *>>
  getFoundNamedDecls() const;
  const std::vector<const clang::NamedDecl *>
  getFoundNamedDecls(std::string MatcherTag) const;

private:
  std::vector<std::string> MatcherTags;
  std::map<std::string, std::vector<const clang::NamedDecl *>> FoundNamedDecls;
};
} // namespace ftg

#endif // FTG_TCANALYSIS_NAMEDDECLCALLBACK_H
