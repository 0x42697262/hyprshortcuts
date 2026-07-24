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

TEST(ToShortcut, BuildsSingleStepShortcutFromDescribedBind) {
    RawBind b;
    b.modmask        = 64 | 1; // super+shift
    b.key            = "S";
    b.description    = "Screenshot: Region";
    b.hasDescription = true;

    auto s = toShortcut(b);
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(s->category, "Screenshot");
    EXPECT_EQ(s->action, "Region");
    ASSERT_EQ(s->steps.size(), 1u);
    ASSERT_EQ(s->steps[0].mods.size(), 2u);
    EXPECT_EQ(s->steps[0].mods[0].label, "Super");
    EXPECT_EQ(s->steps[0].mods[1].label, "Shift");
    EXPECT_EQ(s->steps[0].key.glyph, "S");
}

TEST(ParseStep, SplitsModsAndKey) {
    auto st = parseStep("super+shift+s");
    ASSERT_EQ(st.mods.size(), 2u);
    EXPECT_EQ(st.mods[0].label, "Super");
    EXPECT_EQ(st.mods[1].label, "Shift");
    EXPECT_EQ(st.key.glyph, "S");
}

TEST(ChordPrefixSteps, ParsesSubmapChain) {
    auto steps = chordPrefixSteps("hc:super+x;a");
    ASSERT_EQ(steps.size(), 2u);
    EXPECT_EQ(steps[0].mods.size(), 1u);
    EXPECT_EQ(steps[0].mods[0].label, "Super");
    EXPECT_EQ(steps[0].key.glyph, "X");
    EXPECT_TRUE(steps[1].mods.empty());
    EXPECT_EQ(steps[1].key.glyph, "A");
}

TEST(ChordPrefixSteps, EmptyForNonHyprchordSubmap) {
    EXPECT_TRUE(chordPrefixSteps("").empty());
    EXPECT_TRUE(chordPrefixSteps("resize").empty());
}

TEST(ToShortcut, ExpandsHyprchordChainToMultipleSteps) {
    RawBind b;
    b.modmask        = 0; // sticky final step, no mods in repr
    b.key            = "F";
    b.submap         = "hc:super+x";
    b.description    = "Apps: Firefox";
    b.hasDescription = true;

    auto s = toShortcut(b);
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(s->category, "Apps");
    EXPECT_EQ(s->action, "Firefox");
    ASSERT_EQ(s->steps.size(), 2u);
    EXPECT_EQ(s->steps[0].key.glyph, "X"); // SUPER+X
    EXPECT_EQ(s->steps[0].mods[0].label, "Super");
    EXPECT_EQ(s->steps[1].key.glyph, "F"); // › F
    EXPECT_TRUE(s->steps[1].mods.empty());
}

TEST(ToShortcut, DropsChordMachinery) {
    RawBind chain;
    chain.key = "X";
    chain.description = "hyprchords: chain super+x ...";
    chain.hasDescription = true;
    EXPECT_FALSE(toShortcut(chain).has_value());

    RawBind abort;
    abort.key = "Escape";
    abort.submap = "hc:super+x";
    abort.description = "hyprchords: abort chain";
    abort.hasDescription = true;
    EXPECT_FALSE(toShortcut(abort).has_value());

    RawBind catchall; // catchall has empty key
    catchall.key = "";
    catchall.submap = "hc:super+x";
    catchall.description = "hyprchords: abort chain (unmatched key)";
    catchall.hasDescription = true;
    EXPECT_FALSE(toShortcut(catchall).has_value());
}

TEST(ToShortcut, DropsUndescribedHyprchordAutoLabels) {
    // A chord with no user description carries hyprchord's auto label; hide it.
    RawBind b;
    b.key            = "F";
    b.submap         = "hc:super+x";
    b.description    = "hyprchords: super+x ; f -> lua ";
    b.hasDescription = true;
    EXPECT_FALSE(toShortcut(b).has_value());
}
