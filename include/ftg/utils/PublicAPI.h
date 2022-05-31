#ifndef FTG_UTILS_PUBLICAPI_H
#define FTG_UTILS_PUBLICAPI_H

#include <map>
#include <set>
#include <string>

namespace clang {

class FunctionDecl;
class ASTContext;

} // namespace clang

namespace ftg {

/// TODO: Rename class to manage both public api and exports & deps json
/// This class manages PublicAPI related methods such as loading Json and
/// finding clang::Functiondecl, etc.
class PublicAPI {
public:
  PublicAPI() = default;
  PublicAPI(PublicAPI &) = delete;

  /// @brief Load PublicAPI.Json
  void loadJson(std::string jsonFilePath);
  /// @brief Load exports_and_dependencies.Json
  void loadExportsNDepsJson(std::string JsonPath, std::string LibName);
  /// @brief Check if input funcName is PublicAPI
  bool isPublicAPI(std::string funcName) const;
  /// @brief Check if input funcName is DeprecatedAPI
  bool isDeprecated(std::string funcName) const;
  /// @brief Get the PublicAPIList
  const std::set<std::string> &getPublicAPIList() const;
  /// @brief Get the size of PublicAPIList
  size_t getPublicAPISize() const;
  /// @brief Extracts set of clang::FunctionDecl of its PublicAPIList from AST
  const std::set<clang::FunctionDecl *>
  findClangFunctionDecls(clang::ASTContext &Ctx) const;

private:
  std::set<std::string> PublicAPIs;
  std::set<std::string> DeprecatedAPIs;
};

} // namespace ftg

#endif // FTG_UTILS_PUBLICAPI_H
