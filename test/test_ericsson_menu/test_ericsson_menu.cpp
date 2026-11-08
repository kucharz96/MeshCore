#include <gtest/gtest.h>
#include "../../examples/companion_radio/ui-new/EricssonMenuModel.h"

TEST(EricssonMenuModel, EmptyMenuIsSafe) {
  EricssonMenuModel menu;
  for (unsigned i = 0; i < 1000; ++i) {
    menu.up();
    menu.down();
  }
  EXPECT_EQ(0u, menu.selected());
  EXPECT_EQ(0u, menu.firstVisible(3));
}

TEST(EricssonMenuModel, BothDirectionsWrapAtEdges) {
  EricssonMenuModel menu(8);
  menu.up();
  EXPECT_EQ(7u, menu.selected());
  menu.down();
  EXPECT_EQ(0u, menu.selected());
  for (unsigned i = 0; i < 24; ++i) menu.down();
  EXPECT_EQ(0u, menu.selected());
}

TEST(EricssonMenuModel, VisiblePageAlwaysContainsSelection) {
  for (unsigned count = 1; count <= 20; ++count) {
    EricssonMenuModel menu(count);
    for (unsigned i = 0; i < count * 4; ++i) {
      const auto start = menu.firstVisible(3);
      EXPECT_LE(start, menu.selected());
      EXPECT_LT(menu.selected(), start + 3);
      EXPECT_LE(start, count - 1);
      menu.down();
    }
  }
}

TEST(EricssonMenuModel, ZeroVisibleRowsIsSafe) {
  EricssonMenuModel menu(8);
  menu.up();
  EXPECT_EQ(0u, menu.firstVisible(0));
}

TEST(EricssonMenuModel, ShrinkingResetsInvalidSelection) {
  EricssonMenuModel menu(7);
  for (int i = 0; i < 6; ++i) menu.down();
  EXPECT_EQ(6u, menu.selected());
  menu.setItemCount(3);
  EXPECT_EQ(0u, menu.selected());
  menu.setItemCount(0);
  menu.up();
  EXPECT_EQ(0u, menu.selected());
}

TEST(EricssonMenuModel, MillionNavigationEventsStayInRange) {
  EricssonMenuModel menu(8);
  for (unsigned i = 0; i < 1000000u; ++i) {
    if ((i % 3u) == 0u) menu.up();
    else menu.down();
    EXPECT_LT(menu.selected(), menu.count());
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
