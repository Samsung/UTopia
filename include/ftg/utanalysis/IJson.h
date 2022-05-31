#ifndef FTG_UTANALYSIS_IJSON_H
#define FTG_UTANALYSIS_IJSON_H

#include "ftg/utils/json/json.h"

namespace ftg {
/**
 * @brief Interface for supporting data written in json format
 * @details
 */
class IJson {
public:
  virtual ~IJson() = default;

  virtual Json::Value getJson() = 0;
};

} // namespace ftg

#endif // FTG_UTANALYSIS_IJSON_H
