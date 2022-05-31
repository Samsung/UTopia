#ifndef FTG_ROOTDEFANALYSIS_RDCACHE_H
#define FTG_ROOTDEFANALYSIS_RDCACHE_H

#include "RDNode.h"

namespace ftg {

class RDCache {
public:
  RDCache() = default;
  void cache(RDNode Key, std::set<RDNode> Value);

  bool has(const RDNode &Key) const;
  std::set<RDNode> get(const RDNode &Key) const;

private:
  std::map<RDNode, std::set<RDNode>> Cache;
};

} // namespace ftg

#endif // FTG_ROOTDEFANALYSIS_RDCACHE_H
