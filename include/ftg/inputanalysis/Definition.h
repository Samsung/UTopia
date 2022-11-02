#ifndef FTG_INPUTANALYSIS_DEFINITION_H
#define FTG_INPUTANALYSIS_DEFINITION_H

#include "ftg/analysis/TypeAnalysisReport.h"
#include "ftg/constantanalysis/ASTValue.h"
#include "ftg/type/Type.h"
#include "json/json.h"

namespace ftg {

struct Definition {

  enum DeclType {
    DeclType_None,
    DeclType_Decl,
    DeclType_Global,
    DeclType_StaticLocal
  };

  // NOTE: Identifier
  unsigned ID;

  // NOTE: Locations
  std::string Path;
  unsigned Offset;
  unsigned Length;

  // NOTE: Properties
  bool FilePath;
  bool BufferAllocSize;
  bool Array;
  bool ArrayLen;
  std::set<unsigned> ArrayIDs;
  std::set<unsigned> ArrayLenIDs;
  bool LoopExit;

  // NOTE: Assign Statements
  std::shared_ptr<Type> DataType;
  DeclType Declaration;
  bool AssignOperatorRequired;
  unsigned TypeOffset;
  std::string TypeString;
  unsigned EndOffset;
  std::string VarName;
  std::string Namespace;
  ASTValue Value;
  std::set<std::string> Filters;

  Definition();
  bool fromJson(const Json::Value &Root, const TypeAnalysisReport &Report);
  Json::Value toJson() const;
  template <typename T> friend T &operator<<(T &O, const Definition &RHS) {

    O << RHS.toJson().toStyledString();
    return O;
  }
};

} // namespace ftg

#endif // FTG_INPUTANALYSIS_DEFINITION_H
