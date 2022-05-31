//===-- FuzzStatus.cpp - Implementation of FuzzStatus ---------------------===//

#include "ftg/generation/FuzzStatus.h"

using namespace ftg;

FuzzStatus::operator FuzzStatusType() const { return Type; }
FuzzStatus::operator std::string() const {
  switch (Type) {
  case NOT_FUZZABLE_UNIDENTIFIED:
    return "NOT_FUZZABLE_UNIDENTIFIED";
  case NOT_FUZZABLE_NO_INPUT:
    return "NOT_FUZZABLE_NO_INPUT_PARAM";
  case FUZZABLE_SRC_GENERATED:
    return "GENERATED";
  case FUZZABLE:
  case FUZZABLE_SRC_NOT_GENERATED:
    return "NOT_GENERATED";
  case UNINITIALIZED:
  default:
    return "NOT_FUZZABLE_NO_UT_OR_CALL_SEQUENCE";
  }
}
