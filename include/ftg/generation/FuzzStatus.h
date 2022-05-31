//===-- FuzzStatus.h - Status of fuzzer/api during generation ---*- C++ -*-===//
///
/// \file Defines class that represents status of fuzzer/api
///
//===----------------------------------------------------------------------===//

#ifndef FTG_GENERATION_FUZZSTATUS_H
#define FTG_GENERATION_FUZZSTATUS_H

#include <string>

namespace ftg {
enum FuzzStatusType {
  UNINITIALIZED,
  NOT_FUZZABLE_UNIDENTIFIED,
  NOT_FUZZABLE_NO_INPUT,
  FUZZABLE,
  FUZZABLE_SRC_NOT_GENERATED,
  FUZZABLE_SRC_GENERATED
};

class FuzzStatus {
public:
  constexpr FuzzStatus() : Type(UNINITIALIZED){};
  constexpr FuzzStatus(FuzzStatusType Type) : Type(Type) {}
  explicit operator bool() = delete;
  operator FuzzStatusType() const;
  operator std::string() const;
  constexpr bool operator==(FuzzStatus Status) const {
    return Type == Status.Type;
  }
  constexpr bool operator!=(FuzzStatus Status) const {
    return Type != Status.Type;
  }
  constexpr bool operator==(FuzzStatusType StatusType) const {
    return Type == StatusType;
  }
  constexpr bool operator!=(FuzzStatusType StatusType) const {
    return Type != StatusType;
  }

private:
  FuzzStatusType Type;
};

} // namespace ftg

#endif // FTG_GENERATION_FUZZSTATUS_H
