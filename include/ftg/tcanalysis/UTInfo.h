#ifndef FTG_TCANALYSIS_UTINFO_H
#define FTG_TCANALYSIS_UTINFO_H

#include "clang/AST/Decl.h"

namespace ftg {

class Testcase {
public:
  Testcase(std::string UTName);
  std::string getCleanupName() const;
  std::string getStartupName() const;
  std::string getUTName() const;
  void setCleanupName(std::string CleanupName);
  void setStartupName(std::string startupName);

private:
  std::string UTName;
  std::string StartupName;
  std::string CleanupName;
};

class TCArray {
public:
  void addTestcase(std::string UTName);
  Testcase *getCurTestcase();
  Testcase *getTestcase(std::string UTName);
  const Testcase *getTestcase(std::string UTName) const;

private:
  std::map<std::string, Testcase> TCMap;
  std::string RecentTC;
};

class UTFuncBody {
public:
  static bool isUT(std::string functionName);
  UTFuncBody(const clang::FunctionDecl &FD);
  std::string getCleanupName() const;
  const clang::FunctionDecl &getFunctionDecl() const;
  std::string getName() const;
  std::string getStartupName() const;
  void setCleanupName(std::string cleanupName);
  void setStartupName(std::string startupName);

private:
  std::string CleanupName;
  const clang::FunctionDecl &FD;
  std::string Name;
  std::string StartupName;
};

class Project {
public:
  void addUTFunc(clang::FunctionDecl *FD);
  const std::vector<UTFuncBody> &getUTFuncs() const;
  TCArray &getTCArray();
  void matchExtraAction();

private:
  TCArray TCArrayObj;
  std::vector<UTFuncBody> UTFuncs;
};

} // namespace ftg

#endif // FTG_TCANALYSIS_UTINFO_H
