#include "ftg/utils/Location.h"
#include "ftg/utils/FileUtil.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "json/json.h"

using namespace clang;

namespace ftg {

Location::Location(Json::Value root) {
  m_offset = root["Offset"].asUInt();
  m_length = root["Length"].asUInt();
  m_line = root["Line"].asUInt();
}

Location::Location(const clang::SourceManager &SM, const clang::Stmt &node) {
  setLocation(SM, node.getSourceRange());
}

Location::Location(unsigned offset, unsigned length, unsigned line) {
  setOffset(offset);
  setLength(length);
  setLine(line);
}

Location::Location(unsigned offset, unsigned length, unsigned line,
                   std::string filePath)
    : Location(offset, length, line) {
  setFilePath(filePath);
}

Location::Location(const clang::SourceManager &SM,
                   const clang::SourceRange srcRange) {
  setLocation(SM, srcRange);
}

void Location::setLocation(const clang::SourceManager &SM,
                           const clang::Stmt &node) {
  setLocation(SM, node.getSourceRange());
}

// Ref : lib/Tooling/Core/Replacement.cpp#L120
void Location::setLocation(const clang::SourceManager &SM,
                           const clang::SourceRange srcRange) {
  clang::CharSourceRange srcExpansionSrcrange = SM.getExpansionRange(srcRange);
  const std::pair<clang::FileID, unsigned> DecomposedLocation =
      SM.getDecomposedLoc(srcExpansionSrcrange.getBegin());
  m_offset = DecomposedLocation.second;
  m_length = getRangeSize(SM, srcExpansionSrcrange);
  m_line = SM.getPresumedLoc(srcRange.getBegin()).getLine();
  m_filePath = util::getNormalizedPath(
      SM.getFilename(srcExpansionSrcrange.getBegin()).str());
}

void Location::setLocation(Json::Value root) {
  m_offset = root["Offset"].asUInt();
  m_length = root["Length"].asUInt();
  m_line = root["Line"].asUInt();
}

void Location::setFullLocationFromFunctionDecl(const clang::FunctionDecl *FD) {
  auto &Ctx = FD->getASTContext();
  auto &SM = Ctx.getSourceManager();
  auto BeginLoc = SM.getExpansionLoc(FD->getBeginLoc());
  SourceLocation EndLoc;
  if (!FD->getBody())
    EndLoc = FD->getEndLoc();
  else
    EndLoc = FD->getBody()->getEndLoc();
  EndLoc = SM.getExpansionLoc(EndLoc);
  auto BeginOffset = SM.getDecomposedLoc(BeginLoc).second;
  auto EndOffset = SM.getDecomposedLoc(EndLoc).second;
  assert(BeginOffset <= EndOffset && "Invalid location for main function");

  m_offset = BeginOffset;
  m_length = (EndOffset - BeginOffset +
              Lexer::MeasureTokenLength(EndLoc, SM, LangOptions()));
  m_line = SM.getPresumedLoc(BeginLoc).getLine();
  m_filePath = util::getNormalizedPath(SM.getFilename(BeginLoc).str());
}

// // Ref : lib/Tooling/Core/Replacement.cpp#L122
unsigned Location::getRangeSize(const clang::SourceManager &Sources,
                                const clang::CharSourceRange &Range) {
  const clang::LangOptions &LangOpts = clang::LangOptions();
  clang::SourceLocation SpellingBegin =
      Sources.getSpellingLoc(Range.getBegin());
  clang::SourceLocation SpellingEnd = Sources.getSpellingLoc(Range.getEnd());
  std::pair<clang::FileID, unsigned> Start =
      Sources.getDecomposedLoc(SpellingBegin);
  std::pair<clang::FileID, unsigned> End =
      Sources.getDecomposedLoc(SpellingEnd);
  if (Range.isTokenRange())
    End.second +=
        clang::Lexer::MeasureTokenLength(SpellingEnd, Sources, LangOpts);
  assert(Start.first == End.first && End.second >= Start.second &&
         "Invalid source range");
  return End.second - Start.second;
}

Json::Value Location::getJsonValue() const {
  Json::Value root;
  root["FilePath"] = m_filePath;
  root["Offset"] = m_offset;
  root["Length"] = m_length;
  root["Line"] = m_line;
  return root;
}

} // namespace ftg
