#pragma once

#include <string>
#include <vector>

#include "Types.hpp"

namespace hs {

// Decode a Hyprland modmask into ordered modifier keycaps.
// Order is fixed for a stable, readable display: Super, Ctrl, Alt, Shift, then
// any exotic mods (Hyper, Mod5, CapsLock).
std::vector<KeyCap> decodeModmask(uint32_t modmask);

// Normalize a key into a keycap. Prefers displayKey when non-empty (Hyprland
// fills it for keys bound by code/name), otherwise maps the keysym name to a
// compact glyph + label. Unknown keys fall back to the raw string.
KeyCap normalizeKey(const std::string& key, const std::string& displayKey = "");

// Look up a modifier by name (case-insensitive: "super", "shift", "ctrl",
// "control", "alt", "hyper", ...). Returns false for unknown names (e.g. the
// key token in a chord step). Used to parse hyprchord step reprs.
bool modByName(const std::string& name, KeyCap& out);

} // namespace hs
