#ifndef FTG_APILOADER_APIEXPORTSNDEPSJSONLOADER_H
#define FTG_APILOADER_APIEXPORTSNDEPSJSONLOADER_H

#include "ftg/apiloader/APILoader.h"
#include <set>
#include <string>

namespace ftg {

class APIExportsNDepsJsonLoader : public APILoader {
public:
  APIExportsNDepsJsonLoader(std::string JsonFilePath, std::string LibName)
      : JsonFilePath(JsonFilePath), LibName(LibName) {}
  const std::set<std::string> load() override;

private:
  std::string JsonFilePath;
  std::string LibName;
};

} // namespace ftg
#endif // FTG_APILOADER_APIEXPORTSNDEPSJSONLOADER_H
