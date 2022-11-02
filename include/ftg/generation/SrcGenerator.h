//===-- SrcGenerator.h - Set of CPP code generation functions ---*- C++ -*-===//
///
/// \file
/// Defines functions which generate CPP code.
/// All functions are implemented as static method of SrcGenerator class.
///
//===----------------------------------------------------------------------===//

#ifndef FTG_GENERATION_SRCGENERATOR_H
#define FTG_GENERATION_SRCGENERATOR_H

#include "ftg/type/Type.h"
#include <string>

namespace ftg {
class SrcGenerator {
public:
  static const std::string SignatureTemplate;

  /// Generates assignment statement.
  /// \return
  /// \code{.cpp}
  /// LHS = RHS;
  /// \endcode
  static std::string genAssignStmt(const std::string &LHS,
                                   const std::string &RHS);
  /// Generates variable declaration statement with given type string.
  /// \return
  /// \code{.cpp}
  /// TypeStr VarName;
  /// \endcode
  static std::string genDeclStmt(const std::string &TypeStr,
                                 const std::string &VarName);

  /// Generates variable declaration statement with given FTG type.
  /// Refer genDeclStmt and genArrDeclStmt for generated code example.
  /// \param Array Decides whether variable will be declared as pointer or array
  /// if VarType is array type. Default value is true, which means declare as
  /// array.
  static std::string genDeclStmt(const Type &VarType,
                                 const std::string &VarName, bool Array = true);

  /// Converts FTG type into proper cpp type str.
  static std::string genTypeStr(const Type &Target);

  /// Generates array declaration statement. Array will be default initialized
  /// to avoid any side effects from garbage value.
  /// \return
  /// \code{.cpp}
  /// ArrType VarName[LEN + 1] = {};
  /// \endcode
  static std::string genArrDeclStmt(const Type &ArrType,
                                    const std::string &VarName);

  /// Generates include statements with given header.
  /// \param Header Name of header. It should be enclosed with proper
  /// delimiters. ("" or <>)
  /// \return
  /// \code{.cpp}
  /// #include Header
  /// \endcode
  static std::string genIncludeStmt(const std::string &Header);

  /// Generates assignment statement with max value limit.
  /// If Val is over Max, Max will be assigned to Var.
  /// \return
  /// \code{.cpp}
  /// Var = Val <= Max ? Val : Max;
  /// \endcode
  static std::string genAssignStmtWithMaxCheck(const std::string &Var,
                                               const std::string &Val,
                                               const std::string &Max);

  /// Reformats given code as LLVM style.
  static std::string reformat(const std::string &Code);

  /// Generates std::string to CString(char *) conversion code.
  /// \return
  /// \code
  /// const_cast<char *>(StrVar.c_str())
  /// \endcode
  static std::string genStrToCStr(const std::string &StrVar);

  static std::string genSignature(const std::string &TestName);
};
} // namespace ftg

#endif // FTG_GENERATION_SRCGENERATOR_H
