#include <gtest/gtest.h>

#include "../../examples/companion_radio/LoopTimingMetrics.h"

TEST(LoopTimingMetrics, StartsAtZero) {
  LoopTimingMetrics m;
  const auto s = m.snapshot();
  EXPECT_EQ(0u, s.iterations);
  EXPECT_EQ(0u, s.over10ms);
  EXPECT_EQ(0u, s.over50ms);
  EXPECT_EQ(0u, s.maxTotalUs);
  EXPECT_EQ(0u, s.maxUiUs);
}

TEST(LoopTimingMetrics, TracksIndependentStageMaxima) {
  LoopTimingMetrics m;
  m.record(120, 50, 70, 900, 40, 1180);
  m.record(400, 200, 300, 100, 50, 1050);
  const auto s = m.snapshot();
  EXPECT_EQ(2u, s.iterations);
  EXPECT_EQ(1180u, s.maxTotalUs);
  EXPECT_EQ(400u, s.maxMeshUs);
  EXPECT_EQ(200u, s.maxInterfaceUs);
  EXPECT_EQ(300u, s.maxSensorsUs);
  EXPECT_EQ(900u, s.maxUiUs);
  EXPECT_EQ(50u, s.maxOtherUs);
}

TEST(LoopTimingMetrics, SlowThresholdsAreInclusive) {
  LoopTimingMetrics m;
  m.record(0, 0, 0, 0, 0, 9999);
  m.record(0, 0, 0, 0, 0, 10000);
  m.record(0, 0, 0, 0, 0, 49999);
  m.record(0, 0, 0, 0, 0, 50000);
  const auto s = m.snapshot();
  EXPECT_EQ(4u, s.iterations);
  EXPECT_EQ(3u, s.over10ms);
  EXPECT_EQ(1u, s.over50ms);
  EXPECT_EQ(50000u, s.maxTotalUs);
}

TEST(LoopTimingMetrics, ElapsedMicrosWorksAcrossCounterWrap) {
  EXPECT_EQ(0u, LoopTimingMetrics::elapsedUs(100, 100));
  EXPECT_EQ(900u, LoopTimingMetrics::elapsedUs(100, 1000));
  EXPECT_EQ(75u, LoopTimingMetrics::elapsedUs(UINT32_MAX - 49u, 25u));
  EXPECT_EQ(1u, LoopTimingMetrics::elapsedUs(UINT32_MAX, 0));
}

TEST(LoopTimingMetrics, ResetClearsAllData) {
  LoopTimingMetrics m;
  m.record(6000, 1000, 1000, 40000, 2000, 50000);
  m.reset();
  const auto s = m.snapshot();
  EXPECT_EQ(0u, s.iterations);
  EXPECT_EQ(0u, s.over10ms);
  EXPECT_EQ(0u, s.over50ms);
  EXPECT_EQ(0u, s.maxTotalUs);
  EXPECT_EQ(0u, s.maxMeshUs);
  EXPECT_EQ(0u, s.maxUiUs);
}

TEST(LoopTimingMetrics, MillionMixedEventsRemainBounded) {
  LoopTimingMetrics m;
  for (uint32_t i = 0; i < 1000000u; ++i) {
    if (i % 100u == 0u) {
      m.record(12000, 1000, 1000, 40000, 1000, 55000);
    } else if (i % 10u == 0u) {
      m.record(300, 300, 300, 10000, 100, 11000);
    } else {
      m.record(300, 300, 300, 300, 100, 1300);
    }
  }
  const auto s = m.snapshot();
  EXPECT_EQ(1000000u, s.iterations);
  EXPECT_EQ(100000u, s.over10ms);
  EXPECT_EQ(10000u, s.over50ms);
  EXPECT_EQ(55000u, s.maxTotalUs);
  EXPECT_EQ(40000u, s.maxUiUs);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
