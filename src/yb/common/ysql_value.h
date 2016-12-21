// Copyright (c) YugaByte, Inc.
//
// This file contains the YSQLValue class that represents YSQL values.

#ifndef YB_COMMON_YSQL_VALUE_H
#define YB_COMMON_YSQL_VALUE_H

#include <stdint.h>

#include "yb/common/common.pb.h"
#include "yb/common/ysql_protocol.pb.h"
#include "yb/common/timestamp.h"
#include "yb/util/faststring.h"
#include "yb/util/slice.h"
#include "yb/util/status.h"

namespace yb {

// A class that stores just the YSQL value part. The datatype and null state are stored out-of-line.
// For YSQL row blocks, it is memory-inefficient for the same column in repeated rows to store the
// datatype info. Also, the null state of the columns in the same row can be stored efficiently
// in a bitmap. See YSQLRow for details.
class YSQLValueCore {

 private:
  friend class YSQLValue;
  friend class YSQLRow;

  inline explicit YSQLValueCore(const DataType type) {
    if (type == STRING) {
      new(&string_value_) std::string();
    }
  }

  inline YSQLValueCore(const DataType type, const YSQLValueCore& v) : YSQLValueCore(type) {
    CopyFrom(type, v);
  }

  // Because destructor cannot take an argument, a YSQLValueCore should be destroyed by first
  // calling the Free() method below with the datatype.
  virtual inline ~YSQLValueCore() { }
  inline void Free(const DataType type) {
    if (type == STRING) {
      string_value_.~basic_string();
    }
  }

  //---------------------------------- get value template -----------------------------------
  // A template to get different datatype value fields. CHECK failure will result if the value
  // is not of the expected or the value is null.
  template<typename type_t>
  static inline type_t value(
      const DataType type, const DataType expected_type, const bool is_null, const type_t value) {
    CHECK_EQ(type, expected_type);
    CHECK(!is_null);
    return value;
  }

  //---------------------------------- set value template -----------------------------------
  // A template to set different datatype value fields. CHECK failure will result if the value
  // is not of the expected.
  template<typename type_t>
  static inline void set_value(
      const DataType type, const DataType expected_type, const type_t other, type_t* value) {
    CHECK_EQ(type, expected_type);
    *value = other;
  }

  // Copy a value
  void CopyFrom(DataType type, const YSQLValueCore& v);

  //----------------------------- serializer / deserializer ---------------------------------
  void Serialize(DataType type, bool is_null, YSQLClient client, faststring* buffer) const;
  Status Deserialize(DataType type, YSQLClient client, Slice* data, bool* is_null);

  //-------------------------------- union of different value types -------------------------
  // Just primitive values currently.
  union {
    int8_t int8_value_;
    int16_t int16_value_;
    int32_t int32_value_;
    int64_t int64_value_;
    float float_value_;
    double double_value_;
    std::string string_value_;
    bool bool_value_;
    Timestamp timestamp_value_;
  };
};

// A YSQL value with datatype and null state. This class is good for expression evaluation.
class YSQLValue : YSQLValueCore {
 public:
  // No need to call timestamp's constructor/destructor because it is a simple wrapper over
  // an uint64_t.
  explicit YSQLValue(DataType type) : YSQLValueCore(type), type_(type) { }

  YSQLValue(const DataType type, const bool is_null, const YSQLValueCore& v) : YSQLValue(type) {
    is_null_ = is_null;
    if (!is_null) {
      CopyFrom(type, v);
    }
  }

  YSQLValue(const YSQLValue& v) : YSQLValue(v.type_, v.is_null_, v) { }

  virtual ~YSQLValue() {
    Free(type_);
  }

  // The value's datatype
  DataType type() const { return type_; }

  // Is the value null?
  bool IsNull() const { return is_null_; }

  // Set the value to null or not.
  void SetNull(const bool is_null) { is_null_ = is_null; }

  //----------------------------------- get value methods -----------------------------------
  // Get different datatype values. CHECK failure will result if the value stored is not of the
  // expected datatype or the value is null.
  int8_t int8_value() const { return value(type_, INT8, is_null_, int8_value_); }
  int16_t int16_value() const { return value(type_, INT16, is_null_, int16_value_); }
  int32_t int32_value() const { return value(type_, INT32, is_null_, int32_value_); }
  int64_t int64_value() const { return value(type_, INT64, is_null_, int64_value_); }
  float float_value() const { return value(type_, FLOAT, is_null_, float_value_); }
  double double_value() const { return value(type_, DOUBLE, is_null_, double_value_); }
  std::string string_value() const { return value(type_, STRING, is_null_, string_value_); }
  bool bool_value() const { return value(type_, BOOL, is_null_, bool_value_); }
  Timestamp timestamp_value() const { return value(type_, TIMESTAMP, is_null_, timestamp_value_); }

  //----------------------------------- set value methods -----------------------------------
  // Set different datatype values. CHECK failure will result if the value stored is not of the
  // expected datatype.
  template<typename type_t>
  void set_value(const DataType expected_type, const type_t other, type_t* value) {
    YSQLValueCore::set_value(type_, expected_type, other, value);
    is_null_ = false;
  }

  void set_int8_value(int8_t v) { return set_value(INT8, v, &int8_value_); }
  void set_int16_value(int16_t v) { return set_value(INT16, v, &int16_value_); }
  void set_int32_value(int32_t v) { return set_value(INT32, v, &int32_value_); }
  void set_int64_value(int64_t v) { return set_value(INT64, v, &int64_value_); }
  void set_float_value(float v) { return set_value(FLOAT, v, &float_value_); }
  void set_double_value(double v) { return set_value(DOUBLE, v, &double_value_); }
  void set_string_value(const std::string& v) { return set_value(STRING, v, &string_value_); }
  void set_bool_value(bool v) { return set_value(BOOL, v, &bool_value_); }
  void set_timestamp_value(const Timestamp& v) {
    return set_value(TIMESTAMP, v, &timestamp_value_);
  }

  // Note: YSQLValue doesn't have serialize / deserialize methods because we expect YSQL values
  // to be serialized / deserialized as part of a row block. See YSQLRowBlock.

  //-------------------------------- value datatype and null state --------------------------
 private:
  friend class YSQLRow;
  const DataType type_;
  bool is_null_ = true;
};

} // namespace yb

#endif // YB_COMMON_YSQL_VALUE_H