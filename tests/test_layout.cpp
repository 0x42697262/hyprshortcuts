#include <gtest/gtest.h>

#include <algorithm>

#include "domain/Layout.hpp"

using namespace hs;

namespace {

// Deterministic fake measurer: width ~ chars * 0.6em, height = px.
TextMeasure fakeMeasure() {
    return [](const std::string& text, double px) -> Vec2 {
        return {static_cast<double>(text.size()) * px * 0.6, px};
    };
}

Category catWith(const std::string& name, int rows) {
    Category c;
    c.name = name;
    for (int i = 0; i < rows; ++i) {
        Shortcut s;
        s.steps  = {{{{"⌘", "Super"}}, {"S", "S"}}};
        s.action = "do thing " + std::to_string(i);
        c.items.push_back(s);
    }
    return c;
}

int countRole(const LayoutTree& t, RectRole role) {
    return static_cast<int>(std::count_if(t.rects.begin(), t.rects.end(),
                                          [&](const RectNode& r) { return r.role == role; }));
}

} // namespace

TEST(Layout, ProducesScrimAndPanel) {
    LayoutMetrics m;
    auto          t = computeLayout({catWith("Apps", 2)}, m, fakeMeasure());
    EXPECT_EQ(countRole(t, RectRole::Scrim), 1);
    EXPECT_EQ(countRole(t, RectRole::Panel), 1);
    EXPECT_EQ(countRole(t, RectRole::Card), 1);
}

TEST(Layout, PanelIsCenteredOnScreen) {
    LayoutMetrics m;
    m.screenW = 1920;
    m.screenH = 1080;
    auto t    = computeLayout({catWith("Apps", 3)}, m, fakeMeasure());
    EXPECT_NEAR(t.panel.x, (m.screenW - t.panel.w) / 2.0, 0.001);
    EXPECT_NEAR(t.panel.y, (m.screenH - t.panel.h) / 2.0, 0.001);
    EXPECT_GT(t.panel.w, 0);
    EXPECT_GT(t.panel.h, 0);
}

TEST(Layout, EachModAndKeyGetsAKeycapRect) {
    LayoutMetrics m;
    // One row: one mod (Super) + one key => 2 keycaps.
    auto t = computeLayout({catWith("Apps", 1)}, m, fakeMeasure());
    EXPECT_EQ(countRole(t, RectRole::KeyCap), 2);
}

TEST(Layout, EmitsTitleGlyphAndActionText) {
    LayoutMetrics m;
    auto          t = computeLayout({catWith("Apps", 1)}, m, fakeMeasure());
    int           titles = 0, glyphs = 0, actions = 0, plus = 0, seps = 0;
    for (const auto& tx : t.texts) {
        switch (tx.role) {
            case TextRole::Title: ++titles; break;
            case TextRole::KeyGlyph: ++glyphs; break;
            case TextRole::Action: ++actions; break;
            case TextRole::Plus: ++plus; break;
            case TextRole::ChordSep: ++seps; break;
        }
    }
    EXPECT_EQ(seps, 0); // single-step shortcut has no chord separator
    EXPECT_EQ(titles, 1);
    EXPECT_EQ(glyphs, 2);  // Super + S
    EXPECT_EQ(actions, 1);
    EXPECT_EQ(plus, 1);    // one "+" between the two keycaps
}

TEST(Layout, ChordStepsGetSeparatorBetweenThem) {
    LayoutMetrics m;
    Category      c;
    c.name = "Chords";
    Shortcut s;
    // Two steps: SUPER+X › F  => 2 keycaps + 1 chord separator, no "+" (mods=1 each? no)
    s.steps  = {{{{"⌘", "Super"}}, {"X", "X"}}, {{}, {"F", "F"}}};
    s.action = "Firefox";
    c.items.push_back(s);
    auto t = computeLayout({c}, m, fakeMeasure());

    int keycaps = countRole(t, RectRole::KeyCap);
    int seps = 0, plus = 0;
    for (const auto& tx : t.texts) {
        if (tx.role == TextRole::ChordSep)
            ++seps;
        if (tx.role == TextRole::Plus)
            ++plus;
    }
    EXPECT_EQ(keycaps, 3); // Super, X, F
    EXPECT_EQ(seps, 1);    // one › between the two steps
    EXPECT_EQ(plus, 1);    // one + inside step 1 (Super + X)
}

TEST(Layout, IconStyleEmitsGlyphText) {
    LayoutMetrics m;
    m.keyStyle = KeyCapStyle::Icons;
    auto t     = computeLayout({catWith("Apps", 1)}, m, fakeMeasure());
    std::vector<std::string> glyphs;
    for (const auto& tx : t.texts)
        if (tx.role == TextRole::KeyGlyph)
            glyphs.push_back(tx.text);
    ASSERT_EQ(glyphs.size(), 2u);
    EXPECT_EQ(glyphs[0], "⌘"); // Super's glyph, not its label
    EXPECT_EQ(glyphs[1], "S");
}

