#include "ftg/tcanalysis/GoogleTestCallWriter.h"
#include "ftg/utils/StringUtil.h"

using namespace ftg;

std::string GoogleTestCallWriter::getTCCall(const Unittest &TC,
                                            std::string Indent) const {
  const auto *TCTestBody = TC.getTestBody();
  if (!TCTestBody)
    return "";

  auto GtestClassName = TCTestBody->getClassName();
  if (GtestClassName.empty())
    return "";

  std::string TestInstanceName = "Fuzzer";
  std::map<std::string, std::string> ClassMap; // ClassName:InstanceName

  for (const auto &TestFunc : TC.getTestSequence()) {
    if (!TestFunc.isEnvironment() || TestFunc.getClassName().empty())
      continue;
    auto ClassName = TestFunc.getClassName();
    ClassMap[ClassName] = getInstanceNameFromClass(ClassName);
  }
  ClassMap[FuzzGtestClassName] = TestInstanceName;

  auto Result =
      getGtestFuzzClassDeclString(GtestClassName, TC.getTestSequence(), Indent);

  for (auto ClassDefInfo : ClassMap)
    Result += (Indent + ClassDefInfo.first + " " + ClassDefInfo.second + ";\n");

  bool CallRunTestFlag = false;
  for (const auto &TestFunc : TC.getTestSequence()) {
    if (TestFunc.isEnvironment()) {
      Result += (Indent + ClassMap[TestFunc.getClassName()] + "." +
                 TestFunc.getFunctionName() + "();\n");
    } else if (!CallRunTestFlag) {
      CallRunTestFlag = true;
      Result +=
          (Indent + TestInstanceName + "." + FuzzGtestMethodName + "();\n");
    }
  }

  return Result;
}

std::string GoogleTestCallWriter::getGtestFuzzClassDeclString(
    const std::string &ParentClassName,
    const std::vector<FunctionNode> &TestSequence,
    const std::string &Indent) const {
  std::string ClassDefinition = (Indent + "class " + FuzzGtestClassName +
                                 " : public " + ParentClassName + " {\n");
  ClassDefinition += (Indent + "public:\n");
  ClassDefinition +=
      (Indent + util::getIndent(1) + "void " + FuzzGtestMethodName + "() {\n");
  for (const auto &TestFunc : TestSequence) {
    if (TestFunc.isEnvironment())
      continue;
    ClassDefinition +=
        (Indent + util::getIndent(2) + "try {\n" + Indent + util::getIndent(3) +
         TestFunc.getFunctionName() + "();\n" + Indent + util::getIndent(2) +
         "} catch (std::exception &E) {}\n");
  }
  ClassDefinition += (Indent + util::getIndent(1) + "}\n");
  ClassDefinition += (Indent + "};\n");
  return ClassDefinition;
}

std::string GoogleTestCallWriter::getInstanceNameFromClass(
    const std::string &ClassName) const {
  std::vector<std::string> ClassNameTokens = util::split(ClassName, "::");
  return "AUTOFUZZ" + ClassNameTokens.back();
}
