#include "ftg/sourceanalysis/SourceAnalyzerImpl.h"
#include "ftg/utils/FileUtil.h"
#include "ftg/utils/StringUtil.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace ftg;
using namespace clang;
using namespace ast_matchers;

SourceAnalyzerImpl::SourceAnalyzerImpl(const SourceCollection &SC) {
  for (const auto *ASTUnit : SC.getASTUnits()) {
    if (!ASTUnit)
      continue;

    auto *MainFD = getMainFuncDecl(*ASTUnit);
    if (MainFD) {
      Location MainFuncLoc;
      MainFuncLoc.setFullLocationFromFunctionDecl(MainFD);

      if (util::getRelativePath(MainFuncLoc.getFilePath(), SC.getBaseDir())
              .empty())
        continue;

      Report.setMainFuncLoc(MainFuncLoc);
    }

    Report.addSourceInfo(
        util::getNormalizedPath(ASTUnit->getOriginalSourceFileName()),
        getEndOffset(*ASTUnit), getIncludes(*ASTUnit));
  }
  Report.setSrcBaseDir(SC.getBaseDir());
}

std::unique_ptr<AnalyzerReport> SourceAnalyzerImpl::getReport() {
  return std::make_unique<SourceAnalysisReport>(Report);
}

const SourceAnalysisReport &SourceAnalyzerImpl::getActualReport() const {
  return Report;
}

unsigned SourceAnalyzerImpl::getEndOffset(const clang::ASTUnit &U) const {
  const auto &SM = U.getSourceManager();
  const auto FileID = SM.getMainFileID();
  const auto SourceLoc = SM.getLocForEndOfFile(FileID);
  return SM.getFileOffset(SourceLoc);
}

std::vector<std::string>
SourceAnalyzerImpl::getIncludes(const ASTUnit &AST) const {
  std::vector<std::string> Includes;
  auto &ASTContext = AST.getASTContext();
  auto &SM = *const_cast<SourceManager *>(&ASTContext.getSourceManager());
  FileID FID = SM.getMainFileID();
  Rewriter TheRewriter;
  TheRewriter.setSourceMgr(SM, ASTContext.getLangOpts());

  auto DumpSLocEntry = [&](const SrcMgr::SLocEntry &Entry) {
    if (!Entry.isFile())
      return;

    auto &FI = Entry.getFile();

    SourceLocation IncludeLoc = FI.getIncludeLoc();
    if (!IncludeLoc.isValid() || !SM.isInFileID(IncludeLoc, FID))
      return;

    int Offset = 0;
    clang::SourceLocation EndLoc;
    while (true) {
      EndLoc = IncludeLoc.getLocWithOffset(Offset);
      std::string Str = TheRewriter.getRewrittenText(SourceRange(EndLoc));
      assert(!Str.empty() && "Unexpected Program State");

      auto Front = Str.front();
      if (Front == '\"' || Front == '>')
        break;

      Offset += Str.size();
    }
    clang::SourceRange headerSourceRange(FI.getIncludeLoc(), EndLoc);
    Includes.push_back(TheRewriter.getRewrittenText(headerSourceRange));
  };

  // Dump local SLocEntries.
  for (unsigned ID = 0, NumIDs = SM.local_sloc_entry_size(); ID != NumIDs; ++ID)
    DumpSLocEntry(SM.getLocalSLocEntry(ID));

  // Dump loaded SLocEntries.
  for (unsigned Index = 0; Index != SM.loaded_sloc_entry_size(); ++Index) {
    if (!SM.getLoadedSLocEntry(Index).isFile())
      continue;

    DumpSLocEntry(SM.getLoadedSLocEntry(Index));
  }

  // The sequence of Included Header List from FileInfo.getIncludeLoc is
  // reverse. So, Reverse again to get correct sequence of the list.
  std::reverse(Includes.begin(), Includes.end());
  return Includes;
}

const clang::FunctionDecl *
SourceAnalyzerImpl::getMainFuncDecl(const clang::ASTUnit &U) const {
  const std::string Tag = "Tag";
  auto Matcher =
      functionDecl(isMain(), unless(isExpansionInSystemHeader())).bind(Tag);
  for (auto &Node :
       match(Matcher, *const_cast<clang::ASTContext *>(&U.getASTContext()))) {
    auto *Record = Node.getNodeAs<FunctionDecl>(Tag);
    if (!Record)
      continue;

    return Record;
  }
  return nullptr;
}
