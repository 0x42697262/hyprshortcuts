#include <gtest/gtest.h>

#include "domain/BindModel.hpp"

using namespace hs;

TEST(SplitCategory, SplitsOnFirstColon) {
    auto cl = splitCategory("Apps: Terminal");
    EXPECT_EQ(cl.category, "Apps");
    EXPECT_EQ(cl.label, "Terminal");
}

TEST(SplitCategory, KeepsColonsInLabel) {
    auto cl = splitCategory("Web: open http://example.com");
    EXPECT_EQ(cl.category, "Web");
    EXPECT_EQ(cl.label, "open http://example.com");
}

TEST(SplitCategory, NoColonGoesToDefault) {
    auto cl = splitCategory("Close window");
    EXPECT_EQ(cl.category, "General");
    EXPECT_EQ(cl.label, "Close window");
}

TEST(SplitCategory, EmptyLabelFallsBackToDefault) {
    auto cl = splitCategory("Apps: ");
    EXPECT_EQ(cl.category, "General");
    EXPECT_EQ(cl.label, "Apps:"); // whole trimmed description as label
}

TEST(SplitCategory, RespectsCustomDefault) {
    auto cl = splitCategory("plain", "Misc");
    EXPECT_EQ(cl.category, "Misc");
}

TEST(ToShortcut, DropsBindsWithoutDescription) {
    RawBind b;
    b.hasDescription = false;
    b.description    = "";
    EXPECT_FALSE(toShortcut(b).has_value());
}

TEST(ToShortcut, DropsDisabledBinds) {
    RawBind b;
    b.hasDescription = true;
    b.description    = "Apps: Terminal";
    b.enabled        = false;
    EXPECT_FALSE(toShortcut(b).has_value());
}

TEST(ToShortcut, BuildsShortcutFromDescribedBind) {
    RawBind b;
    b.modmask        = 64 | 1; // super+shift
    b.key            = "S";
    b.description    = "Screenshot: Region";
    b.hasDescription = true;

    auto s = toShortcut(b);
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(s->category, "Screenshot");
    EXPECT_EQ(s->action, "Region");
    ASSERT_EQ(s->mods.size(), 2u);
    EXPECT_EQ(s->mods[0].label, "Super");
    EXPECT_EQ(s->mods[1].label, "Shift");
    EXPECT_EQ(s->key.glyph, "S");
}
