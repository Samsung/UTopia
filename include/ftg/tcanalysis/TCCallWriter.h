#ifndef FTG_TCANALYSIS_TCCALLWRITER_H
#define FTG_TCANALYSIS_TCCALLWRITER_H

#include "ftg/tcanalysis/Unittest.h"

namespace ftg {

class TCCallWriter {
public:
  virtual ~TCCallWriter() = default;
  virtual std::string getTCCall(const Unittest &TC,
                                std::string Indent = "") const = 0;
};
} // namespace ftg

#endif // FTG_TCANALYSIS_TCCALLWRITER_H
