#include "ftg/utanalysis/Location.h"
#include "ftg/utils/json/json.h"
#include "clang/Lex/Lexer.h"

namespace ftg {

Location::Location(Json::Value root) {
  m_offset = root["Offset"].asUInt();
  m_length = root["Length"].asUInt();
  m_line = root["Line"].asInt();
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
  m_filePath = SM.getFileEntryForID(SM.getMainFileID())->getName();
}

void Location::setLocation(Json::Value root) {
  m_offset = root["Offset"].asUInt();
  m_length = root["Length"].asUInt();
  m_line = root["Line"].asUInt();
}

void Location::setFullLocationFromFunctionDecl(
    const clang::FunctionDecl *functionDecl) {
  clang::ASTContext *TUDeclContext = &functionDecl->getASTContext();
  clang::SourceManager &SM = TUDeclContext->getSourceManager();

  if (!functionDecl->getBody()) {
    clang::SourceRange funcRange(functionDecl->getBeginLoc(),
                                 functionDecl->getEndLoc());
    setLocation(SM, funcRange);
  } else {
    clang::SourceRange funcRange(functionDecl->getBeginLoc(),
                                 functionDecl->getBody()->getEndLoc());
    setLocation(SM, funcRange);
  }
}

// // Ref : lib/Tooling/Core/Replacement.cpp#L122
int Location::getRangeSize(const clang::SourceManager &Sources,
                           const clang::CharSourceRange &Range) {
  const clang::LangOptions &LangOpts = clang::LangOptions();
  clang::SourceLocation SpellingBegin =
      Sources.getSpellingLoc(Range.getBegin());
  clang::SourceLocation SpellingEnd = Sources.getSpellingLoc(Range.getEnd());
  std::pair<clang::FileID, unsigned> Start =
      Sources.getDecomposedLoc(SpellingBegin);
  std::pair<clang::FileID, unsigned> End =
      Sources.getDecomposedLoc(SpellingEnd);
  if (Start.first != End.first)
    return -1;
  if (Range.isTokenRange())
    End.second +=
        clang::Lexer::MeasureTokenLength(SpellingEnd, Sources, LangOpts);
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
