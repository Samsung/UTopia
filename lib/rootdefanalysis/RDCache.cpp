#include "RDCache.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace ftg {

void RDCache::cache(const RDNode &Key, std::set<RDNode> Value) {
  RDNode CopiedKey = Key;
  CopiedKey.clearVisit();
  if (Cache.find(CopiedKey) != Cache.end()) {
    outs() << "Error Key: " << CopiedKey << "\n";
    assert(false && "Unexpected Program State");
  }

  for (auto &V : Value) {
    auto &Node = *const_cast<RDNode *>(&V);
    Node.clearVisit();
  }
  Cache.insert(std::make_pair(CopiedKey, Value));
}

bool RDCache::has(const RDNode &Key) const {
  return Cache.find(Key) != Cache.end();
}

std::set<RDNode> RDCache::get(const RDNode &Key) const {
  auto Acc = Cache.find(Key);
  if (Acc == Cache.end())
    return {};

  return Acc->second;
}

} // end namespace ftg
