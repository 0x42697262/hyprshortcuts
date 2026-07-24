#pragma once

#include <src/helpers/Monitor.hpp>

#include "../domain/Layout.hpp"
#include "TextRenderer.hpp"

namespace hs {

// A straight RGBA colour in 0..1. Populated from config (OverlayController) or
// left at the struct defaults, which reproduce the built-in dark theme.
struct RGBA {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 1;
};

// The full colour theme. Rect fills use their alpha; text colours use rgb only
// (their alpha comes from the fade). Defaults match the original hardcoded look.
struct Theme {
    // Rect fills.
    RGBA scrim  = {0.00f, 0.00f, 0.00f, 0.45f};
    RGBA panel  = {0.11f, 0.12f, 0.15f, 0.94f};
    RGBA card   = {0.16f, 0.17f, 0.21f, 0.96f};
    RGBA keycap = {0.24f, 0.25f, 0.30f, 1.00f};
    // Text colours.
    RGBA title    = {0.55f, 0.78f, 1.00f};
    RGBA keyGlyph = {0.96f, 0.96f, 0.98f};
    RGBA separator = {0.50f, 0.52f, 0.58f}; // the "+" joiner and chord "›"
    RGBA action   = {0.82f, 0.84f, 0.88f};
};

// Draws a computed LayoutTree onto the currently-rendering monitor using
// Hyprland's GL. Must be called from the render hook (live GL context).
// `fade` (0..1) is applied as a global alpha for show/hide animation.
class OverlayRenderer {
  public:
    void setTheme(const Theme& theme) { m_theme = theme; }
    void draw(const LayoutTree& tree, PHLMONITOR monitor, double fade, TextRenderer& text);

  private:
    Theme m_theme;
};

} // namespace hs
