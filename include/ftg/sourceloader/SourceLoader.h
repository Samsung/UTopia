//===-- sourceloader.h - interface of source loader -------------*- C++ -*-===//
///
/// \file
/// Defines interface for loading target sources(AST/BC) of analysis.
///
//===----------------------------------------------------------------------===//

#ifndef FTG_SOURCELOADER_SOURCELOADER_H
#define FTG_SOURCELOADER_SOURCELOADER_H

#include "ftg/sourceloader/SourceCollection.h"

namespace ftg {

/// Interface of source loader which loads target sources(AST/BC) of analysis
class SourceLoader {
public:
  virtual std::unique_ptr<SourceCollection> load() = 0;
  virtual ~SourceLoader() = default;
};

} // namespace ftg

#endif // FTG_SOURCELOADER_SOURCELOADER_H
