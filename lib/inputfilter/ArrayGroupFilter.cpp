#include "ftg/inputfilter/ArrayGroupFilter.h"
#include "ftg/utils/AssignUtil.h"
#include "clang/AST/Expr.h"

using namespace clang;

namespace ftg {

const std::string ArrayGroupFilter::FilterName = "ArrayGroupFilter";

ArrayGroupFilter::ArrayGroupFilter(std::unique_ptr<GroupFilter> NextFilter)
    : GroupFilter(FilterName, std::move(NextFilter)) {}

void ArrayGroupFilter::check(
    std::map<unsigned, std::shared_ptr<Definition>> &DefMap) const {
  auto ArrayGroups = generateArrayGroups(DefMap);
  applyArrayGroups(DefMap, ArrayGroups);
}

void ArrayGroupFilter::applyArrayGroups(
    const std::map<unsigned, std::shared_ptr<Definition>> &DefMap,
    std::vector<std::set<unsigned>> &ArrayGroups) const {
  for (const auto &DefMapIter : DefMap) {
    const auto &Def = DefMapIter.second;
    if (!Def || Def->Filters.empty())
      continue;

    auto ID = Def->ID;
    auto GroupIter = std::find_if(ArrayGroups.begin(), ArrayGroups.end(),
                                  [ID](const std::set<unsigned> &Group) {
                                    return Group.find(ID) != Group.end();
                                  });
    if (GroupIter == ArrayGroups.end())
      continue;

    for (auto MemberID : *GroupIter) {
      const auto DefMapFindIter = DefMap.find(MemberID);
      assert(DefMapFindIter != DefMap.end() && "Unexpected Program State");
      const auto &Def = DefMapFindIter->second;
      assert(Def && "Unexpected Program State");
      Def->Filters.emplace(FilterName);
    }
    ArrayGroups.erase(GroupIter);
  }
}

std::vector<std::set<unsigned>> ArrayGroupFilter::generateArrayGroups(
    const std::map<unsigned, std::shared_ptr<Definition>> &DefMap) const {
  std::vector<std::set<unsigned>> Result;
  for (const auto &DefMapIter : DefMap) {
    const auto &Def = DefMapIter.second;
    assert(Def && "Unexpected Program State");

    std::vector<unsigned> Group;
    if (Def->Array)
      Group.insert(Group.end(), Def->ArrayLenIDs.begin(),
                   Def->ArrayLenIDs.end());
    else if (Def->ArrayLen)
      Group.insert(Group.end(), Def->ArrayIDs.begin(), Def->ArrayIDs.end());
    else
      continue;

    Group.erase(std::remove_if(Group.begin(), Group.end(),
                               [DefMap](const auto &ID) {
                                 return DefMap.find(ID) == DefMap.end();
                               }),
                Group.end());
    Group.emplace_back(Def->ID);
    updateGroups(Result, std::set<unsigned>(Group.begin(), Group.end()));
  }
  return Result;
}

void ArrayGroupFilter::updateGroups(std::vector<std::set<unsigned>> &Result,
                                    const std::set<unsigned> &Group) const {
  for (auto &ResultGroup : Result) {
    for (auto E : Group) {
      if (ResultGroup.find(E) == ResultGroup.end())
        continue;
      ResultGroup.insert(Group.begin(), Group.end());
      return;
    }
  }
  Result.push_back(Group);
}

} // namespace ftg
