#include "ftg/tcanalysis/BoostCallWriter.h"
#include "ftg/utils/StringUtil.h"

using namespace ftg;

std::string BoostCallWriter::getTCCall(const Unittest &TC,
                                       std::string Indent) const {
  const std::string InstanceName = "Fuzzer";

  const auto *TCTestBody = TC.getTestBody();
  if (!TCTestBody)
    return "";

  auto StructName = TCTestBody->getClassName();
  if (StructName.empty())
    return "";

  return Indent + "try {\n" + Indent + util::getIndent(1) + "struct " +
         StructName + " " + InstanceName + ";\n" + Indent + util::getIndent(1) +
         InstanceName + ".test_method();\n" + Indent +
         "} catch (std::exception &E) {}\n";
}
