#include <gtest/gtest.h>
#include "../../examples/companion_radio/ui-new/EricssonContactPager.h"

TEST(EricssonContactPager, EmptyAddressBookDoesNotMove) {
  EricssonContactPager p;
  EXPECT_TRUE(p.empty());
  p.previous();
  p.next();
  EXPECT_EQ(0u, p.selected());
  EXPECT_EQ(0u, p.firstVisible(3));
}

TEST(EricssonContactPager, WrapsBothDirections) {
  EricssonContactPager p(350);
  p.previous();
  EXPECT_EQ(349u, p.selected());
  p.next();
  EXPECT_EQ(0u, p.selected());
  for (unsigned i = 0; i < 350; ++i) p.next();
  EXPECT_EQ(0u, p.selected());
}

TEST(EricssonContactPager, SelectionStaysInsideThreeRowViewport) {
  EricssonContactPager p(350);
  for (unsigned i = 0; i < 3 * 350; ++i) {
    const auto first = p.firstVisible(3);
    EXPECT_LE(first, p.selected());
    EXPECT_LT(p.selected(), first + 3u);
    EXPECT_LE(p.selected(), p.count() - 1u);
    p.next();
  }
}

TEST(EricssonContactPager, ShrinkOrDeleteKeepsSelectionValid) {
  EricssonContactPager p(7);
  for (unsigned i = 0; i < 6; ++i) p.next();
  EXPECT_EQ(6u, p.selected());
  p.setCount(4);
  EXPECT_EQ(3u, p.selected());
  p.setCount(0);
  EXPECT_TRUE(p.empty());
  EXPECT_EQ(0u, p.selected());
  p.setCount(2);
  EXPECT_EQ(0u, p.selected());
}

TEST(EricssonContactPager, OpeningBrowserResetsCursor) {
  EricssonContactPager p(350);
  for (unsigned i = 0; i < 200; ++i) p.next();
  p.reset(45);
  EXPECT_EQ(0u, p.selected());
  EXPECT_EQ(45u, p.count());
}

TEST(EricssonContactPager, SupportsMax16BitCountAndZeroRows) {
  EricssonContactPager p(UINT16_MAX);
  p.previous();
  EXPECT_EQ(static_cast<unsigned>(UINT16_MAX - 1u), p.selected());
  EXPECT_EQ(0u, p.firstVisible(0));
}

TEST(EricssonContactPager, MillionNavigationOperationsRemainInBounds) {
  EricssonContactPager p(350);
  for (unsigned i = 0; i < 1000000u; ++i) {
    if (i & 1u) p.next();
    else p.previous();
    ASSERT_LT(p.selected(), p.count());
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
