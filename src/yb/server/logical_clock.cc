// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// The following only applies to changes made to this file as part of YugaByte development.
//
// Portions Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//

#include "yb/server/logical_clock.h"

#include "yb/gutil/strings/substitute.h"
#include "yb/util/metrics.h"
#include "yb/util/monotime.h"
#include "yb/util/status.h"
#include "yb/util/atomic.h"

namespace yb {
namespace server {

METRIC_DEFINE_gauge_uint64(server, logical_clock_hybrid_time,
                           "Logical Clock Hybrid Time",
                           yb::MetricUnit::kUnits,
                           "Logical clock hybrid time.");

using base::subtle::Atomic64;
using base::subtle::Barrier_AtomicIncrement;
using base::subtle::NoBarrier_CompareAndSwap;

HybridTime LogicalClock::Now() {
  return HybridTime(++now_);
}

HybridTime LogicalClock::Peek() {
  return HybridTime(now_.load(std::memory_order_acquire));
}

HybridTime LogicalClock::NowLatest() {
  return Now();
}

void LogicalClock::Update(const HybridTime& to_update) {
  if (to_update.is_valid()) {
    UpdateAtomicMax(&now_, to_update.value());
  }
}

Status LogicalClock::WaitUntilAfter(const HybridTime& then,
                                    const MonoTime& deadline) {
  return STATUS(ServiceUnavailable,
      "Logical clock does not support WaitUntilAfter()");
}

Status LogicalClock::WaitUntilAfterLocally(const HybridTime& then,
                                           const MonoTime& deadline) {
  if (IsAfter(then)) return Status::OK();
  return STATUS(ServiceUnavailable,
      "Logical clock does not support WaitUntilAfterLocally()");
}

bool LogicalClock::IsAfter(HybridTime t) {
  return now_.load(std::memory_order_acquire) >= t.value();
}

LogicalClock* LogicalClock::CreateStartingAt(const HybridTime& hybrid_time) {
  // initialize at 'hybrid_time' - 1 so that the  first output value is 'hybrid_time'.
  return new LogicalClock(hybrid_time.value() - 1);
}

uint64_t LogicalClock::NowForMetrics() {
  // We don't want reading metrics to change the clock.
  return now_.load(std::memory_order_acquire);
}


void LogicalClock::RegisterMetrics(const scoped_refptr<MetricEntity>& metric_entity) {
  METRIC_logical_clock_hybrid_time.InstantiateFunctionGauge(
      metric_entity,
      Bind(&LogicalClock::NowForMetrics, Unretained(this)))
    ->AutoDetachToLastValue(&metric_detacher_);
}

string LogicalClock::Stringify(HybridTime hybrid_time) {
  return strings::Substitute("L: $0", hybrid_time.ToUint64());
}

}  // namespace server
}  // namespace yb

