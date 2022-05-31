#ifndef FTG_ROOTDEFANALYSIS_RDTARGET_H
#define FTG_ROOTDEFANALYSIS_RDTARGET_H

#include "RDSpace.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"
#include <set>
#include <vector>

namespace ftg {

class RDTarget {

public:
  enum MemoryType { MemoryType_REG, MemoryType_CONST, MemoryType_MEM };

  static std::shared_ptr<RDTarget> create(llvm::Value &IR);
  static std::shared_ptr<RDTarget> create(llvm::Value &IR,
                                          std::vector<unsigned> MemoryIndices);
  static std::shared_ptr<RDTarget> create(const RDTarget &T);
  static std::set<std::shared_ptr<RDTarget>> create(llvm::Value &IR,
                                                    RDSpace *Space);

  RDTarget(MemoryType Kind, llvm::Value &IR,
           std::vector<unsigned> MemoryIndices = {});
  RDTarget(const RDTarget &Rhs);

  virtual ~RDTarget() = default;

  MemoryType getKind() const;
  llvm::Value &getIR() const;
  const std::vector<unsigned> &getMemoryIndices() const;

  void setMemoryIndices(std::vector<unsigned> MemoryIndices);

  virtual bool isAssignable() const = 0;
  bool isGlobalVariable() const;
  bool includes(const RDTarget &Target) const;

  virtual void dump() const = 0;

  bool operator==(const RDTarget &Rhs) const;
  bool operator!=(const RDTarget &Rhs) const;
  bool operator<(const RDTarget &Rhs) const;
  RDTarget &operator=(const RDTarget &Rhs);

protected:
  MemoryType Kind;
  llvm::Value *IR;
  std::vector<unsigned> MemoryIndices;
};

class RDRegister : public RDTarget {

public:
  static bool classof(const RDTarget *Target);

  RDRegister(llvm::Value &Target);

  bool isAssignable() const override;

  void dump() const override;
};

class RDConstant : public RDTarget {

public:
  static bool classof(const RDTarget *Target);

  RDConstant(llvm::Value &Target, std::vector<unsigned> MemoryIndices);

  bool isAssignable() const override;

  void dump() const override;
};

class RDMemory : public RDTarget {

public:
  static bool classof(const RDTarget *Target);

  RDMemory(llvm::Value &Target, std::vector<unsigned> MemoryIndices);

  bool isAssignable() const override;

  void dump() const override;
};

} // namespace ftg

#endif // FTG_ROOTDEFANALYSIS_RDTARGET_H
