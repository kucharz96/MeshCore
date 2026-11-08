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

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
