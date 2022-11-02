#include "ftg/generation/UTModify.h"
#include "ftg/tcanalysis/TCAnalyzerFactory.h"
#include "ftg/utils/AssignUtil.h"
#include "ftg/utils/FileUtil.h"

#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <experimental/filesystem>

using namespace llvm;
using namespace clang::tooling;
namespace fs = std::experimental::filesystem;

namespace ftg {

const std::string UTModify::HeaderName = "autofuzz.h";
const std::string UTModify::UTEntryFuncName = "enterAutofuzz";

UTModify::UTModify(const Fuzzer &F, const SourceAnalysisReport &SourceReport) {
  clang::IntrusiveRefCntPtr<llvm::vfs::OverlayFileSystem> OverlayFS =
      new llvm::vfs::OverlayFileSystem(llvm::vfs::getRealFileSystem());
  FManager = new clang::FileManager(clang::FileSystemOptions(), OverlayFS);

  clang::IntrusiveRefCntPtr<clang::DiagnosticOptions> DiagOpts =
      new clang::DiagnosticOptions();
  auto DiagnosticPrinter = std::make_unique<clang::TextDiagnosticPrinter>(
      llvm::outs(), DiagOpts.get());
  clang::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID =
      new clang::DiagnosticIDs();
  auto Diagnostics = std::make_unique<clang::DiagnosticsEngine>(
      DiagID, DiagOpts.get(), DiagnosticPrinter.get(), false);
  SManager = std::make_unique<clang::SourceManager>(*Diagnostics, *FManager);

  std::map<std::string, std::string> FuzzVarDecls;
  std::map<std::string, std::vector<std::string>> FuzzVarFlagDecls;
  std::map<std::string, std::vector<UTModify::GlobalVarSetter>> Setters;
  auto UTPath = F.getUT().getFilePath();
  for (const auto &Iter : F.getFuzzInputMap()) {
    auto &Input = Iter.second;
    assert(Input && "Unexpected Program State");
    if (!Input->getCopyFrom()) {
      generateFuzzVarDeclarations(FuzzVarDecls, FuzzVarFlagDecls, *Input);
      generateFuzzVarGlobalSetters(Setters, *Input);
    }
    generateFuzzVarReplacements(*Input);
  }
  generateTop(FuzzVarDecls, FuzzVarFlagDecls, Setters, UTPath, SourceReport);
  generateBottom(FuzzVarFlagDecls, Setters, F.getUT(), SourceReport);
  if (!generateHeader(fs::path(UTPath).parent_path().string())) {
    clear();
    return;
  }
  generateMainDeletion(SourceReport);

  for (const auto &Iter : Replaces) {
    auto Replace = Iter.second;
    if (FileReplaceMap[Replace.getFilePath().str()].add(Replace)) {
      clear();
      return;
    }
  }
}

bool UTModify::addReplace(const clang::tooling::Replacement &Replace) {
  auto Key = std::make_pair(Replace.getFilePath().str(), Replace.getOffset());
  const auto [It, Inserted] = Replaces.insert(std::make_pair(Key, Replace));
  if (Inserted)
    return true;
  auto Existing = It->second;
  if (Existing.getLength() && Replace.getLength()) {
    assert(false && "Not Mergeable Replacements Found");
    return false;
  }
  auto Merged = Replacement(Existing.getFilePath(), Existing.getOffset(),
                            Existing.getLength() + Replace.getLength(),
                            Existing.getReplacementText().str() +
                                Replace.getReplacementText().str());
  Replaces.insert_or_assign(Key, Merged);
  return true;
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
  Replaces.clear();
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
    const std::map<std::string, std::vector<std::string>> &FuzzVarFlagDecls,
    const std::map<std::string, std::vector<GlobalVarSetter>> &Setters,
    const Unittest &UT, const SourceAnalysisReport &SourceReport) {
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
      Content = "\n" + CPPMacroStart + Content + CPPMacroEnd;
    addReplace(Replacement(Path, EndOffset, 0, Content));
  }
}

