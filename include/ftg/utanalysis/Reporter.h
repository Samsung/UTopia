#ifndef FTG_UTANALYSIS_REPORTER_H
#define FTG_UTANALYSIS_REPORTER_H

#include "ftg/utanalysis/UTLoader.h"

namespace ftg {
/**
 * @brief Interface of Reporter
 * @details
 */
class IReporter {
public:
  virtual ~IReporter() = default;
  virtual void report(UTLoader *Loader, std::string Path) = 0;
};
} // namespace ftg
#endif // FTG_UTANALYSIS_REPORTER_H
