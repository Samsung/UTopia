#include "ArrayAnalysisReport.h"

namespace ftg {

const std::string ArrayAnalysisReport::getReportType() const { return "Array"; }

Json::Value ArrayAnalysisReport::toJson() const {
  Json::Value Root, Element;
  for (auto Iter : Result)
    Element[Iter.first] = Iter.second;
  Root[getReportType()] = Element;
  return Root;
}

bool ArrayAnalysisReport::fromJson(Json::Value Report) {
  if (!Report.isMember(getReportType()))
    return false;

  auto ArrayReport = Report[getReportType()];
  for (auto MemberName : ArrayReport.getMemberNames())
    set(MemberName, ArrayReport[MemberName].asInt());

  return true;
}

void ArrayAnalysisReport::add(ArgFlow &AF, std::vector<int> Indices) {
  auto &A = AF.getLLVMArg();
  int Result = NO_ARRAY;
  if (AF.isArray()) {
    Result = ARRAY_NOLEN;
    auto FI = AF.getFieldInfo();
    if (FI) {
      if (FI->SizeFields.size() > 0)
        Result = *FI->SizeFields.begin();
    } else if (AF.hasLenRelatedArg())
      Result = AF.getLenRelatedArgNo();
  }
  set(A, Result, Indices);

  auto SI = AF.getStructInfo();
  if (!SI)
    return;

  for (auto Iter : SI->FieldResults) {
    if (!Iter.second)
      continue;

    Indices.push_back(Iter.first);
    add(*Iter.second, Indices);
    Indices.pop_back();
  }
}

} // namespace ftg
