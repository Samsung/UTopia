#include "FilePathAnalysisReport.h"

namespace ftg {

FilePathAnalysisReport::FilePathAnalysisReport() = default;

FilePathAnalysisReport::FilePathAnalysisReport(
    const FilePathAnalysisReport &Report) {
  for (const auto &Iter : Report.get())
    set(Iter.first, Iter.second);
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
  for (auto Iter : Result)
    Member[Iter.first] = Iter.second;
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
