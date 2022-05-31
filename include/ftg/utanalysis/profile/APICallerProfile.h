#ifndef APICALLERPROFILE_H
#define APICALLERPROFILE_H

#include "astinfo/LocIndex.h"
#include "llvm/IR/Use.h"
#include "llvm/Support/raw_ostream.h"
#include <set>

namespace ftg {

class APICallerProfile {
public:
  APICallerProfile() = default;

  void addUnknown(llvm::Use &Src);
  void addOut(llvm::Use &Src);
  void addNoDef(llvm::Use &Src);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &O,
                                       const APICallerProfile &Src);

private:
  static std::set<llvm::Use *> Unknowns;
  static std::set<llvm::Use *> Outs;
  static std::set<llvm::Use *> NoDefs;

  void printDetailedInfo(llvm::raw_ostream &,
                         const std::set<llvm::Use *> &) const;
  std::tuple<LocIndex, std::string, size_t> getDetailedInfo(llvm::Use &) const;
  std::vector<llvm::Use *> sort(const std::set<llvm::Use *> &Src) const;
  LocIndex getLocIndexFromUse(llvm::Use &) const;
};

} // namespace ftg

#endif // ifndef APICALLERPROFILE_H