std::string UTModify::generateDeclTypeName(const Type &T) const {
  std::string TypeName = "";
  if (T.isPointerType()) {
    const auto *PointerT = &T;
    const auto *PointeeT = PointerT->getPointeeType();

    while (PointerT != PointeeT) {
      TypeName += "*";
      PointerT = PointeeT;
      PointeeT = PointerT->getPointeeType();
    }
    assert(PointeeT && "Unexpected Program State");

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

void UTModify::generateFuzzVarReplacements(const FuzzInput &Input) {
  const auto &Def = Input.getDef();
  const auto &T = Def.DataType;
  assert(T && "Unexpected Program State");

  if (Def.Declaration != Definition::DeclType_None) {
    auto TypeString = util::trim(Def.TypeString);
    auto Stripped = util::stripConstExpr(TypeString);
    if (Stripped != TypeString) {
      addReplace(Replacement(Def.Path, Def.TypeOffset,
                             static_cast<unsigned int>(Def.TypeString.size()),
                             Stripped + " "));
    }
  }

  if (Def.Declaration == Definition::DeclType_Global)
    return;

  const auto *CopyFrom = Input.getCopyFrom();
  const auto *AssignBase = CopyFrom ? CopyFrom : &Input;
  auto AssignStmt = generateAssignStatement(*AssignBase);
  if (Def.Declaration == Definition::DeclType_StaticLocal) {
    auto FlagVarName = util::getStaticLocalFlagVarName(Input.getFuzzVarName());
    AssignStmt = " if (" + FlagVarName + ") { " + FlagVarName + " = 0; " +
                 AssignStmt + " }";
    addReplace(Replacement(Def.Path, Def.EndOffset, 0, AssignStmt));
    return;
  }

  if (T->isFixedLengthArrayPtr()) {
    addReplace(Replacement(Def.Path, Def.EndOffset, 0, " " + AssignStmt));
    return;
  }

  addReplace(Replacement(Def.Path, Def.Offset, Def.Length, AssignStmt));
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
      " \"\"\n"
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
      "// Redefine access modifier to public\n"
      "// to access private/protected Testbody or Setup/Teardown\n"
      "#define private public\n"
      "#define protected public\n"
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

void UTModify::generateMainDeletion(const SourceAnalysisReport &SourceReport) {
  const auto &MainFuncLoc = SourceReport.getMainFuncLoc();
  if (MainFuncLoc.getFilePath().empty())
    return;
  addReplace(Replacement(MainFuncLoc.getFilePath(), MainFuncLoc.getOffset(),
                         MainFuncLoc.getLength(), ""));
}

void UTModify::generateTop(
    const std::map<std::string, std::string> &FuzzVarDecls,
    const std::map<std::string, std::vector<std::string>> &FuzzVarFlagDecls,
    const std::map<std::string, std::vector<GlobalVarSetter>> &Setters,
    const std::string &UTPath, const SourceAnalysisReport &SourceReport) {
  const std::string FlagType = "unsigned int";
  // NOTE: UTPath should be inlcuded in filepaths even if it does not have
  // any fuzzable inputs, since ut path should include definition of static
  // flags or declaration of global setter functions to link them all.
  std::set<std::string> Paths = {UTPath};
  for (auto Iter : FuzzVarDecls)
    Paths.emplace(Iter.first);

  for (const auto &Path : Paths) {
    auto Iter = FuzzVarDecls.find(Path);
    std::string FuzzVarDeclStatements;
    assert((Iter != FuzzVarDecls.end() || Path == UTPath) &&
           "Unexpected Program State");

    if (Iter != FuzzVarDecls.end())
      FuzzVarDeclStatements = Iter->second;

    auto Content = generateHeaderInclusion(Path, UTPath, SourceReport);
    auto ContentInCppMacro = FuzzVarDeclStatements;
    if (Path == UTPath) {
      for (const auto &Iter2 : FuzzVarFlagDecls)
        for (const auto &Iter3 : Iter2.second)
          ContentInCppMacro += FlagType + " " + Iter3 + " = 1;\n";

      for (const auto &Iter2 : Setters)
        for (const auto &Iter3 : Iter2.second)
          ContentInCppMacro += "void " + Iter3.FuncName + "();\n";
    } else {
      auto FuzzVarFlagDeclsIter = FuzzVarFlagDecls.find(Path);
      if (FuzzVarFlagDeclsIter != FuzzVarFlagDecls.end())
        for (const auto &Iter2 : FuzzVarFlagDeclsIter->second)
          ContentInCppMacro += "extern " + FlagType + " " + Iter2 + ";\n";
    }
    if (!ContentInCppMacro.empty())
      Content += CPPMacroStart + ContentInCppMacro + CPPMacroEnd;
    addReplace(Replacement(Path, 0, 0, Content));
  }
}

void UTModify::genModifiedSrcs(const std::string &OrgSrcDir,
                               const std::string &OutDir,
                               const std::string &Signature) const {
  for (auto Iter : FileReplaceMap) {
    auto FilePath = Iter.first;
    assert(!FilePath.empty() && "Unexpected Program State");
    auto Replacements = Iter.second;

    auto OutPath = util::rebasePath(FilePath, OrgSrcDir, OutDir);
    applyReplacements(FilePath, OutPath, Replacements, Signature);
  }

  for (auto Iter : FileNewMap) {
    auto FilePath = Iter.first;
    assert(!FilePath.empty() && "Unexpected Program State");
    auto Content = Signature + Iter.second;

    auto OutPath = util::rebasePath(FilePath, OrgSrcDir, OutDir);
    util::saveFile(OutPath.c_str(), Content.c_str());
  }
}

void UTModify::applyReplacements(
    const std::string &OrgFilePath, const std::string &OutFilePath,
    const clang::tooling::Replacements &Replacements,
    const std::string &Signature) const {
  auto ID = SManager->getOrCreateFileID(FManager->getFile(OrgFilePath).get(),
                                        clang::SrcMgr::C_User);
  clang::Rewriter Rewrite(*SManager, clang::LangOptions());
  clang::tooling::applyAllReplacements(Replacements, Rewrite);
  std::error_code EC;
  llvm::raw_fd_ostream OutFile(OutFilePath, EC, llvm::sys::fs::F_None);
  Rewrite.InsertTextBefore(SManager->getLocForStartOfFile(ID), Signature);
  Rewrite.getEditBuffer(ID).write(OutFile);
}

}; // end of namespace ftg
