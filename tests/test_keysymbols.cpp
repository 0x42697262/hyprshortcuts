#include <gtest/gtest.h>

#include "domain/KeySymbols.hpp"

using namespace hs;

TEST(Modmask, EmptyMaskHasNoMods) {
    EXPECT_TRUE(decodeModmask(0).empty());
}

TEST(Modmask, SuperShiftIsOrderedSuperThenShift) {
    // 64 (Super) | 1 (Shift)
    auto mods = decodeModmask(64 | 1);
    ASSERT_EQ(mods.size(), 2u);
    EXPECT_EQ(mods[0].label, "Super");
    EXPECT_EQ(mods[1].label, "Shift");
}

TEST(Modmask, CanonicalOrderSuperCtrlAltShift) {
    auto mods = decodeModmask(1 | 4 | 8 | 64); // shift|ctrl|alt|super, arbitrary bit order
    ASSERT_EQ(mods.size(), 4u);
    EXPECT_EQ(mods[0].label, "Super");
    EXPECT_EQ(mods[1].label, "Ctrl");
    EXPECT_EQ(mods[2].label, "Alt");
    EXPECT_EQ(mods[3].label, "Shift");
}

TEST(NormalizeKey, KnownKeysymGetsGlyphAndLabel) {
    auto k = normalizeKey("RETURN");
    EXPECT_EQ(k.glyph, "⏎"); // ⏎
    EXPECT_EQ(k.label, "Enter");
}

TEST(NormalizeKey, KeysymNameIsCaseInsensitive) {
    EXPECT_EQ(normalizeKey("return").label, "Enter");
    EXPECT_EQ(normalizeKey("Escape").label, "Esc");
}

TEST(NormalizeKey, SingleCharIsUppercased) {
    auto k = normalizeKey("s");
    EXPECT_EQ(k.glyph, "S");
    EXPECT_EQ(k.label, "S");
}

TEST(NormalizeKey, MouseButtonsMapToLabels) {
    EXPECT_EQ(normalizeKey("mouse:272").glyph, "LMB");
    EXPECT_EQ(normalizeKey("mouse:273").glyph, "RMB");
    EXPECT_EQ(normalizeKey("mouse:274").glyph, "MMB");
    EXPECT_EQ(normalizeKey("mouse:275").glyph, "MB275");
}

TEST(NormalizeKey, DisplayKeyIsPreferredWhenSet) {
    // key is empty/opaque but Hyprland gave a displayKey
    auto k = normalizeKey("code:10", "1");
    EXPECT_EQ(k.glyph, "1");
    EXPECT_EQ(k.label, "1");
}

TEST(NormalizeKey, UnknownKeysymPassesThrough) {
    auto k = normalizeKey("XF86AudioPlay");
    EXPECT_EQ(k.glyph, "XF86AudioPlay");
    EXPECT_EQ(k.label, "XF86AudioPlay");
}
