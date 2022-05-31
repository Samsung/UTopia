//===-- SrcGenerator.cpp - Set of CPP code generation functions -----------===//

#include "ftg/generation/SrcGenerator.h"
#include "ftg/utils/AssignUtil.h"
#include <clang/Format/Format.h>

using namespace ftg;

std::string SrcGenerator::genAssignStmt(const std::string &LHS,
                                        const std::string &RHS) {
  return LHS + " = " + RHS + ";";
}
std::string SrcGenerator::genDeclStmt(const std::string &TypeStr,
                                      const std::string &VarName) {
  return TypeStr + " " + VarName + ";";
}

std::string SrcGenerator::genTypeStr(const Type &Target) {
  return Target.getNameSpace() +
         util::getFuzzInputTypeString(Target.getASTTypeName());
}

std::string SrcGenerator::genDeclStmt(const Type &VarType,
                                      const std::string &VarName, bool Array) {
  return VarType.isFixedLengthArrayPtr() && Array
             ? genArrDeclStmt(static_cast<const PointerType &>(VarType),
                              VarName)
             : genDeclStmt(genTypeStr(VarType), VarName);
}

std::string SrcGenerator::genArrDeclStmt(const PointerType &ArrType,
                                         const std::string &VarName) {
  // ArrLen + 1 to deal with Cstr
  // Initializing array to avoid possible side effects.
  auto TypeStr = genTypeStr(*ArrType.getPointeeType());
  auto Size = ArrType.getArrayInfo()->getMaxLength();
  auto ArrDecl = TypeStr + " " + VarName + "[" + std::to_string(Size) + " + 1]";
  return genAssignStmt(ArrDecl, "{}");
}

std::string SrcGenerator::genIncludeStmt(const std::string &Header) {
  return "#include " + Header + "\n";
}

std::string SrcGenerator::genAssignStmtWithMaxCheck(const std::string &Var,
                                                    const std::string &Val,
                                                    const std::string &Max) {
  return Var + " = " + Val + " <= " + Max + " ? " + Val + " : " + Max + ";";
}

std::string SrcGenerator::reformat(const std::string &Code) {
  auto Replaces =
      clang::format::reformat(clang::format::getLLVMStyle(), Code,
                              clang::tooling::Range(0, Code.length()));
  return clang::tooling::applyAllReplacements(Code, Replaces).get();
}

std::string SrcGenerator::genStrToCStr(const std::string &StrVar) {
  return "const_cast<char *>(" + StrVar + ".c_str())";
}
