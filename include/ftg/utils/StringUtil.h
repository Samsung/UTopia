#ifndef FTG_UTILS_STRINGUTIL_H
#define FTG_UTILS_STRINGUTIL_H

#include <algorithm>
#include <string>
#include <vector>

namespace ftg {

namespace util {

std::string regex(std::string str, std::string regexStr, unsigned group = 0);

static inline void replaceStrOnce(std::string &str, const std::string &from,
                                  const std::string &to) {
  size_t start_pos = 0;
  if ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
  }
}

static inline void replaceStrAll(std::string &str, const std::string &from,
                                 const std::string &to) {
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos +=
        to.length(); // Handles case where 'to' is a substring of 'from'
  }
}

// trim from start
static inline std::string ltrim(std::string s) {
  s.erase(s.begin(),
          std::find_if(s.begin(), s.end(),
                       std::not1(std::ptr_fun<int, int>(std::isspace))));
  return s;
}

// trim from end
static inline std::string rtrim(std::string s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       std::not1(std::ptr_fun<int, int>(std::isspace)))
              .base(),
          s.end());
  return s;
}

// trim from both ends
static inline std::string trim(std::string s) {
  s = ltrim(s);
  s = rtrim(s);
  return s;
}

std::string demangle(const char *name);

std::string getRelativePath(std::string path, std::string projectBuildPath);

std::string getDemangledName(std::string MangledName);

bool isHeaderFile(std::string path);

static inline std::vector<std::string> split(const std::string &Data,
                                             const std::string &Delimiter) {
  std::vector<std::string> Tokens;
  std::string Token;

  size_t StartPos = 0;
  size_t EndPos;
  size_t DelimiterLen = Delimiter.length();

  while ((EndPos = Data.find(Delimiter, StartPos)) != std::string::npos) {
    Token = Data.substr(StartPos, EndPos - StartPos);
    StartPos = EndPos + DelimiterLen;
    Tokens.push_back(Token);
  }
  Tokens.push_back(Data.substr(StartPos));

  return Tokens;
}

// In C++ case, method name consist of namespace, class and method name.
// ex) namespace1::namespace2::Class::Method
// return class name with namespace except method name.
std::string getClassNameWithNamespace(std::string FullMethodName);

// In C++ case, method name consist of namespace, class and method name.
// return method name only.
std::string getMethodName(std::string FullMethodName);

std::string getIndent(unsigned count = 1);

/// Validate if name is valid cpp identifier
/// \return true if Name is valid cpp identifier else false
bool isValidIdentifier(const std::string &Name);

const std::string INDENT = "  ";

} // namespace util

} // namespace ftg

#endif // FTG_UTILS_STRINGUTIL_H
