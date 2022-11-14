#ifndef FTG_PROPANALYSIS_ARGPROPANALYSISREPORT_HPP
#define FTG_PROPANALYSIS_ARGPROPANALYSISREPORT_HPP

#include "ftg/AnalyzerReport.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

namespace ftg {

template <typename T> class ArgPropAnalysisReport : public AnalyzerReport {

public:
  std::tuple<std::string, unsigned, std::vector<int>>
  decomposeArgPropReportKey(std::string Key) const {
    auto Result =
        std::make_tuple<std::string, unsigned, std::vector<int>>("", 0, {});

    try {
      auto SIdx = Key.find("(");
      auto EIdx = Key.find(")");
      if (SIdx == std::string::npos || EIdx == std::string::npos)
        return Result;
      if (EIdx <= SIdx + 1)
        return Result;

      std::get<0>(Result) = Key.substr(0, SIdx);
      std::get<1>(Result) = std::stol(Key.substr(SIdx + 1, EIdx));

      SIdx = Key.rfind("[");
      EIdx = Key.rfind("]");
      if (SIdx == std::string::npos || EIdx == std::string::npos)
        return Result;

      std::vector<int> Indices;
      unsigned BaseOffset = SIdx + 1;
      for (unsigned S = SIdx + 1; S < EIdx; ++S) {
        if (Key[S] != ',')
          continue;
        Indices.emplace_back(std::stoi(Key.substr(BaseOffset, S)));
        BaseOffset = S + 1;
      }
      std::get<2>(Result) = Indices;
    } catch (std::exception &E) {
    }
    return Result;
  }

  void set(const llvm::Argument &Key, T Value, std::vector<int> Indices = {}) {
    auto RealKey = getAsKey(Key, Indices);
    Result.emplace(RealKey, Value);
  }

  void set(std::string FuncName, unsigned ArgIdx, T Value,
           std::vector<int> Indices = {}) {
    Result.emplace(getAsKey(FuncName, ArgIdx, Indices), Value);
  }

  void set(std::string Key, T Value) { Result.emplace(Key, Value); }

  void set(const std::map<std::string, T> &Map) { Result = Map; }

  bool has(const llvm::Argument &Key, std::vector<int> Indices = {}) const {
    auto RealKey = getAsKey(Key, Indices);
    return Result.find(RealKey) != Result.end();
  }

  bool has(std::string FuncName, unsigned ArgIdx,
           std::vector<int> Indices = {}) const {
    auto RealKey = getAsKey(FuncName, ArgIdx, Indices);
    return Result.find(RealKey) != Result.end();
  }

  bool has(std::string Key) const { return Result.find(Key) != Result.end(); }

  T get(const llvm::Argument &Key, std::vector<int> Indices = {}) const {
    auto RealKey = getAsKey(Key, Indices);
    auto Iter = Result.find(RealKey);
    assert(Iter != Result.end() && "Unexpected Program State");
    return Iter->second;
  }

  T get(std::string FuncName, unsigned ArgIdx,
        std::vector<int> Indices = {}) const {
    auto RealKey = getAsKey(FuncName, ArgIdx, Indices);
    auto Iter = Result.find(RealKey);
    assert(Iter != Result.end() && "Unexpected Program State");
    return Iter->second;
  }

  T get(std::string Key) const {
    auto Iter = Result.find(Key);
    assert(Iter != Result.end() && "Unexpected Program State");
    return Iter->second;
  }

  const std::map<std::string, T> &get() const { return Result; }

protected:
  std::map<std::string, T> Result;

  std::string getAsKey(const llvm::Argument &A,
                       std::vector<int> Indices = {}) const {
    const auto *F = A.getParent();
    assert(F && "Unexpected Program State");
    return getAsKey(F->getName().str(), A.getArgNo(), Indices);
  }

  std::string getAsKey(std::string FuncName, unsigned ArgIdx,
                       std::vector<int> Indices = {}) const {
    std::string Key =
        std::string(FuncName) + "(" + std::to_string(ArgIdx) + ")";
    if (Indices.size() == 0)
      return Key;

    Key += "[";
    for (auto Index : Indices)
      Key += std::to_string(Index) + ",";
    Key.pop_back();
    Key += "]";
    return Key;
  }
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_ARGPROPANALYSISREPORT_HPP
