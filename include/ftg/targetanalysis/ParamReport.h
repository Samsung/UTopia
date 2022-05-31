#ifndef FTG_TARGETANALYSIS_PARAMREPORT_H
#define FTG_TARGETANALYSIS_PARAMREPORT_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace ftg {

class Param;

class ParamReport;

enum Dir {
  Dir_NoOp = 0x000,
  Dir_In = 0x100,
  Dir_Out = 0x010,
  Dir_Unidentified = 0x001
};

enum Alloc {
  Alloc_None = 0x000,
  Alloc_Size = 0x001,
  Alloc_Address = 0x010,
  Alloc_Free = 0x100
};

using ArgDir = unsigned;
using ArgAlloc = unsigned;

class ParamReport {
public:
  ParamReport(std::string FunctionName, unsigned Index,
              Param *Definition = nullptr, ArgDir ArgDirection = Dir_NoOp,
              bool IsArrayType = false, bool IsArrayLength = false,
              ArgAlloc ArgAllocation = Alloc_None, bool IsFilePath = false,
              bool HasLengthRelatedArg = false,
              unsigned LengthRelatedArgNo = 0);
  std::string getFunctionName() const;
  const Param &getDefinition() const;
  void setDirection(ArgDir Direction);
  ArgDir getDirection() const;
  void updateDirection(ArgDir Direction);
  void setIsArray(bool IsArray);
  bool isArray() const;
  void setIsArrayLen(bool IsArrayLen);
  bool isArrayLen() const;
  void setAllocation(ArgAlloc Allocation);
  void updateAllocation(ArgAlloc Allocation);
  ArgAlloc getAllocation() const;
  void setParamIndex(unsigned ParamIndex);
  unsigned getParamIndex() const;
  void setHasLenRelatedArg(bool HasLengthRelatedArg);
  bool hasLenRelatedArg() const;
  void setLenRelatedArgNo(unsigned LengthRelatedArgNo);
  unsigned getLenRelatedArgNo() const;
  void setIsFilePathString(bool IsFilePathString);
  bool isFilePathString() const;
  bool isStruct() const;
  ParamReport &getChildParam(size_t Index) const;
  void addChildParam(std::shared_ptr<ParamReport> ChildParam);
  std::vector<std::shared_ptr<ParamReport>> &getChildParams();

private:
  Param *Def;
  ArgDir Dir;
  bool IsArr;
  bool IsArrLen;
  ArgAlloc Alloc;
  unsigned ParamIdx;
  bool HasLenRelatedArg;
  unsigned LenRelatedArgNo;
  bool IsFilePath;
  std::string FuncName;
  std::vector<std::shared_ptr<ParamReport>> ChildParams;
};
} // namespace ftg

#endif // FTG_TARGETANALYSIS_PARAMREPORT_H
