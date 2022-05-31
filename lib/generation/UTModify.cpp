#include "ftg/generation/UTModify.h"
#include "ftg/tcanalysis/TCAnalyzerFactory.h"
#include "ftg/utils/AssignUtil.h"
#include <experimental/filesystem>

using namespace llvm;
using namespace clang::tooling;
namespace fs = std::experimental::filesystem;

namespace ftg {

const std::string UTModify::HeaderName = "autofuzz.h";
const std::string UTModify::UTEntryFuncName = "enterAutofuzz";

UTModify::UTModify(const Fuzzer &F, const SourceAnalysisReport &SourceReport) {
  std::vector<Replacement> ModPoints;
  std::map<std::string, std::string> FuzzVarDecls;
  std::map<std::string, std::vector<std::string>> FuzzVarFlagDecls;
  std::map<std::string, std::vector<UTModify::GlobalVarSetter>> Setters;
  auto UTPath = F.getUT().getFilePath();
  for (const auto &Iter : F.getFuzzInputMap()) {
    auto &Input = Iter.second;
    assert(Input && "Unexpected Program State");
    generateFuzzVarDeclarations(FuzzVarDecls, FuzzVarFlagDecls, *Input);
    generateFuzzVarGlobalSetters(Setters, *Input);
    generateFuzzVarReplacements(ModPoints, *Input);
  }
  generateTop(ModPoints, FuzzVarDecls, FuzzVarFlagDecls, Setters, UTPath,
              SourceReport);
  generateBottom(ModPoints, FuzzVarFlagDecls, Setters, F.getUT(), SourceReport);
  if (!generateHeader(fs::path(UTPath).parent_path().string())) {
    clear();
    return;
  }
  generateMainDeletion(ModPoints, SourceReport);

  for (const auto Iter : ModPoints) {
    if (FileReplaceMap[Iter.getFilePath().str()].add(Iter)) {
      clear();
      return;
    }
  }
}

const std::map<std::string, std::string> &UTModify::getNewFiles() const {
  return FileNewMap;
}

const std::map<std::string, clang::tooling::Replacements> &
UTModify::getReplacements() const {
  return FileReplaceMap;
}

const std::string UTModify::CPPMacroEnd = "#ifdef __cplusplus\n"
                                          "}\n"
                                          "#endif\n";
const std::string UTModify::CPPMacroStart = "#ifdef __cplusplus\n"
                                            "extern \"C\" {\n"
                                            "#endif\n";
const std::string UTModify::FilePathPrefix = "FUZZ_FILEPATH_PREFIX";

void UTModify::clear() {
  FileNewMap.clear();
  FileReplaceMap.clear();
}

std::string UTModify::generateAssignStatement(const FuzzInput &Input) const {
  const auto &Def = Input.getDef();

  auto VarName = Input.getFuzzVarName();
  auto Identifier = generateIdentifier(Def);
  auto &T = Def.DataType;
  assert(T && "Unexpected Program State");

  if (T->isFixedLengthArrayPtr())
    return util::getFixedLengthArrayAssignStmt(Identifier, VarName);

  if (Def.Declaration == Definition::DeclType_Global ||
      Def.Declaration == Definition::DeclType_StaticLocal)
    return Identifier + " = " + VarName + ";";

  if (Def.AssignOperatorRequired)
    return " = " + VarName;

  return VarName;
}

void UTModify::generateBottom(
    std::vector<Replacement> &Replace,
    const std::map<std::string, std::vector<std::string>> &FuzzVarFlagDecls,
    const std::map<std::string, std::vector<GlobalVarSetter>> &Setters,
    const Unittest &UT, const SourceAnalysisReport &SourceReport) const {
  auto UTPath = UT.getFilePath();
  auto UTType = UT.getType();

  std::set<std::string> Paths = {UTPath};
  for (const auto &Iter : Setters)
    Paths.emplace(Iter.first);

  for (const auto &Path : Paths) {
    auto EndOffset = SourceReport.getEndOffset(Path);
    assert(EndOffset > 0 && "Unexpected Program State");

    std::string Content;
    auto Iter = Setters.find(Path);
    if (Iter != Setters.end()) {
      for (const auto &Iter2 : Iter->second) {
        Content += ("void " + Iter2.FuncName + "() {\n" + util::getIndent(1) +
                    Iter2.FuncBody + "\n" + "}\n");
      }
    }

    if (Path == UTPath) {
      Content += "void " + UTEntryFuncName + "() {\n";
      for (const auto &Iter2 : Setters)
        for (const auto &Iter3 : Iter2.second)
          Content += util::getIndent(1) + Iter3.FuncName + "();\n";

      for (const auto &Iter2 : FuzzVarFlagDecls)
        for (const auto &Iter3 : Iter2.second)
          Content += util::getIndent(1) + Iter3 + " = 1;\n";

      auto TCFactory = createTCAnalyzerFactory(UTType);
      assert(TCFactory && "Unexpected Program State");
      auto CallWriter = TCFactory->createTCCallWriter();
      assert(CallWriter && "Unexpected Program State");

      Content += CallWriter->getTCCall(UT, util::getIndent(1));
      Content += "}\n";
    }
    if (!Content.empty())
      Content = CPPMacroStart + Content + CPPMacroEnd;
    Replace.emplace_back(Path, EndOffset, 0, Content);
  }
}

std::string UTModify::generateDeclTypeName(const Type &T) const {
  std::string TypeName = "";
  if (llvm::isa<PointerType>(&T)) {
    const auto *PointerT = &T;
    const auto *PointeeT = &(PointerT->getPointeeType());
    assert(PointeeT && "Unexpected Program State");

    while (PointerT != PointeeT) {
      TypeName += "*";
      PointerT = PointeeT;
      PointeeT = &(PointerT->getPointeeType());
    }
    TypeName = PointeeT->getASTTypeName() + " " + TypeName;
  } else {
    TypeName = T.getASTTypeName();
  }
  return util::stripConstExpr(TypeName);
}

void UTModify::generateFuzzVarDeclarations(
    std::map<std::string, std::string> &FuzzVarDecls,
    std::map<std::string, std::vector<std::string>> &FuzzVarFlagDecls,
    const FuzzInput &Input) const {
  const auto &Def = Input.getDef();
  const auto &DataType = Def.DataType;
  assert(DataType && "Unexpected Program State");

  auto Path = Def.Path;
  auto VarName = Input.getFuzzVarName();
  auto TypeName = generateDeclTypeName(*DataType);
  auto Declaration = "extern " + TypeName + " " + VarName + ";\n";
  if (DataType->isFixedLengthArrayPtr())
    Declaration +=
        "extern unsigned " + util::getAvasFuzzSizeVarName(VarName) + ";\n";

  auto Iter = FuzzVarDecls.find(Path);
  if (Iter == FuzzVarDecls.end())
    FuzzVarDecls.emplace(Path, Declaration);
  else
    Iter->second += Declaration;

  if (Def.Declaration == Definition::DeclType_StaticLocal) {
    auto Iter = FuzzVarFlagDecls.find(Path);
    auto FlagVarName = util::getStaticLocalFlagVarName(VarName);
    if (Iter == FuzzVarFlagDecls.end())
      FuzzVarFlagDecls.emplace(Path, std::vector<std::string>({FlagVarName}));
    else
      Iter->second.emplace_back(FlagVarName);
  }
}

void UTModify::generateFuzzVarGlobalSetters(
    std::map<std::string, std::vector<GlobalVarSetter>> &Setters,
    const FuzzInput &Input) const {
  const auto &Def = Input.getDef();
  if (Def.Declaration != Definition::DeclType_Global)
    return;

  auto Path = Def.Path;
  auto FuncName = util::getAssignGlobalFuncName(Input.getFuzzVarName());
  auto FuncBody = util::getIndent(1) + generateAssignStatement(Input);
  UTModify::GlobalVarSetter Setter = {FuncName, FuncBody};
  auto Iter = Setters.find(Path);
  if (Iter == Setters.end()) {
    Setters.emplace(Path, std::vector<UTModify::GlobalVarSetter>({Setter}));
    return;
  }
  Iter->second.emplace_back(Setter);
}

void UTModify::generateFuzzVarReplacements(std::vector<Replacement> &Replace,
                                           const FuzzInput &Input) const {
  const auto &Def = Input.getDef();
  const auto &T = Def.DataType;
  assert(T && "Unexpected Program State");

  if (Def.Declaration != Definition::DeclType_None) {
    auto TypeString = util::trim(Def.TypeString);
    auto Stripped = util::stripConstExpr(TypeString);
    if (Stripped != TypeString) {
      Replace.emplace_back(Def.Path, Def.TypeOffset,
                           (unsigned int)Def.TypeString.size(), Stripped + " ");
    }
  }

  if (Def.Declaration == Definition::DeclType_Global)
    return;

  auto AssignStmt = generateAssignStatement(Input);
  if (Def.Declaration == Definition::DeclType_StaticLocal) {
    auto FlagVarName = util::getStaticLocalFlagVarName(Input.getFuzzVarName());
    AssignStmt = " if (" + FlagVarName + ") { " + FlagVarName + " = 0; " +
                 AssignStmt + " }";
    Replace.emplace_back(Def.Path, Def.EndOffset, 0, AssignStmt);
    return;
  }

  if (T->isFixedLengthArrayPtr()) {
    Replace.emplace_back(Def.Path, Def.EndOffset, 0, " " + AssignStmt);
    return;
  }

  Replace.emplace_back(Def.Path, Def.Offset, Def.Length, AssignStmt);
}

bool UTModify::generateHeader(const std::string &BasePath) {
  const std::string CheckerMacro = "AUTOFUZZ";
  const std::string HeaderGuard = CheckerMacro + "_H";
  const std::string ExceptionName = "AutofuzzException";
  const std::string Content =
      "#ifndef " + HeaderGuard +
      "\n"
      "#define " +
      HeaderGuard +
      "\n\n"
      "#ifndef " +
      CheckerMacro +
      "\n"
      "#error " +
      CheckerMacro +
      " is not defined. Do not include this header "
      "without " +
      CheckerMacro +
      " Definition.\n"
      "#endif // " +
      CheckerMacro +
      "\n\n"
      "#ifndef " +
      FilePathPrefix +
      "\n"
      "#define " +
      FilePathPrefix +
      " ./\n"
      "#endif // " +
      FilePathPrefix +
      "\n\n"
      "#ifdef __cplusplus\n"
      "#include <exception>\n"
      "#include <ostream>\n"
      "#include <string>\n\n"
      "struct " +
      ExceptionName + " : public std::exception {\n" + util::getIndent(1) +
      ExceptionName + "(const std::string Src) {}\n" + util::getIndent(1) +
      "template <typename T>\n" + util::getIndent(1) + ExceptionName +
      " &operator<<(const T Rhs) { " + "return *this; }\n" +
      util::getIndent(1) + ExceptionName + " &operator<<(" +
      "std::ostream& (&Src)(std::ostream&)) { return *this; }\n"
      "};\n\n"
      "#undef GTEST_PRED_FORMAT2_\n"
      "#define THROW_AUTOFUZZ_EXCEPTION(Msg) throw " +
      ExceptionName +
      "(Msg)\n"
      "#define GTEST_PRED_FORMAT2_(pred_format, v1, v2, on_failure) \\\n" +
      util::getIndent(1) + "GTEST_ASSERT_(\\\n" + util::getIndent(2) +
      "pred_format(#v1, #v2, v1, v2), "
      "THROW_AUTOFUZZ_EXCEPTION)\n\n"
      "extern \"C\" {\n"
      "#endif // __cplusplus\n"
      "void " +
      UTEntryFuncName +
      "();\n"
      "#ifdef __cplusplus\n"
      "}\n"
      "#endif // __cplusplus\n"
      "#endif // " +
      HeaderGuard + "\n";
  auto HeaderPath = (fs::path(BasePath) / fs::path(HeaderName)).string();
  return FileNewMap.emplace(HeaderPath, Content).second;
}

std::string UTModify::generateHeaderInclusion(
    const std::string Path, const std::string &UTPath,
    const SourceAnalysisReport &SourceReport) const {
  auto IncludedHeaders = SourceReport.getIncludedHeaders(Path);
  if (Path == UTPath)
    IncludedHeaders.emplace_back("\"" + HeaderName + "\"");
  std::string Result;
  for (const auto &IncludedHeader : IncludedHeaders)
    Result += ("#include " + IncludedHeader + "\n");
  return Result;
}

std::string UTModify::generateIdentifier(const Definition &Def) const {
  if (Def.Declaration == Definition::DeclType_Global) {
    auto Identifier = Def.VarName;
    if (!Def.Namespace.empty())
      Identifier = Def.Namespace + Identifier;
    return Identifier;
  }

  if (Def.Declaration == Definition::DeclType_None)
    return "";

  return Def.VarName;
}

void UTModify::generateMainDeletion(
    std::vector<clang::tooling::Replacement> &Replace,
    const SourceAnalysisReport &SourceReport) const {
  const auto &MainFuncLoc = SourceReport.getMainFuncLoc();
  if (MainFuncLoc.getFilePath().empty())
    return;
  Replace.emplace_back(MainFuncLoc.getFilePath(), MainFuncLoc.getOffset(),
                       MainFuncLoc.getLength(), "");
}

void UTModify::generateTop(
    std::vector<Replacement> &Replace,
    const std::map<std::string, std::string> &FuzzVarDecls,
    const std::map<std::string, std::vector<std::string>> &FuzzVarFlagDecls,
    const std::map<std::string, std::vector<GlobalVarSetter>> &Setters,
    const std::string &UTPath, const SourceAnalysisReport &SourceReport) const {
  const std::string FlagType = "unsigned int";
  for (auto Iter : FuzzVarDecls) {
    auto Path = Iter.first;
    auto Content = generateHeaderInclusion(Path, UTPath, SourceReport);
    Content += CPPMacroStart + Iter.second;
    if (Path == UTPath) {
      for (const auto &Iter2 : FuzzVarFlagDecls)
        for (const auto &Iter3 : Iter2.second)
          Content += FlagType + " " + Iter3 + " = 1;\n";

      for (const auto &Iter2 : Setters)
        for (const auto &Iter3 : Iter2.second)
          Content += "void " + Iter3.FuncName + "();\n";
    } else {
      auto FuzzVarFlagDeclsIter = FuzzVarFlagDecls.find(Path);
      if (FuzzVarFlagDeclsIter != FuzzVarFlagDecls.end())
        for (const auto &Iter2 : FuzzVarFlagDeclsIter->second)
          Content += "extern " + FlagType + " " + Iter2 + ";\n";
    }
    Content += CPPMacroEnd;
    Replace.emplace_back(Path, 0, 0, Content);
  }
}

}; // end of namespace ftg
