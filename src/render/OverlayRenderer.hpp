#pragma once

#include <src/helpers/Monitor.hpp>

#include "../domain/Layout.hpp"
#include "TextRenderer.hpp"

namespace hs {

// Draws a computed LayoutTree onto the currently-rendering monitor using
// Hyprland's GL. Must be called from the render hook (live GL context).
// `fade` (0..1) is applied as a global alpha for show/hide animation.
class OverlayRenderer {
  public:
    void draw(const LayoutTree& tree, PHLMONITOR monitor, double fade, TextRenderer& text);
};

} // namespace hs
