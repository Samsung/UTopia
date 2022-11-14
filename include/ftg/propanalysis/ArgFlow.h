#ifndef FTG_PROPANALYSIS_ARGFLOW_H
#define FTG_PROPANALYSIS_ARGFLOW_H

#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstrTypes.h"

namespace ftg {

enum Dir {
  Dir_NoOp = 0x000,
  Dir_In = 0x100,
  Dir_Out = 0x010,
  Dir_Unidentified = 0x001
};

using ArgDir = unsigned;

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
  friend class AllocSizeAnalyzer;
  friend class PreDefinedAnalyzer;

public:
  class FieldInfo {
  public:
    unsigned FieldNum;
    std::set<unsigned> SizeFields;
    std::set<unsigned> ArrayFields;
    std::set<llvm::Value *> Values;
    ArgFlow &Parent; // struct result

    FieldInfo(unsigned FieldNum, ArgFlow &Parent);
    bool hasLenRelatedField();
  };

  class StructInfo {
  public:
    ArgDir FieldsDirection = Dir_NoOp;
    bool FieldsAllocSize = false;
    std::map<unsigned, std::shared_ptr<ArgFlow>> FieldResults;
    llvm::StructType *StructType;
    StructInfo(llvm::StructType *ST);
  };

  unsigned LoopDepth = 0;
  bool LoopExit = false;

  ArgFlow(llvm::Argument &A);
  void setState(AnalysisState State);
  void setFilePathString();
  void setStruct(llvm::StructType *ST);
  void setIsVariableLenArray(bool Value);
  void setIsArray(bool Value);
  void setSizeArg(llvm::Argument &A);
  void setToArrLen(llvm::Argument &RelatedArg);
  void setToArrLen(unsigned FieldNum); // for field result

  bool isAllocSize() const;
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
  // TODO: implement to handle more than one related argss
  unsigned getLenRelatedArgNo();
  std::shared_ptr<StructInfo> getStructInfo();
  std::shared_ptr<FieldInfo> getFieldInfo();
  ArgFlow &getOrCreateFieldFlow(unsigned FieldNum);
  std::set<size_t> &getArrIndices();
  const std::set<llvm::Argument *> &getSizeArgs();

  void mergeAllocSize(const ArgFlow &CalleeArgResult);
  void mergeDirection(const ArgFlow &CalleeArgResult);
  void mergeArray(const ArgFlow &CalleeArgResult, llvm::CallBase &C);
  void collectRelatedLengthField(llvm::Value &V);
  void collectRelatedLengthArg(llvm::Value &V);
  ArgFlow &operator|=(const ArgDir &ArgDir);
  void setArgDir(unsigned ArgDir);

private:
  AnalysisState State = AnalysisState_Not_Analyzed;
  ArgDir Direction = Dir_NoOp;
  llvm::Argument &Arg;
  bool IsFilePathString = false;
  bool IsAllocSize = false;
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
  void setAllocSize();
  void setUsedByRet(bool ArgRet);
  void setField(unsigned FieldNum, ArgFlow &Parent);

  int findRelatedField(llvm::Value &V,
                       llvm::SmallSet<llvm::Value *, 8> &VisitedValues,
                       ArgFlow &ParentResult);
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_ARGFLOW_H
