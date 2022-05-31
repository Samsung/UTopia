#include "AllocAnalysisReport.h"

namespace ftg {

const std::string AllocAnalysisReport::getReportType() const { return "Alloc"; }

Json::Value AllocAnalysisReport::toJson() const {

  Json::Value Root, Object;

  for (auto Iter : Result) {
    Object[Iter.first] = Iter.second;
  }

  Root[getReportType()] = Object;
  return Root;
}

bool AllocAnalysisReport::fromJson(Json::Value Report) {
  if (!Report.isMember(getReportType()))
    return false;
  auto AllocReport = Report[getReportType()];
  for (auto MemberName : AllocReport.getMemberNames()) {
    auto AllocVal = AllocReport[MemberName];
    set(MemberName, AllocVal.asUInt());
  }
  return true;
}

void AllocAnalysisReport::add(ArgFlow &AF, std::vector<int> Indices) {
  auto &A = AF.getLLVMArg();
  set(A, AF.getArgAlloc(), Indices);

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
