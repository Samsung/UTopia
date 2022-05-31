#ifndef FTG_UTANALYSIS_TARGETAPIFINDER_H
#define FTG_UTANALYSIS_TARGETAPIFINDER_H

#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include <queue>
#include <set>

namespace ftg {

/// @brief TargetAPIFinder provides target callees(API Function) and
/// target callers(CallSites for API function)
/// in a given function sequence(Testcase).
class TargetAPIFinder {

public:
  /// @brief Constructor
  /// @param[in] APIs API functions.
  TargetAPIFinder(std::set<llvm::Function *> APIs);
  /// @brief Find API functions that are callable within
  /// a given function sequence.
  /// @param[in] Funcs Given function sequence.
  /// @return API Functions that are callable within a given function sequence.
  std::set<llvm::Function *> findAPIs(std::vector<llvm::Function *> Funcs);
  /// @brief Find call sites for API Functions within
  /// a given function sequence.
  /// @param[in] Funcs Given function sequence.
  /// @return API Functions that are callable within a given function sequence.
  std::set<llvm::CallBase *>
  findAPICallers(std::vector<llvm::Function *> Funcs);

private:
  /// @brief Target API Functions
  std::set<llvm::Function *> APIs;
  /// @brief Cached functions that were already visited.
  /// This helps to increase performance with caches by reusing previous result
  /// instead of revisiting.
  std::set<llvm::Function *> CachedFunctions;
  /// @brief Cache table for callees of target API functions(values)
  /// that are discoverable in a given function(key).
  std::map<llvm::Function *, std::set<llvm::Function *>> TargetCallees;
  /// @brief Cache table for callers of target API functions(values)
  /// that are discoverable in a given function(key).
  std::map<llvm::Function *, std::set<llvm::CallBase *>> TargetCallers;
  /// @brief Cache table for defined functions that are callable(values) in a
  /// given function(key)
  std::map<llvm::Function *, std::set<llvm::Function *>> Subroutines;

  template <typename T1, typename T2>
  void find(std::vector<llvm::Function *> &Funcs, T1 &Map, T2 &Result);
  void update(llvm::Function &Func);
};

} // namespace ftg

#endif // FTG_UTANALYSIS_TARGETAPIFINDER_H
