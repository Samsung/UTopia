#ifndef FTG_CONSTANTANALYSIS_ASTVALUE_H
#define FTG_CONSTANTANALYSIS_ASTVALUE_H

#include "ftg/JsonSerializable.h"
#include "clang/AST/Expr.h"
#include "llvm/Support/raw_ostream.h"

namespace ftg {

struct ASTValueData {

  typedef enum { VType_INT, VType_FLOAT, VType_STRING, VType_ARRAY } VType;

  VType Type;
  std::string Value;

  ASTValueData(VType Type, std::string Value);

  std::string getAsString(VType Type) const;
  bool operator==(const ASTValueData &Rhs) const;
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &O,
                                       const ASTValueData &Rhs);
};

class ASTValue : public JsonSerializable {
public:
  ASTValue();
  ASTValue(const clang::Expr &E, const clang::ASTContext &Ctx);
  ASTValue(Json::Value Json);
  ASTValue(const ASTValue &Rhs);

  std::pair<bool, std::vector<ASTValueData>>
  getData(const clang::Expr &E, const clang::ASTContext &Ctx) const;

  bool isValid() const;

  Json::Value toJson() const override;
  bool fromJson(Json::Value Json) override;

  bool operator==(const ASTValue &Rhs) const;
  ASTValue &operator=(const ASTValue &Rhs);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &O,
                                       const ASTValue &Rhs);

  bool isArray() const;
  const std::vector<ASTValueData> &getValues() const;

private:
  std::string getValueAsReal(const clang::Expr &E,
                             const clang::ASTContext &Ctx) const;
  std::string getValueAsString(const clang::Expr &E,
                               const clang::ASTContext &Ctx) const;
  std::string getValue(const clang::StringLiteral &SL) const;
  std::vector<ASTValueData> getDataForArray(const clang::Expr &E,
                                            const clang::ASTContext &Ctx) const;

  const clang::Expr *getValueExpr(const clang::DeclRefExpr &E,
                                  const clang::ASTContext &Ctx) const;

  bool IsArray;
  std::vector<ASTValueData> Values;
};

} // namespace ftg

#endif // FTG_CONSTANTANALYSIS_ASTVALUE_H
