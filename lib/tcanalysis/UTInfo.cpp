#include "ftg/tcanalysis/UTInfo.h"
#include <regex>

using namespace ftg;
using namespace llvm;

////////////////////////////////////////////////////////
// Start of Defined Method body of Project class.     //
////////////////////////////////////////////////////////
void Project::addUTFunc(clang::FunctionDecl *FD) {
  if (!FD)
    return;

  UTFuncs.emplace_back(*FD);
}

const std::vector<UTFuncBody> &Project::getUTFuncs() const { return UTFuncs; }

TCArray &Project::getTCArray() { return TCArrayObj; }

void Project::matchExtraAction() {
  for (auto &UT : UTFuncs) {
    auto *TC = TCArrayObj.getTestcase(UT.getName());
    if (!TC)
      continue;

    UT.setStartupName(TC->getStartupName());
    UT.setCleanupName(TC->getCleanupName());
  }
}
////////////////////////////////////////////////////////
// Start of Defined Method body of Testcase class.    //
////////////////////////////////////////////////////////
Testcase::Testcase(std::string UTName)
    : UTName(UTName), StartupName(""), CleanupName("") {}

std::string Testcase::getCleanupName() const { return CleanupName; }

std::string Testcase::getStartupName() const { return StartupName; }

std::string Testcase::getUTName() const { return UTName; }

void Testcase::setCleanupName(std::string CleanupName) {
  this->CleanupName = CleanupName;
}

void Testcase::setStartupName(std::string StartupName) {
  this->StartupName = StartupName;
}

////////////////////////////////////////////////////////
// Start of Defined Method body of TCArray class.     //
////////////////////////////////////////////////////////
void TCArray::addTestcase(std::string UTName) {
  TCMap.emplace(UTName, Testcase(UTName));
  RecentTC = UTName;
}

Testcase *TCArray::getCurTestcase() { return getTestcase(RecentTC); }

Testcase *TCArray::getTestcase(std::string UTName) {
  auto Iter = TCMap.find(UTName);
  if (Iter == TCMap.end())
    return nullptr;

  return &Iter->second;
}

const Testcase *TCArray::getTestcase(std::string UTName) const {
  auto Iter = TCMap.find(UTName);
  if (Iter == TCMap.end())
    return nullptr;

  return &Iter->second;
}

////////////////////////////////////////////////////////
// Start of Defined Method body of UTFuncBody class.  //
////////////////////////////////////////////////////////
bool UTFuncBody::isUT(std::string Name) {
  // UTFuncBody Convention :
  //     utc_<MODULE_NAME>_<SubroutineCall_NAME>_[p|n][0-9]*$
  // std::regex UTFuncBodyreg("^utc_[a-z|A-Z|0-9|_]*_[p|n][0-9|_]*$");
  // UTFuncBody Rough Naming Convention
  std::regex UTFuncBodyRegex("^utc_[a-z|A-Z|0-9|_]+");
  std::smatch Regex;
  return std::regex_search(Name, Regex, UTFuncBodyRegex);
}

UTFuncBody::UTFuncBody(const clang::FunctionDecl &FD)
    : FD(FD), Name(FD.getNameAsString()) {}

std::string UTFuncBody::getCleanupName() const { return CleanupName; }

const clang::FunctionDecl &UTFuncBody::getFunctionDecl() const { return FD; }

std::string UTFuncBody::getName() const { return Name; }

std::string UTFuncBody::getStartupName() const { return StartupName; }

void UTFuncBody::setCleanupName(std::string CleanupName) {
  this->CleanupName = CleanupName;
}

void UTFuncBody::setStartupName(std::string StartupName) {
  this->StartupName = StartupName;
}
