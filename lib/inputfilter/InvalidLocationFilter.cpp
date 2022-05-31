#include "ftg/inputfilter/InvalidLocationFilter.h"
#include "ftg/utils/StringUtil.h"
#include <regex>

using namespace ftg;

const std::string InvalidLocationFilter::FilterName = "InvalidLocationFilter";

InvalidLocationFilter::InvalidLocationFilter(
    std::string BaseDir, std::unique_ptr<InputFilter> NextFilter)
    : InputFilter(FilterName, std::move(NextFilter)), BaseDir(BaseDir) {}

bool InvalidLocationFilter::check(const ASTIRNode &Node) const {
  auto Loc = getLoc(Node.AST);
  auto Path = std::get<0>(Loc);
  auto Offset = std::get<1>(Loc);
  auto Length = std::get<2>(Loc);

  if (Offset == 0 || Path.empty() || Length == 4294967295U)
    return true;

  if (util::getRelativePath(Path, BaseDir).empty())
    return true;

  if (isHeaderFile(Path))
    return true;

  return false;
}

std::tuple<std::string, unsigned, unsigned>
InvalidLocationFilter::getLoc(const ASTDefNode &ADN) const {
  if (const auto *AssignedNode = ADN.getAssigned()) {
    const auto &Index = AssignedNode->getIndex();
    return std::make_tuple(Index.getPath(), AssignedNode->getOffset(),
                           AssignedNode->getLength());
  }
  const auto &AssigneeNode = ADN.getAssignee();
  const auto &Index = AssigneeNode.getIndex();
  return std::make_tuple(
      Index.getPath(), AssigneeNode.getOffset() + AssigneeNode.getLength(), 0);
}

bool InvalidLocationFilter::isHeaderFile(const std::string &Name) const {
  try {
    std::regex Rule("\\.h$");
    std::sregex_iterator Iter(Name.begin(), Name.end(), Rule);
    std::sregex_iterator End;
    return Iter != End;
  } catch (std::regex_error &E) {
    return false;
  }
}
