#ifndef FTG_GENERATION_FUZZER_H
#define FTG_GENERATION_FUZZER_H

#include "ftg/generation/FuzzInput.h"
#include "ftg/generation/FuzzStatus.h"
#include "ftg/inputanalysis/InputAnalysisReport.h"
#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/tcanalysis/Unittest.h"
#include "clang/Tooling/Core/Replacement.h"

namespace ftg {

class Fuzzer {
public:
  static std::shared_ptr<Fuzzer> create(const Unittest &UT,
                                        const InputAnalysisReport &InputReport);

  Fuzzer(const Unittest &UT);
  void prepareFuzzInputs(const InputAnalysisReport &InputReport);

  const Unittest &getUT() const;

  const std::map<unsigned, std::shared_ptr<FuzzInput>> &getFuzzInputMap() const;
  const std::map<unsigned, std::shared_ptr<FuzzNoInput>> &
  getFuzzNoInputMap() const;
  std::string getName() const;
  void setName(std::string Name);
  void addReplace(const clang::tooling::Replacement &Replace);

  std::map<std::string, clang::tooling::Replacements> &getSrcReplacements();

  void setUTBottomString(std::string FilePath, std::string UTBottomString);
  std::string getUTBottomString(std::string FilePath);

  void setStatus(FuzzStatus Status);
  FuzzStatus getStatus() const;

  void setRelativeUTDir(std::string Path);
  std::string getRelativeUTDir() const;

  // Used only for test FuzzInputGenerator. TODO: Find better test method.
  bool isFuzzableInput(unsigned DefID) const;

private:
  std::string Name;
  const Unittest *UT;

  std::map<unsigned, std::shared_ptr<FuzzInput>> DefIDFuzzInputMap;
  std::map<unsigned, std::shared_ptr<FuzzNoInput>> DefIDFuzzNoInputMap;

  // Source Replacements (key: filepath to replace)
  std::map<std::string, clang::tooling::Replacements> SrcReplacements;
  std::map<std::string, std::string> UTBottomStringMap;

  // information for FTG build
  FuzzStatus ProjectStatus = NOT_FUZZABLE_UNIDENTIFIED;
  std::string RelativeUTDir;
};

} // namespace ftg

#endif // FTG_GENERATION_FUZZER_H
