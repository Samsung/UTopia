#ifndef FTG_UTANALYSIS_LOCATION_H
#define FTG_UTANALYSIS_LOCATION_H

#include "clang/AST/AST.h"

namespace Json {

class Value;

}

namespace ftg {

class Location {
public:
  Location() : m_offset(0), m_length(0), m_line(0) {}
  Location(Json::Value root);

  Location(const clang::SourceManager &SM, const clang::Stmt &node);
  Location(unsigned offset, unsigned length, unsigned line);
  Location(unsigned offset, unsigned length, unsigned line,
           std::string filePath);
  Location(const clang::SourceManager &SM, const clang::SourceRange srcRange);

  void setLocation(const clang::SourceManager &SM, const clang::Stmt &node);
  void setLocation(const clang::SourceManager &SM,
                   const clang::SourceRange srcRange);
  void setLocation(Json::Value root);
  void setFileEndLoc(clang::FunctionDecl *functionDecl);
  void setFullLocationFromFunctionDecl(const clang::FunctionDecl *functionDecl);

  std::string getFilePath() const { return m_filePath; }

  void setOffset(unsigned offset) { m_offset = offset; }

  void setLength(unsigned length) { m_length = length; }

  void setLine(unsigned line) { m_line = line; }

  void setFilePath(std::string filePath) { m_filePath = filePath; }

  size_t getOffset() const { return m_offset; }

  size_t getLength() const { return m_length; }

  size_t getLine() const { return m_line; }

  Json::Value getJsonValue() const;
  void setValueFromJson(Json::Value root);

private:
  static unsigned getRangeSize(const clang::SourceManager &Sources,
                               const clang::CharSourceRange &Range);
  size_t m_offset;
  size_t m_length;
  size_t m_line;
  std::string m_filePath;
};

} // namespace ftg

#endif // FTG_UTANALYSIS_LOCATION_H
