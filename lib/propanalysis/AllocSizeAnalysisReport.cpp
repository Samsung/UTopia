#include "ftg/propanalysis/AllocSizeAnalysisReport.h"

namespace ftg {

const std::string AllocSizeAnalysisReport::getReportType() const {
  return "AllocSize";
}

Json::Value AllocSizeAnalysisReport::toJson() const {
  Json::Value Root, Object;
  for (const auto &[Key, Value] : Result) {
    if (!Value)
      continue;

    Object[Key] = Value;
  }
  Root[getReportType()] = Object;
  return Root;
}

bool AllocSizeAnalysisReport::fromJson(Json::Value Report) {
  if (!Report.isMember(getReportType()))
    return false;
  auto AllocSizeReport = Report[getReportType()];
  for (auto MemberName : AllocSizeReport.getMemberNames()) {
    auto AllocVal = AllocSizeReport[MemberName];
    set(MemberName, AllocVal.asBool());
  }
  return true;
}

void AllocSizeAnalysisReport::add(ArgFlow &AF, std::vector<int> Indices) {
  auto &A = AF.getLLVMArg();
  set(A, AF.isAllocSize(), Indices);

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
