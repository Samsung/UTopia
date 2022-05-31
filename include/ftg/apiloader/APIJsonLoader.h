#ifndef FTG_APILOADER_APIJSONLOADER_H
#define FTG_APILOADER_APIJSONLOADER_H

#include "ftg/apiloader/APILoader.h"
#include <set>
#include <string>

namespace ftg {

class APIJsonLoader : public APILoader {
public:
  APIJsonLoader(std::string JsonFilePath) : JsonFilePath(JsonFilePath) {}
  const std::set<std::string> load() override;

private:
  bool isDeprecatedLibrary(std::string LibName);
  std::string JsonFilePath;
};

} // namespace ftg
#endif // FTG_APILOADER_APIJSONLOADER_H
