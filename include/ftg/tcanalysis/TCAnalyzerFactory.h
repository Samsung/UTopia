#ifndef FTG_TCANALYSIS_TCANALYZERFACTORY_H
#define FTG_TCANALYSIS_TCANALYZERFACTORY_H

#include "TCCallWriter.h"
#include "ftg/tcanalysis/BoostCallWriter.h"
#include "ftg/tcanalysis/BoostExtractor.h"
#include "ftg/tcanalysis/GoogleTestCallWriter.h"
#include "ftg/tcanalysis/GoogleTestExtractor.h"
#include "ftg/tcanalysis/TCTCallWriter.h"
#include "ftg/tcanalysis/TCTExtractor.h"

namespace ftg {

class TCAnalyzerFactory {
public:
  virtual ~TCAnalyzerFactory() = default;
  virtual std::unique_ptr<TCExtractor>
  createTCExtractor(const SourceCollection &SC) const = 0;
  virtual std::unique_ptr<TCCallWriter> createTCCallWriter() const = 0;
};

class TCTAnalyzerFactory : public TCAnalyzerFactory {
public:
  ~TCTAnalyzerFactory() = default;
  std::unique_ptr<TCExtractor>
  createTCExtractor(const SourceCollection &SC) const override {
    return std::make_unique<TCTExtractor>(SC);
  }
  std::unique_ptr<TCCallWriter> createTCCallWriter() const override {
    return std::make_unique<TCTCallWriter>();
  }
};

class GoogleTestAnalyzerFactory : public TCAnalyzerFactory {
public:
  ~GoogleTestAnalyzerFactory() = default;
  std::unique_ptr<TCExtractor>
  createTCExtractor(const SourceCollection &SC) const override {
    return std::make_unique<GoogleTestExtractor>(SC);
  }
  std::unique_ptr<TCCallWriter> createTCCallWriter() const override {
    return std::make_unique<GoogleTestCallWriter>();
  }
};

class BoostAnalyzerFactory : public TCAnalyzerFactory {
public:
  ~BoostAnalyzerFactory() = default;
  std::unique_ptr<TCExtractor>
  createTCExtractor(const SourceCollection &SC) const override {
    return std::make_unique<BoostExtractor>(SC);
  }
  std::unique_ptr<TCCallWriter> createTCCallWriter() const override {
    return std::make_unique<BoostCallWriter>();
  }
};

std::unique_ptr<TCAnalyzerFactory> createTCAnalyzerFactory(std::string Type);

} // namespace ftg

#endif // FTG_TCANALYSIS_TCANALYZERFACTORY_H
