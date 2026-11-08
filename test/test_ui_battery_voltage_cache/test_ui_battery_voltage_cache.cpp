#include <gtest/gtest.h>
#include "../../examples/companion_radio/ui-new/UiBatteryVoltageCache.h"

TEST(UiBatteryVoltageCache, MustSampleBeforeFirstDisplay) {
  UiBatteryVoltageCache cache;
  EXPECT_FALSE(cache.hasValue());
  EXPECT_TRUE(cache.needsSample(0));
  EXPECT_TRUE(cache.needsSample(4000));
}

TEST(UiBatteryVoltageCache, ReusesRecentSampleWithoutNewAdcRead) {
  UiBatteryVoltageCache cache;
  cache.store(12000, 3844);
  EXPECT_TRUE(cache.hasValue());
  EXPECT_EQ(3844u, cache.value());
  EXPECT_FALSE(cache.needsSample(12000));
  EXPECT_FALSE(cache.needsSample(16999));
  EXPECT_TRUE(cache.needsSample(17000));
}

TEST(UiBatteryVoltageCache, ResamplesEveryFiveSeconds) {
  UiBatteryVoltageCache cache;
  cache.store(0, 3600);
  EXPECT_FALSE(cache.needsSample(4999));
  EXPECT_TRUE(cache.needsSample(5000));
  cache.store(5000, 3590);
  EXPECT_FALSE(cache.needsSample(9999));
  EXPECT_TRUE(cache.needsSample(10000));
  EXPECT_EQ(3590u, cache.value());
}

TEST(UiBatteryVoltageCache, WorksAcrossMillisRollover) {
  UiBatteryVoltageCache cache;
  cache.store(UINT32_MAX - 1000u, 3880);
  EXPECT_FALSE(cache.needsSample(1000));
  EXPECT_FALSE(cache.needsSample(3998));
  EXPECT_TRUE(cache.needsSample(3999));
}

TEST(UiBatteryVoltageCache, InvalidateForcesRefresh) {
  UiBatteryVoltageCache cache;
  cache.store(123, 3980);
  cache.invalidate();
  EXPECT_FALSE(cache.hasValue());
  EXPECT_TRUE(cache.needsSample(124));
  cache.store(124, 3970);
  EXPECT_TRUE(cache.hasValue());
  EXPECT_EQ(3970u, cache.value());
}

TEST(UiBatteryVoltageCache, MillionRedrawsDoNotRequireMillionSamples) {
  UiBatteryVoltageCache cache;
  unsigned samples = 0;
  for (uint32_t now = 0; now < 1000000; ++now) {
    if (cache.needsSample(now)) {
      cache.store(now, 3900);
      ++samples;
    }
  }
  EXPECT_EQ(200u, samples);
  EXPECT_EQ(3900u, cache.value());
}


TEST(UiBatteryVoltageCache, DelayedAdcCompletionStartsFreshnessAtCompletion) {
  UiBatteryVoltageCache cache;
  const uint32_t readStartMs = 1000;
  EXPECT_TRUE(cache.needsSample(readStartMs));
  // ADC setup and sampling are blocking; timestamp only after completion.
  cache.store(readStartMs + 15, 3850);
  EXPECT_FALSE(cache.needsSample(readStartMs + 5014));
  EXPECT_TRUE(cache.needsSample(readStartMs + 5015));
}

TEST(UiBatteryVoltageCache, RepeatedInvalidationsNeverServeStaleSample) {
  UiBatteryVoltageCache cache;
  for (uint32_t cycle = 0; cycle < 100000; ++cycle) {
    cache.store(cycle, static_cast<uint16_t>(3000 + cycle % 1000));
    EXPECT_TRUE(cache.hasValue());
    cache.invalidate();
    EXPECT_FALSE(cache.hasValue());
    EXPECT_TRUE(cache.needsSample(cycle));
  }
}

TEST(UiBatteryVoltageCache, FaultInjectionDroppedReadsRetainLastGoodSample) {
  UiBatteryVoltageCache cache;
  cache.store(0, 3900);
  // Simulate a higher-level ADC read failure by not calling store().
  // The cache must continue requesting a fresh sample, while preserving
  // its last displayed value. Actual ADC failure detection is external.
  for (uint32_t now = 5000; now < 15000; now += 100) {
    EXPECT_TRUE(cache.needsSample(now));
    EXPECT_EQ(3900u, cache.value());
  }
  cache.store(15000, 3750);
  EXPECT_FALSE(cache.needsSample(15001));
  EXPECT_EQ(3750u, cache.value());
}

TEST(UiBatteryVoltageCache, MillisRolloverWithRepeatedSampling) {
  UiBatteryVoltageCache cache;
  uint32_t now = UINT32_MAX - 12000u;
  for (unsigned i = 0; i < 1000; ++i) {
    EXPECT_TRUE(cache.needsSample(now));
    cache.store(now, 3800);
    EXPECT_FALSE(cache.needsSample(now + 4999u));
    now += UiBatteryVoltageCache::RefreshMs;
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
