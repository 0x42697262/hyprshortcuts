#include <gtest/gtest.h>

#include "domain/Grouping.hpp"

using namespace hs;

namespace {
Shortcut sc(const std::string& category, const std::string& action) {
    Shortcut s;
    s.category = category;
    s.action   = action;
    s.steps    = {{{}, {action, action}}};
    return s;
}
} // namespace

TEST(Grouping, GroupsByCategory) {
    std::vector<Shortcut> in{sc("Apps", "a"), sc("Window", "w"), sc("Apps", "b")};
    auto                  cats = groupByCategory(in);
    ASSERT_EQ(cats.size(), 2u);
    // "Apps" < "Window" alphabetically
    EXPECT_EQ(cats[0].name, "Apps");
    ASSERT_EQ(cats[0].items.size(), 2u);
    EXPECT_EQ(cats[1].name, "Window");
}

TEST(Grouping, PreservesItemOrderWithinCategory) {
    std::vector<Shortcut> in{sc("Apps", "first"), sc("Apps", "second"), sc("Apps", "third")};
    auto                  cats = groupByCategory(in);
    ASSERT_EQ(cats.size(), 1u);
    ASSERT_EQ(cats[0].items.size(), 3u);
    EXPECT_EQ(cats[0].items[0].action, "first");
    EXPECT_EQ(cats[0].items[1].action, "second");
    EXPECT_EQ(cats[0].items[2].action, "third");
}

TEST(Grouping, DefaultCategorySortsLast) {
    std::vector<Shortcut> in{sc("General", "g"), sc("Apps", "a"), sc("Zebra", "z")};
    auto                  cats = groupByCategory(in);
    ASSERT_EQ(cats.size(), 3u);
    EXPECT_EQ(cats[0].name, "Apps");
    EXPECT_EQ(cats[1].name, "Zebra");
    EXPECT_EQ(cats[2].name, "General"); // default sinks below even "Zebra"
}

TEST(Dedupe, CollapsesIdenticalShortcuts) {
    // Same category/action/steps => duplicate (hyprchord sticky-mods pair).
    std::vector<Shortcut> in{sc("Apps", "Firefox"), sc("Apps", "Firefox"), sc("Apps", "Files")};
    auto                  out = dedupeShortcuts(in);
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0].action, "Firefox");
    EXPECT_EQ(out[1].action, "Files");
}

TEST(Dedupe, KeepsShortcutsDifferingByKey) {
    Shortcut a = sc("Apps", "Same");
    a.steps    = {{{}, {"A", "A"}}};
    Shortcut b = sc("Apps", "Same");
    b.steps    = {{{}, {"B", "B"}}};
    auto out   = dedupeShortcuts({a, b});
    EXPECT_EQ(out.size(), 2u); // different keys => not duplicates
}

TEST(Grouping, CategorySortIsCaseInsensitive) {
    std::vector<Shortcut> in{sc("beta", "b"), sc("Alpha", "a")};
    auto                  cats = groupByCategory(in);
    ASSERT_EQ(cats.size(), 2u);
    EXPECT_EQ(cats[0].name, "Alpha");
    EXPECT_EQ(cats[1].name, "beta");
}
