#ifndef FTG_APILOADER_APILOADER_H
#define FTG_APILOADER_APILOADER_H

#include <set>
#include <string>

namespace ftg {

class APILoader {
public:
  virtual ~APILoader() = default;
  virtual const std::set<std::string> load() = 0;
};

} // namespace ftg
#endif // FTG_APILOADER_APILOADER_H
