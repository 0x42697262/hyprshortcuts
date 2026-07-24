#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Types.hpp"

namespace hs {

struct Vec2 {
    double w = 0;
    double h = 0;
};

// Text measurement is injected so Layout stays Hyprland-free and unit-testable.
// Production wires this to pango (TextRenderer::measure); tests pass a fake.
using TextMeasure = std::function<Vec2(const std::string& text, double px)>;

struct Rect {
    double x = 0;
    double y = 0;
    double w = 0;
    double h = 0;
};

enum class RectRole { Scrim, Panel, Card, KeyCap };
enum class TextRole { Title, KeyGlyph, Plus, ChordSep, Action };

// How a keycap's key is drawn: the compact Unicode glyph (⌘, ⏎) or the
// human-readable label (Super, Enter). Keys without a distinct glyph render the
// same in both modes (glyph == label).
enum class KeyCapStyle { Icons, Text };

struct RectNode {
    Rect     rect;
    RectRole role   = RectRole::Panel;
    double   radius = 0;
};

struct TextNode {
    Rect        rect; // bounding box; renderer aligns text per role
    std::string text;
    double      px   = 14;
    TextRole    role = TextRole::Action;
};

// Fully positioned render tree in monitor-local pixels, ready for the renderer.
struct LayoutTree {
    Rect                  panel;
    std::vector<RectNode> rects; // draw order: scrim, panel, cards, keycaps
    std::vector<TextNode> texts;
};

struct LayoutMetrics {
    double      screenW      = 1920;
    double      screenH      = 1080;
    KeyCapStyle keyStyle     = KeyCapStyle::Icons;
    double maxPanelWidthFrac = 0.9;
    int    maxColumns        = 4;
    double columnWidth       = 340;
    double colGap            = 28;
    double panelPad          = 32;
    double panelRadius       = 18;
    double cardPadX          = 16;
    double cardPadY          = 12;
    double cardGap           = 18;
    double cardRadius        = 12;
    double titleHeight       = 26;
    double titleGap          = 6;
    double titlePx           = 16;
    double rowHeight         = 30;
    double rowGap            = 6;
    double keycapH           = 24;
    double keycapMinW        = 26;
    double keycapPadX        = 9;
    double keycapGap         = 6;   // gap around the "+" joiner
    double keycapRadius      = 6;
    double glyphPx           = 14;
    double plusPx            = 13;
    double keyToActionGap    = 14;
    double actionPx          = 15;
};

// Compute the full render tree for the given categories.
LayoutTree computeLayout(const std::vector<Category>& cats, const LayoutMetrics& m,
                         const TextMeasure& measure);

} // namespace hs
