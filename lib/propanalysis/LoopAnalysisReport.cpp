#include "LoopAnalysisReport.h"

namespace ftg {

void LoopAnalysisReport::add(ArgFlow &AF, std::vector<int> Indices) {
  LoopAnalysisResult Result = {AF.LoopExit, AF.LoopDepth};
  set(AF.getLLVMArg(), Result, Indices);

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

bool LoopAnalysisReport::fromJson(Json::Value Report) {
  try {
    if (!Report.isMember(getReportType()))
      return false;
  } catch (Json::Exception &E) {
    return false;
  }

  auto LoopReport = Report[getReportType()];
  for (auto MemberName : LoopReport.getMemberNames()) {
    try {
      const auto &Element = LoopReport[MemberName];
      LoopAnalysisResult Result = {Element["LoopExit"].asBool(),
                                   Element["LoopDepth"].asUInt()};
      set(MemberName, Result);
    } catch (Json::Exception &E) {
    }
  }
  return true;
}

const std::string LoopAnalysisReport::getReportType() const { return "Loop"; }

Json::Value LoopAnalysisReport::toJson() const {
  Json::Value Root, Element;
  for (auto Iter : Result) {
    Element[Iter.first]["LoopExit"] = Iter.second.LoopExit;
    Element[Iter.first]["LoopDepth"] = Iter.second.LoopDepth;
  }
  Root[getReportType()] = Element;
  return Root;
}

} // namespace ftg
