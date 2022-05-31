#include "APIManualLoader.h"

namespace ftg {

APIManualLoader::APIManualLoader(std::set<std::string> APINames)
    : APINames(APINames) {}

const std::set<std::string> APIManualLoader::load() { return APINames; }

} // namespace ftg
