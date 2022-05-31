#ifndef FTG_UTILS_MANUALALLOCLOADER_H
#define FTG_UTILS_MANUALALLOCLOADER_H

#include "llvm/IR/Function.h"
#include <set>

namespace ftg {

class ManualAllocLoader {

public:
  bool isAllocFunction(const llvm::Function &F) const;
  std::set<unsigned> getAllocSizeArgNo(const llvm::Function &F) const;
  std::set<unsigned> getAllocSizeArgNo(std::string Name) const;

private:
  struct AllocSizeArg {
    std::string Name;
    std::set<unsigned> AllocSizeArgNo;
  };
  const std::vector<AllocSizeArg> FuncList = {{"malloc", {0}},
                                              {"calloc", {0, 1}},
                                              {"realloc", {1}},
                                              {"operator new", {0}},
                                              {"::basic_string(", {2}}};
  const std::vector<AllocSizeArg> StringList = {
      {"::basic_string(unsigned long,", {1}},
      {"::basic_string(unsigned int,", {1}},
      {"unsigned long, unsigned long)", {3}},
      {"unsigned int, unsigned int)", {3}},
      {"const*, unsigned long, std::allocator<", {2}},
      {"const*, unsigned int, std::allocator<", {2}}};
};

} // namespace ftg

#endif // FTG_UTILS_MANUALALLOCLOADER_H
