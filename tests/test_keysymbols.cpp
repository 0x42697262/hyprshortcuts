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

TEST(NormalizeKey, RealKeyBeatsDisplayKey) {
    // displayKey often holds a human string; the real keysym in `key` wins.
    auto k = normalizeKey("E", "SUPER + E");
    EXPECT_EQ(k.glyph, "E");
}

TEST(NormalizeKey, DisplayKeyIsFallbackWhenKeyEmpty) {
    auto k = normalizeKey("", "1"); // e.g. a code:NN bind
    EXPECT_EQ(k.glyph, "1");
}

TEST(NormalizeKey, IgnoresHyprchordInternalDisplayKey) {
    // hyprchord's internal id must never be shown as a key.
    auto k = normalizeKey("", "hyprchords:1:0:64");
    EXPECT_EQ(k.glyph, "?");
}

TEST(NormalizeKey, UnknownKeysymPassesThrough) {
    auto k = normalizeKey("XF86WWW");
    EXPECT_EQ(k.glyph, "XF86WWW");
    EXPECT_EQ(k.label, "XF86WWW");
}

TEST(NormalizeKey, MapsCommonXF86MediaKeys) {
    EXPECT_EQ(normalizeKey("XF86AudioPlay").label, "Play/Pause");
    EXPECT_EQ(normalizeKey("XF86AudioNext").label, "Next");
}
