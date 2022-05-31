//===-- JsonSerializable.h - interface for json serializable ----*- C++ -*-===//
///
/// \file
/// Defines interface for json serializable classes.
///
//===----------------------------------------------------------------------===//

#ifndef FTG_JSONSERIALIZABLE_H
#define FTG_JSONSERIALIZABLE_H

#include "ftg/utils/json/json.h"

namespace ftg {
class JsonSerializable {
public:
  virtual ~JsonSerializable() = default;
  virtual Json::Value toJson() const = 0;
  /// Load json data into current instance.
  /// \return true if data is loaded successfully else false.
  virtual bool fromJson(Json::Value) = 0;
};
} // namespace ftg

#endif // FTG_JSONSERIALIZABLE_H
