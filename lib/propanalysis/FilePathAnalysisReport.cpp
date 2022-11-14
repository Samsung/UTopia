#include "FilePathAnalysisReport.h"

namespace ftg {

FilePathAnalysisReport::FilePathAnalysisReport() {
  set("fopen", 0, true);
  set("freopen", 0, true);
  set("open", 0, true);
  set("open64", 0, true);
}

bool FilePathAnalysisReport::fromJson(Json::Value Report) {
  try {
    if (!Report.isMember(getReportType()))
      return false;

    auto FilePathReport = Report[getReportType()];
    for (auto MemberName : FilePathReport.getMemberNames()) {
      set(MemberName, FilePathReport[MemberName].asBool());
    }
  } catch (Json::Exception &E) {
    return false;
  }
  return true;
}

const std::string FilePathAnalysisReport::getReportType() const {
  return "FilePath";
}

Json::Value FilePathAnalysisReport::toJson() const {
  Json::Value Root, Member;
  for (const auto &[Key, Value] : Result) {
    if (!Value)
      continue;

    Member[Key] = Value;
  }
  Root[getReportType()] = Member;
  return Root;
}

void FilePathAnalysisReport::add(ArgFlow &AF, std::vector<int> Indices) {
  auto &A = AF.getLLVMArg();
  set(A, AF.isFilePathString(), Indices);

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

} // end namespace ftg
