// Copyright (c) YugaByte, Inc.

#include "yb/util/uuid.h"

namespace yb {

Uuid::Uuid() {
}

Uuid::Uuid(const Uuid& other) {
  boost_uuid_ = other.boost_uuid_;
}

CHECKED_STATUS Uuid::FromString(const std::string& strval) {
  try {
    boost_uuid_ = boost::lexical_cast<boost::uuids::uuid>(strval);
  } catch (std::exception& e) {
    return STATUS(Corruption, "Couldn't read Uuid from string!");
  }
  return Status::OK();
}

std::string Uuid::ToString() const {
  std::string strval;
  CHECK_OK(ToString(&strval));
  return strval;
}

CHECKED_STATUS Uuid::ToString(std::string *strval) const {
  *strval = boost::uuids::to_string(boost_uuid_);
  return Status::OK();
}

CHECKED_STATUS Uuid::EncodeToComparable(std::string* bytes) const {
  try {
    uint8_t output[kUuidSize];
    if (boost_uuid_.version() == boost::uuids::uuid::version_time_based) {
      // Take the MSB of the UUID and get the timestamp ordered bytes.
      toTimestampBytes(output);
    } else {
      toVersionFirstBytes(output);
    }
    memcpy(output + kUuidMsbSize, boost_uuid_.data + kUuidMsbSize, kUuidLsbSize);
    bytes->assign(reinterpret_cast<char *>(output), kUuidSize);
  } catch (std::exception& e) {
    return STATUS(Corruption, "Couldn't serialize Uuid to raw bytes!");
  }
  return Status::OK();
}

CHECKED_STATUS Uuid::ToBytes(std::string* bytes) const {
  try {
    (*bytes).assign(boost_uuid_.begin(), boost_uuid_.end());
  } catch (std::exception& e) {
    return STATUS(Corruption, "Couldn't serialize Uuid to raw bytes!");
  }
  return Status::OK();
}

CHECKED_STATUS Uuid::FromSlice(const Slice& slice, size_t size_hint) {
  size_t expected_size = (size_hint == 0) ? slice.size() : size_hint;
  if (expected_size > slice.size()) {
    return STATUS_SUBSTITUTE(InvalidArgument, "Size of slice: $0 is smaller than provided "
        "size_hint: $1", slice.size(), expected_size);
  }
  if (expected_size != kUuidSize) {
    return STATUS_SUBSTITUTE(InvalidArgument, "Size of slice is invalid: $0", expected_size);
  }
  memcpy(boost_uuid_.begin(), slice.data(), kUuidSize);
  return Status::OK();
}

CHECKED_STATUS Uuid::FromBytes(const std::string& bytes) {
  Slice slice (bytes.data(), bytes.size());
  return FromSlice(slice);
}

CHECKED_STATUS Uuid::DecodeFromComparableSlice(const Slice& slice, size_t size_hint) {
  size_t expected_size = (size_hint == 0) ? slice.size() : size_hint;
  if (expected_size > slice.size()) {
    return STATUS_SUBSTITUTE(InvalidArgument, "Size of slice: $0 is smaller than provided "
        "size_hint: $1", slice.size(), expected_size);
  }
  if (expected_size != kUuidSize) {
    return STATUS_SUBSTITUTE(InvalidArgument,
                             "Decode error: Size of slice is invalid: $0", expected_size);
  }
  const uint8_t* bytes = slice.data();
  if ((bytes[0] & 0xF0) == 0x10) {
    // Check the first byte to see if it is version 1.
    fromTimestampBytes(bytes);
  } else {
    fromVersionFirstBytes(bytes);
  }
  memcpy(boost_uuid_.data + kUuidMsbSize, bytes + kUuidMsbSize, kUuidLsbSize);
  return Status::OK();
}

CHECKED_STATUS Uuid::DecodeFromComparable(const std::string& bytes) {
  Slice slice (bytes.data(), bytes.size());
  return DecodeFromComparableSlice(slice);
}

} // namespace yb