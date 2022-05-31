#ifndef FTG_PROPANALYSIS_ARGFLOW_H
#define FTG_PROPANALYSIS_ARGFLOW_H

#include "ftg/propanalysis/FieldInfo.h"
#include "ftg/propanalysis/StructInfo.h"
#include "ftg/targetanalysis/ParamReport.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstrTypes.h"

namespace ftg {

enum AnalysisState {
  AnalysisState_Not_Analyzed, // default value
  AnalysisState_Pre_Analyzed, // in function declaration
  AnalysisState_Unidentified, // in function declaration
  AnalysisState_Analyzing,    // in function definition
  AnalysisState_Analyzed      // in function definition
};

bool isUnionFieldAccess(const llvm::Instruction *I);

class ArgFlow;
class FieldInfo;
class StructInfo;

class ArgFlow {
  friend class TargetLibAnalyzer;
  friend class ArgFlowAnalyzer;
  friend class AllocAnalyzer;
  friend class PreDefinedAnalyzer;

public:
  unsigned LoopDepth = 0;
  bool LoopExit = false;

  ArgFlow(llvm::Argument &A);
  void setState(AnalysisState State);
  void setFilePathString(bool IsFilePathString);
  void setStruct(llvm::StructType *ST);
  void setIsVariableLenArray(bool Value);
  void setIsArray(bool Value);
  void setSizeArg(llvm::Argument &A);
  void setToArrLen(llvm::Argument &RelatedArg);
  void setToArrLen(unsigned FieldNum); // for field result

  bool isArray() const;
  bool isArrayLen() const;
  bool hasLenRelatedArg();
  bool isFilePathString();
  bool isStruct();
  bool isField();
  bool isUsedByRet();
  bool isVariableLenArray() const;

  llvm::Argument &getLLVMArg();
  AnalysisState getState() const;
  ArgDir getArgDir() const;
  ArgAlloc getArgAlloc();
  // TODO: implement to handle more than one related argss
  unsigned getLenRelatedArgNo();
  std::shared_ptr<StructInfo> getStructInfo();
  std::shared_ptr<FieldInfo> getFieldInfo();
  ArgFlow &getOrCreateFieldFlow(unsigned FieldNum);
  std::set<size_t> &getArrIndices();
  const std::set<llvm::Argument *> &getSizeArgs();

  void mergeAlloc(const ArgFlow &CalleeArgResult);
  void mergeDirection(const ArgFlow &CalleeArgResult);
  void mergeArray(const ArgFlow &CalleeArgResult, llvm::CallBase &C);
  void collectRelatedLengthField(llvm::Value &V);
  void collectRelatedLengthArg(llvm::Value &V);
  ArgFlow &operator|=(const ArgDir &ArgDir);
  void setArgDir(unsigned ArgDir);

private:
  AnalysisState State = AnalysisState_Not_Analyzed;
  ArgDir Direction = Dir_NoOp;
  ArgAlloc Allocation = Alloc_None;
  llvm::Argument &Arg;
  bool IsFilePathString = false;
  bool IsArray = false;
  bool IsArrayLen = false;
  bool IsVariableLenArray = false;
  bool IsUsedByRet = false;
  std::set<size_t> ArrIndexes;
  std::set<llvm::Argument *> SizeArgs;
  std::set<llvm::Argument *> ArrayArgs;
  std::vector<std::string> ExternAPIs;
  // If result for Struct, STInfo will be assigned
  std::shared_ptr<StructInfo> STInfo;
  // If result for Field, FDInfo will be assigned
  std::shared_ptr<FieldInfo> FDInfo;
  unsigned Depth = 0; // depth from argument

  llvm::Argument *findRelatedArg(llvm::Value &V,
                                 std::set<llvm::Value *> &Visit);
  void setArgAlloc(unsigned ArgAlloc);
  void setUsedByRet(bool ArgRet);
  void setField(unsigned FieldNum, ArgFlow &Parent);

  int findRelatedField(llvm::Value &V,
                       llvm::SmallSet<llvm::Value *, 8> &VisitedValues,
                       ArgFlow &ParentResult);
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_ARGFLOW_H
