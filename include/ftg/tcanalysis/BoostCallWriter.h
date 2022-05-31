#ifndef FTG_TCANALYSIS_BOOSTCALLWRITER_H
#define FTG_TCANALYSIS_BOOSTCALLWRITER_H

#include "ftg/tcanalysis/TCCallWriter.h"

namespace ftg {

class BoostCallWriter : public TCCallWriter {
public:
  std::string getTCCall(const Unittest &TC, std::string Indent) const override;
};
} // namespace ftg

#endif // FTG_TCANALYSIS_BOOSTCALLWRITER_H
