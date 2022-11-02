#ifndef FTG_INPUTFILTER_ARRAYGROUPFILTER_H
#define FTG_INPUTFILTER_ARRAYGROUPFILTER_H

#include "ftg/inputfilter/GroupFilter.h"
#include "ftg/type/Type.h"

namespace ftg {

class ArrayGroupFilter : public GroupFilter {
public:
  static const std::string FilterName;
  ArrayGroupFilter(std::unique_ptr<GroupFilter> NextFilter = nullptr);

protected:
  virtual void
  check(std::map<unsigned, std::shared_ptr<Definition>> &DefMap) const override;

private:
  void applyArrayGroups(
      const std::map<unsigned, std::shared_ptr<Definition>> &DefMap,
      std::vector<std::set<unsigned>> &ArrayGroups) const;
  std::vector<std::set<unsigned>> generateArrayGroups(
      const std::map<unsigned, std::shared_ptr<Definition>> &DefMap) const;
  void updateGroups(std::vector<std::set<unsigned>> &Result,
                    const std::set<unsigned> &Group) const;
};

} // namespace ftg

#endif // FTG_INPUTFILTER_ARRAYGROUPFILTER_H
