#ifndef FTG_TYPE_GLOBALDEF_H
#define FTG_TYPE_GLOBALDEF_H

#include "ftg/JsonSerializable.h"
#include "clang/AST/Decl.h"
#include <cstdint>
#include <string>
#include <vector>

namespace ftg {

class EnumConst : public JsonSerializable {
public:
  EnumConst(std::string Name, int64_t Value);
  EnumConst(Json::Value Json);
  std::string getName() const;
  int64_t getValue() const;

  Json::Value toJson() const override;
  bool fromJson(Json::Value) override;

protected:
  std::string Name;
  int64_t Value;
};

class Enum : public JsonSerializable {
public:
  Enum(std::string Name, std::vector<EnumConst> Enumerators);
  Enum(const clang::EnumDecl &ClangDecl);
  Enum(Json::Value Json);
  std::string getName() const;
  const std::vector<EnumConst> &getElements() const;

  Json::Value toJson() const override;;
  bool fromJson(Json::Value) override;

private:
  std::string Name;          // mangled - unique name
  std::vector<EnumConst> Enumerators;
};

} // namespace ftg

#endif // FTG_TYPE_GLOBALDEF_H
