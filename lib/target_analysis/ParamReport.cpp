#include "ftg/targetanalysis/ParamReport.h"
#include <cassert>

namespace ftg {
ParamReport::ParamReport(std::string FunctionName, unsigned Index,
                         Param *Definition, ArgDir ArgDirection,
                         bool IsArrayType, bool IsArrayLength,
                         ArgAlloc ArgAllocation, bool IsFilePath,
                         bool HasLengthRelatedArg, unsigned LengthRelatedArgNo)
    : Def(Definition), Dir(ArgDirection), IsArr(IsArrayType),
      IsArrLen(IsArrayLength), Alloc(ArgAllocation), ParamIdx(Index),
      HasLenRelatedArg(HasLengthRelatedArg),
      LenRelatedArgNo(LengthRelatedArgNo), IsFilePath(IsFilePath),
      FuncName(FunctionName) {}

std::string ParamReport::getFunctionName() const { return FuncName; }
const Param &ParamReport::getDefinition() const { return *Def; }
void ParamReport::setDirection(ArgDir Direction) { Dir = Direction; }
ArgDir ParamReport::getDirection() const { return Dir; }
void ParamReport::updateDirection(ArgDir Direction) { Dir |= Direction; }
void ParamReport::setIsArray(bool IsArray) { IsArr = IsArray; }
bool ParamReport::isArray() const { return IsArr; }
void ParamReport::setIsArrayLen(bool IsArrayLen) { IsArrLen = IsArrayLen; }
bool ParamReport::isArrayLen() const { return IsArrLen; }
void ParamReport::setAllocation(ArgAlloc Allocation) { Alloc = Allocation; }
void ParamReport::updateAllocation(ArgAlloc Allocation) { Alloc |= Allocation; }
ArgAlloc ParamReport::getAllocation() const { return Alloc; }
void ParamReport::setParamIndex(unsigned ParamIndex) { ParamIdx = ParamIndex; }
unsigned ParamReport::getParamIndex() const { return ParamIdx; }
void ParamReport::setHasLenRelatedArg(bool HasLengthRelatedArg) {
  HasLenRelatedArg = HasLengthRelatedArg;
}
bool ParamReport::hasLenRelatedArg() const { return HasLenRelatedArg; }
void ParamReport::setLenRelatedArgNo(unsigned LengthRelatedArgNo) {
  LenRelatedArgNo = LengthRelatedArgNo;
}
unsigned ParamReport::getLenRelatedArgNo() const { return LenRelatedArgNo; }
void ParamReport::setIsFilePathString(bool IsFilePathString) {
  IsFilePath = IsFilePathString;
}
bool ParamReport::isFilePathString() const { return IsFilePath; }
bool ParamReport::isStruct() const {
  return ChildParams.size() > 0 ? true : false;
}
ParamReport &ParamReport::getChildParam(size_t Index) const {
  assert(Index < ChildParams.size() &&
         "Out of child param index in this Param!");
  return *ChildParams[Index];
}

void ParamReport::addChildParam(std::shared_ptr<ParamReport> ChildParam) {
  ChildParams.push_back(ChildParam);
}

std::vector<std::shared_ptr<ParamReport>> &ParamReport::getChildParams() {
  return ChildParams;
}

} // namespace ftg
