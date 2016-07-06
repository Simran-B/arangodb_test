////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Max Neunhoeffer
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_BASICS_HYBRID_LOGICAL_CLOCK_H
#define ARANGODB_BASICS_HYBRID_LOGICAL_CLOCK_H 1

#include <chrono>

class HybridLogicalClock {
  std::chrono::high_resolution_clock clock;
  std::atomic<uint64_t> lastTimeStamp;

 public:
  HybridLogicalClock() : lastTimeStamp(0) {
  }
  HybridLogicalClock(HybridLogicalClock const& other) = delete;
  HybridLogicalClock(HybridLogicalClock&& other) = delete;
  HybridLogicalClock& operator=(HybridLogicalClock const& other) = delete;
  HybridLogicalClock& operator=(HybridLogicalClock&& other) = delete;

  uint64_t getTimeStamp() {
    uint64_t oldTimeStamp;
    uint64_t newTimeStamp;
    do {
      uint64_t physical = getPhysicalTime();
      oldTimeStamp = lastTimeStamp.load(std::memory_order_relaxed);
      uint64_t oldTime = extractTime(oldTimeStamp);
      newTimeStamp = (physical <= oldTime) ?
                     assembleTimeStamp(oldTime, extractCount(oldTimeStamp)+1) :
                     assembleTimeStamp(physical, 0);
    } while (!lastTimeStamp.compare_exchange_weak(oldTimeStamp, newTimeStamp,
                 std::memory_order_release, std::memory_order_relaxed));
    return newTimeStamp;
  }

  // Call the following when a message with a time stamp has been received:
  uint64_t getTimeStamp(uint64_t receivedTimeStamp) {
    uint64_t oldTimeStamp;
    uint64_t newTimeStamp;
    do {
      uint64_t physical = getPhysicalTime();
      oldTimeStamp = lastTimeStamp.load(std::memory_order_relaxed);
      uint64_t oldTime = extractTime(oldTimeStamp);
      uint64_t recTime = extractTime(receivedTimeStamp);
      uint64_t newTime = (std::max)((std::max)(oldTime, physical),recTime);
      // Note that this implies newTime >= oldTime and newTime >= recTime
      uint64_t newCount;
      if (newTime == oldTime) {
        if (newTime == recTime) {
          // all three identical
          newCount = (std::max)(extractCount(oldTime), extractCount(recTime))+1;
        } else {
          // this means recTime < newTime
          newCount = extractCount(oldTime) + 1;
        }
      } else {
        // newTime > oldTime
        if (newTime == recTime) {
          newCount = extractCount(recTime) + 1;
        } else {
          newCount = 0;
        }
      }
      newTimeStamp = assembleTimeStamp(newTime, newCount);
    } while (!lastTimeStamp.compare_exchange_weak(oldTimeStamp, newTimeStamp,
                 std::memory_order_release, std::memory_order_relaxed));
    return newTimeStamp;
  }

 private:
  // Helper to get the physical time in milliseconds since the epoch:
  uint64_t getPhysicalTime() {
    auto now = clock.now();
    uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    return ms;
  }

  static uint64_t extractTime(uint64_t t) {
    return t >> 20;
  }

  static uint64_t extractCount(uint64_t t) {
    return t & 0xfffffUL;
  }

  static uint64_t assembleTimeStamp(uint64_t time, uint64_t count) {
    return (time << 20) | count;
  }

};

#endif