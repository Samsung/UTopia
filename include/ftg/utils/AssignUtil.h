#ifndef FTG_UTILS_ASSIGNUTIL_H
#define FTG_UTILS_ASSIGNUTIL_H

#include "ftg/utils/StringUtil.h"
#include <string>

namespace ftg {

namespace util {

#define APIARG_VARNAME std::string("autofuzz_mutation")
#define AVASFUZZ_VARNAME std::string("autofuzz")
#define PROTO_VARNAME std::string("arg")
#define PROTO_PKGNAME std::string("mutation")

static std::string inline getAVASIncludeString() {
  return "#include \"0avasfuzz.h\" \n";
}

static std::string inline getAvasFuzzVarName(unsigned callIdx,
                                             unsigned argIdx) {
  return AVASFUZZ_VARNAME + std::to_string(callIdx) + "_" +
         std::to_string(argIdx);
}

static std::string inline getAvasFuzzVarName(size_t ID) {
  return AVASFUZZ_VARNAME + std::to_string(ID);
}

static std::string inline getAvasFuzzSizeVarName(std::string AvasFuzzVarName) {
  return AvasFuzzVarName + "size";
}

static std::string inline getProtoVarName(unsigned callIdx, unsigned argIdx) {
  return PROTO_VARNAME + std::to_string(callIdx) + "_" + std::to_string(argIdx);
}

static std::string inline getProtoVarName(size_t ID) {
  return "fuzzvar" + std::to_string(ID);
}

static std::string inline getProtoVarStr(std::string ProtoVarName) {
  return APIARG_VARNAME + "." + ProtoVarName + "()";
}

static std::string inline getProtoVarSizeStr(std::string ProtoVarName) {
  return getProtoVarStr(ProtoVarName) + ".size()";
}

static std::string inline getAssignGlobalFuncName(std::string VarName) {
  return "assign_fuzz_input_to_global_" + VarName;
}

static std::string inline getStaticLocalFlagVarName(std::string VarName) {
  return VarName + "_flag";
}

static std::string inline getFixedLengthArrayAssignStmt(std::string LHS,
                                                        std::string RHS) {
  std::string SizeStr = util::getAvasFuzzSizeVarName(RHS);
  return "{ for (unsigned i=0; i<" + SizeStr + "; ++i) { " + LHS +
         "[i] = " + RHS + "[i]; } }";
}

static std::string inline wrapWithExternCStmt(std::string Str) {
  std::string Prefix = "#ifdef __cplusplus\nextern \"C\"\n{\n#endif\n";
  std::string Postfix = "\n#ifdef __cplusplus\n}\n#endif\n";
  return Prefix + Str + Postfix;
}

static std::string inline getAvasFuzzVarType(std::string typeStr,
                                             size_t formalPtrDepth,
                                             size_t actualPtrDepth) {
  std::string ret = typeStr;
  int diff = formalPtrDepth - actualPtrDepth;
  while (diff) {
    if (diff > 0) {
      ret = util::regex(ret, "(.+)\\*$", 1);
      diff--;
    } else {
      ret += "*";
      diff++;
    }
  }
  return ret;
}

static std::string inline stripPtrExpr(std::string str) {
  size_t ptrExprOffset = str.find_first_of('*');
  if (ptrExprOffset != std::string::npos) {
    str = str.substr(0, str.find_first_of('*') - 1);
    str = trim(str);
  }
  return str;
}

static std::string inline stripConstExpr(std::string str) {
  std::string ret;
  for (const std::string &s : util::split(str, " ")) {
    if (s == "const" || s == "constexpr")
      continue;
    else if (s == "*const" || s == "const*")
      ret += '*';
    else
      ret += s + ' ';
  }
  return util::trim(ret);
}

static std::string inline getStaticCast(std::string castStr,
                                        std::string varStr) {
  return "static_cast<" + castStr + ">(" + varStr + ")";
}

static std::string inline getConstCast(std::string castStr,
                                       std::string varStr) {
  return "const_cast<" + castStr + ">(" + varStr + ")";
}

static std::string changeStaticArrayToPtr(std::string str) {
  std::string ret = regex(str, "(.+)\\[.*\\]", 1);
  return ret.empty() ? str : ret + "*";
}

static std::string inline getFuzzInputTypeString(std::string str) {
  return changeStaticArrayToPtr(stripConstExpr(str));
}

} // namespace util

} // namespace ftg

#endif // FTG_UTILS_ASSIGNUTIL_H
