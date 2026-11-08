#include <gtest/gtest.h>
#include <string>
#include <string.h>
#include "../../examples/companion_radio/ui-new/EricssonInboxModel.h"

TEST(EricssonInboxModel, EmptyInboxIsSafe) {
  EricssonInboxModel inbox;
  EXPECT_EQ(0u, inbox.count());
  EXPECT_TRUE(inbox.empty());
  EXPECT_EQ(nullptr, inbox.at(0));
  EXPECT_EQ(nullptr, inbox.current());
  inbox.next();
  inbox.previous();
  inbox.selectLatest();
  EXPECT_EQ(0u, inbox.selected());
}

TEST(EricssonInboxModel, StoresDirectAndChannelMessagesNewestFirst) {
  EricssonInboxModel inbox;
  inbox.add("Alpha", "First", 100, false);
  inbox.add("Group", "Second", 101, true);
  ASSERT_EQ(2u, inbox.count());
  ASSERT_NE(nullptr, inbox.at(0));
  EXPECT_STREQ("Second", inbox.at(0)->text);
  EXPECT_STREQ("Group", inbox.at(0)->sender);
  EXPECT_EQ(101u, inbox.at(0)->receivedAt);
  EXPECT_TRUE(inbox.at(0)->channel);
  EXPECT_STREQ("First", inbox.at(1)->text);
  EXPECT_FALSE(inbox.at(1)->channel);
}

TEST(EricssonInboxModel, BoundedTruncationPreservesTermination) {
  EricssonInboxModel inbox;
  std::string name(2500, 'N');
  std::string text(10000, 'T');
  inbox.add(name.c_str(), text.c_str(), 0, false);
  auto entry = inbox.at(0);
  ASSERT_NE(nullptr, entry);
  EXPECT_EQ(EricssonInboxModel::SenderBytes - 1, strlen(entry->sender));
  EXPECT_EQ(EricssonInboxModel::TextBytes - 1, strlen(entry->text));
  EXPECT_EQ('\0', entry->sender[EricssonInboxModel::SenderBytes - 1]);
  EXPECT_EQ('\0', entry->text[EricssonInboxModel::TextBytes - 1]);
}

TEST(EricssonInboxModel, NullInputsYieldEmptyTerminatedStrings) {
  EricssonInboxModel inbox;
  inbox.add(nullptr, nullptr, 0, false);
  ASSERT_NE(nullptr, inbox.at(0));
  EXPECT_STREQ("", inbox.at(0)->sender);
  EXPECT_STREQ("", inbox.at(0)->text);
}

TEST(EricssonInboxModel, KeepsOnlyLastSixteenMessages) {
  EricssonInboxModel inbox;
  for (unsigned i = 0; i < 100; ++i) {
    const std::string text = std::to_string(i);
    inbox.add("Sender", text.c_str(), i, false);
  }
  ASSERT_EQ(EricssonInboxModel::Capacity, inbox.count());
  for (unsigned i = 0; i < inbox.count(); ++i) {
    const auto* message = inbox.at(i);
    ASSERT_NE(nullptr, message);
    EXPECT_EQ(99u - i, message->receivedAt);
  }
  EXPECT_EQ(nullptr, inbox.at(inbox.count()));
}

TEST(EricssonInboxModel, SelectionWrapsAndViewportContainsSelection) {
  EricssonInboxModel inbox;
  for (unsigned i = 0; i < 10; ++i) inbox.add("A", "B", i, false);
  inbox.selectLatest();
  inbox.previous();
  EXPECT_EQ(9u, inbox.selected());
  inbox.next();
  EXPECT_EQ(0u, inbox.selected());
  for (unsigned i = 0; i < 100; ++i) {
    inbox.next();
    const auto first = inbox.firstVisible(3);
    EXPECT_LE(first, inbox.selected());
    EXPECT_LT(inbox.selected(), first + 3u);
  }
}

TEST(EricssonInboxModel, NewArrivalPreservesSelectedOlderMessageWhenPossible) {
  EricssonInboxModel inbox;
  inbox.add("A", "one", 1, false);
  inbox.add("B", "two", 2, false);
  inbox.selectLatest();
  ASSERT_STREQ("two", inbox.current()->text);
  inbox.add("C", "three", 3, true);
  EXPECT_STREQ("two", inbox.current()->text);
  inbox.selectLatest();
  EXPECT_STREQ("three", inbox.current()->text);
}

TEST(EricssonInboxModel, MillionArrivalsAndNavigationRemainBounded) {
  EricssonInboxModel inbox;
  for (uint32_t i = 0; i < 1000000u; ++i) {
    inbox.add("Node", "body", i, (i & 1u) != 0);
    if ((i & 3u) == 0) inbox.previous();
    else inbox.next();
    ASSERT_LE(inbox.count(), EricssonInboxModel::Capacity);
    ASSERT_LT(inbox.selected(), inbox.count());
    ASSERT_NE(nullptr, inbox.current());
  }
  EXPECT_EQ(EricssonInboxModel::Capacity, inbox.count());
  inbox.selectLatest();
  EXPECT_EQ(999999u, inbox.current()->receivedAt);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
