#include "DirectionAnalysisReport.h"

namespace ftg {

const std::string DirectionAnalysisReport::getReportType() const {
  return "Direction";
}

Json::Value DirectionAnalysisReport::toJson() const {
  Json::Value Root, Object;

  for (auto Iter : Result) {
    Object[Iter.first] = Iter.second;
  }

  Root[getReportType()] = Object;
  return Root;
}

bool DirectionAnalysisReport::fromJson(Json::Value Report) {
  if (!Report.isMember(getReportType()))
    return false;
  auto DirectionReport = Report[getReportType()];
  for (auto MemberName : DirectionReport.getMemberNames()) {
    auto DirectionVal = DirectionReport[MemberName];
    set(MemberName, DirectionVal.asUInt());
  }
  return true;
}

void DirectionAnalysisReport::add(ArgFlow &AF, std::vector<int> Indices) {
  auto &A = AF.getLLVMArg();
  set(A, AF.getArgDir(), Indices);

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
