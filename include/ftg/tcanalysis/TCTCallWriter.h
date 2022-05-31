#ifndef FTG_TCANALYSIS_TCTCALLWRITER_H
#define FTG_TCANALYSIS_TCTCALLWRITER_H

#include "ftg/tcanalysis/TCCallWriter.h"

namespace ftg {

class TCTCallWriter : public TCCallWriter {
public:
  std::string getTCCall(const Unittest &TC, std::string Indent) const override;
};
} // namespace ftg

#endif // FTG_TCANALYSIS_TCTCALLWRITER_H
