#include "ftg/utils/StringUtil.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/Support/raw_ostream.h"

#include <assert.h>
#include <cxxabi.h>
#include <iostream>
#include <regex>
#include <sys/types.h>

namespace ftg {

namespace util {

std::string regex(std::string str, std::string regexStr, unsigned group) {
  try {
    std::regex re(regexStr);
    std::smatch match;
    if (std::regex_match(str, match, re)) {
      assert(group < match.size() && "Out of regex group size!");
      return match[group].str();
    } else {
      return "";
    }
  } catch (std::regex_error &e) {
    std::cout << "regex Exception : " << e.code() << std::endl;
    assert("Can't use regex class");
  }
  return "";
}

std::string demangle(const char *name) {
  int status = -1;
  std::unique_ptr<char, void (*)(void *)> res{
      abi::__cxa_demangle(name, nullptr, nullptr, &status), std::free};
  return (status == 0) ? res.get() : std::string(name);
}

std::string getRelativePath(std::string path, std::string projectBuildPath) {
  return regex(path, "^" + projectBuildPath + "/?(.+)", 1);
}

std::string replaceString(std::string OriginalStr, std::string FromStr,
                          std::string ToStr) {
  auto replaceIdx = OriginalStr.find(FromStr);
  if (replaceIdx == std::string::npos)
    return OriginalStr;
  else {
    return OriginalStr.replace(replaceIdx, FromStr.size(), ToStr);
  }
}

std::string getDemangledName(std::string MangledName) {

  const std::string AnonymousNS = "(anonymous namespace)::";

  std::string DemangledName = llvm::demangle(MangledName);
  DemangledName = replaceString(DemangledName, "()", "");

  if (DemangledName.find(AnonymousNS) == 0)
    DemangledName = DemangledName.replace(0, AnonymousNS.size() - 2, "");

  replaceStrAll(DemangledName, "(anonymous namespace)::", "");
  return DemangledName;
}

bool isHeaderFile(std::string path) {

  try {
    std::regex path_regex("\\.h$");
    std::sregex_iterator it(path.begin(), path.end(), path_regex);
    std::sregex_iterator end;
    if (it != end) {
      return true;
    }
    return false;
  } catch (std::regex_error &E) {
    return false;
  }
}

// TOBE : handle anonymous namespace
std::string getClassNameWithNamespace(std::string FullMethodName) {
  std::vector<std::string> Tokens = util::split(FullMethodName, "::");
  if (Tokens.size() == 1)
    return "";
  std::string ClassName;
  int i;
  for (i = 0; i < (signed)Tokens.size() - 2; i++) {
    ClassName += Tokens[i] + "::";
  }
  ClassName += Tokens[i];
  return ClassName;
}

std::string getMethodName(std::string FullMethodName) {
  std::vector<std::string> Tokens = util::split(FullMethodName, "::");
  return Tokens.back();
}

std::string getIndent(unsigned count) {
  std::string ReturnIndent = "";
  while (count--) {
    ReturnIndent += INDENT;
  }
  return ReturnIndent;
}

bool isValidIdentifier(const std::string &Name) {
  std::regex Pattern("^[a-zA-Z_][a-zA-Z0-9_]*$");
  return std::regex_match(Name, Pattern);
}

Json::Value strToJson(std::string JsonStr) {
  Json::Value Json;
  std::istringstream Iss(JsonStr);
  Json::CharReaderBuilder Reader;
  Json::parseFromStream(Reader, Iss, &Json, nullptr);
  return Json;
}

} // namespace util

} // namespace ftg
