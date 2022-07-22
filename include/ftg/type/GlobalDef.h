#ifndef FTG_GLOBAL_DEF_H
#define FTG_GLOBAL_DEF_H

#include "clang/AST/Decl.h"
#include <cstdint>
#include <string>
#include <vector>

namespace ftg {

class EnumConst {
public:
  EnumConst(std::string Name, int64_t Value);
  std::string getName() const;
  int64_t getValue() const;

protected:
  std::string Name;
  int64_t Value;
};

class Enum {
public:
  Enum(std::string Name, std::vector<EnumConst> Enumerators);
  Enum(const clang::EnumDecl &ClangDecl);
  std::string getName() const;
  const std::vector<EnumConst> &getElements() const;

private:
  std::string Name;          // mangled - unique name
  std::vector<EnumConst> Enumerators;
};

} // namespace ftg

#endif