TEST(Layout, TextStyleEmitsLabelText) {
    LayoutMetrics m;
    m.keyStyle = KeyCapStyle::Text;
    auto t     = computeLayout({catWith("Apps", 1)}, m, fakeMeasure());
    std::vector<std::string> labels;
    for (const auto& tx : t.texts)
        if (tx.role == TextRole::KeyGlyph)
            labels.push_back(tx.text);
    ASSERT_EQ(labels.size(), 2u);
    EXPECT_EQ(labels[0], "Super"); // Super's label, not its glyph
    EXPECT_EQ(labels[1], "S");
}

TEST(Layout, TextStyleWidensKeycapsForLongLabels) {
    const auto capWidthFor = [](KeyCapStyle style) {
        LayoutMetrics m;
        m.keyStyle = style;
        Category c;
        c.name = "T";
        Shortcut s;
        s.steps  = {{{}, {"⏎", "Enter"}}}; // glyph shorter than label
        s.action = "x";
        c.items.push_back(s);
        auto   t    = computeLayout({c}, m, fakeMeasure());
        double capW = 0;
        for (const auto& r : t.rects)
            if (r.role == RectRole::KeyCap)
                capW = r.rect.w;
        return capW;
    };
    EXPECT_GT(capWidthFor(KeyCapStyle::Text), capWidthFor(KeyCapStyle::Icons));
}

TEST(Layout, UsesMultipleColumnsForManyCategories) {
    LayoutMetrics m;
    m.screenW = 3840; // wide screen fits several columns
    std::vector<Category> cats;
    for (int i = 0; i < 8; ++i)
        cats.push_back(catWith("Cat" + std::to_string(i), 2));
    auto t = computeLayout(cats, m, fakeMeasure());
    // Panel should be wider than a single column (more than one column used).
    EXPECT_GT(t.panel.w, m.columnWidth + 2 * m.panelPad + 1);
}

TEST(Layout, FewCategoriesFitOnePage) {
    LayoutMetrics m;
    auto          pages = computePages({catWith("A", 2), catWith("B", 2)}, m, fakeMeasure());
    ASSERT_EQ(pages.size(), 1u);
    EXPECT_EQ(pages[0].pageCount, 1);
    EXPECT_EQ(pages[0].pageIndex, 0);
}

TEST(Layout, TallContentSplitsIntoPages) {
    LayoutMetrics m;
    m.screenW = 400; // narrow -> a single column
    m.screenH = 300; // short -> little vertical room per page
    std::vector<Category> cats;
    for (int i = 0; i < 5; ++i)
        cats.push_back(catWith("C" + std::to_string(i), 2));
    auto pages = computePages(cats, m, fakeMeasure());

    ASSERT_GT(pages.size(), 1u);
    for (size_t i = 0; i < pages.size(); ++i) {
        EXPECT_EQ(pages[i].pageIndex, static_cast<int>(i));
        EXPECT_EQ(pages[i].pageCount, static_cast<int>(pages.size()));
    }
    // every category card survives, spread across the pages
    int cards = 0;
    for (const auto& p : pages)
        cards += countRole(p, RectRole::Card);
    EXPECT_EQ(cards, 5);
}

TEST(Layout, AllPagesShareOnePanelSizeAndPosition) {
    LayoutMetrics m;
    m.screenW = 400; // single column, short screen -> several pages
    m.screenH = 300;
    std::vector<Category> cats;
    for (int i = 0; i < 6; ++i)
        cats.push_back(catWith("C" + std::to_string(i), i % 3 + 1)); // varying card heights
    auto pages = computePages(cats, m, fakeMeasure());

    ASSERT_GT(pages.size(), 1u);
    for (const auto& p : pages) {
        EXPECT_NEAR(p.panel.w, pages[0].panel.w, 0.001);
        EXPECT_NEAR(p.panel.h, pages[0].panel.h, 0.001);
        EXPECT_NEAR(p.panel.x, pages[0].panel.x, 0.001); // so the window never shifts
        EXPECT_NEAR(p.panel.y, pages[0].panel.y, 0.001);
    }
}

TEST(Layout, KeycapWidthGrowsWithGlyphWidth) {
    LayoutMetrics m;
    Category      c;
    c.name = "T";
    Shortcut s;
    s.steps  = {{{}, {"WWWWWW", "wide"}}}; // wide glyph
    s.action = "x";
    c.items.push_back(s);
    auto t = computeLayout({c}, m, fakeMeasure());

    double capW = 0;
    for (const auto& r : t.rects)
        if (r.role == RectRole::KeyCap)
            capW = r.rect.w;
    EXPECT_GT(capW, m.keycapMinW); // wide glyph forced the cap past the minimum
}
