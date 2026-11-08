#include <gtest/gtest.h>
#include <helpers/radiolib/RadioHealthMetrics.h>

TEST(RadioHealthMetrics, StartsWithZeroCounters) {
  RadioHealthMetrics metrics;
  const RadioHealthSnapshot snapshot = metrics.snapshot();
  EXPECT_EQ(0u, snapshot.rx_arm_errors);
  EXPECT_EQ(0u, snapshot.rx_read_errors);
  EXPECT_EQ(0u, snapshot.tx_start_errors);
  EXPECT_EQ(0u, snapshot.agc_resets);
}

TEST(RadioHealthMetrics, RecordsDifferentFailureTypesIndependently) {
  RadioHealthMetrics metrics;
  metrics.recordRxArmError();
  metrics.recordRxArmError();
  metrics.recordRxReadError();
  metrics.recordTxStartError();
  metrics.recordAgcReset();
  metrics.recordAgcReset();
  metrics.recordAgcReset();

  const RadioHealthSnapshot snapshot = metrics.snapshot();
  EXPECT_EQ(2u, snapshot.rx_arm_errors);
  EXPECT_EQ(1u, snapshot.rx_read_errors);
  EXPECT_EQ(1u, snapshot.tx_start_errors);
  EXPECT_EQ(3u, snapshot.agc_resets);
}

TEST(RadioHealthMetrics, SnapshotIsNotMutableAlias) {
  RadioHealthMetrics metrics;
  RadioHealthSnapshot old = metrics.snapshot();
  metrics.recordTxStartError();
  EXPECT_EQ(0u, old.tx_start_errors);
  EXPECT_EQ(1u, metrics.snapshot().tx_start_errors);
}

TEST(RadioHealthMetrics, ResetClearsAllCounters) {
  RadioHealthMetrics metrics;
  metrics.recordRxArmError();
  metrics.recordRxReadError();
  metrics.recordTxStartError();
  metrics.recordAgcReset();
  metrics.reset();
  const RadioHealthSnapshot snapshot = metrics.snapshot();
  EXPECT_EQ(0u, snapshot.rx_arm_errors);
  EXPECT_EQ(0u, snapshot.rx_read_errors);
  EXPECT_EQ(0u, snapshot.tx_start_errors);
  EXPECT_EQ(0u, snapshot.agc_resets);
}

TEST(RadioHealthMetrics, HandlesMillionIndependentEvents) {
  RadioHealthMetrics metrics;
  for (unsigned i = 0; i < 1000000u; ++i) {
    metrics.recordRxReadError();
    if ((i % 2u) == 0u) metrics.recordRxArmError();
    if ((i % 4u) == 0u) metrics.recordTxStartError();
    if ((i % 8u) == 0u) metrics.recordAgcReset();
  }
  const RadioHealthSnapshot snapshot = metrics.snapshot();
  EXPECT_EQ(500000u, snapshot.rx_arm_errors);
  EXPECT_EQ(1000000u, snapshot.rx_read_errors);
  EXPECT_EQ(250000u, snapshot.tx_start_errors);
  EXPECT_EQ(125000u, snapshot.agc_resets);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
