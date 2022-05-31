#ifndef TESTUTIL_APIMANUALLOADER_H
#define TESTUTIL_APIMANUALLOADER_H

#include "ftg/apiloader/APILoader.h"

namespace ftg {

class APIManualLoader : public APILoader {
public:
  APIManualLoader(std::set<std::string> APINames = {});
  const std::set<std::string> load() override;

private:
  std::set<std::string> APINames;
};

} // namespace ftg

#endif // TESTUTIL_APIMANUALLOADER_H
