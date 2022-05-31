#ifndef MACROPROFILE_H_

#include <llvm/IR/Value.h>
#include <set>

namespace ftg {

class MacroProfile {
public:
  struct RDIndex {
    llvm::Value *V;
    int OpIdx;

    RDIndex(llvm::Value &V, int OpIdx);
    bool operator<(const RDIndex &RHS) const;
  };

  unsigned TotalAPICall;
  unsigned APICallInMacro;
  unsigned MacroIndexSize;
  std::set<RDIndex> TotalDefs;
  std::set<RDIndex> DefsInMacro;

  static MacroProfile &get();

  template <typename T> friend T &operator<<(T &O, const MacroProfile &RHS) {
    O << "= [ Macro Profile ] ================\n";
    O << "| APICall: " << RHS.APICallInMacro << " / " << RHS.TotalAPICall
      << "\n";
    O << "| Def: " << RHS.DefsInMacro.size() << " / " << RHS.TotalDefs.size()
      << "\n";
    O << "| MacroIndex: " << RHS.MacroIndexSize << "\n";
    O << "====================================\n";
    return O;
  }

private:
  MacroProfile();

  static MacroProfile *INSTANCE;
};

} // end namespace ftg

#endif
// MACROPROFILE_H_
