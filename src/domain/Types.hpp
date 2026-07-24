#pragma once

#include <cstdint>
#include <string>
#include <vector>

// The domain layer is deliberately free of any Hyprland headers: it operates on
// plain data (RawBind) copied out of Hyprland by the adapter (BindSource), so
// every transformation here can be unit-tested on a normal host build.

namespace hs {

// Raw bind data copied out of Hyprland's SKeybind. POD, no Hyprland types.
struct RawBind {
    uint32_t    modmask        = 0;
    std::string key            = "";  // keysym name, e.g. "RETURN", "S", or "mouse:272"
    std::string displayKey     = "";  // SKeybind.displayKey, preferred when non-empty
    std::string description    = "";  // may carry a "Category: Label" prefix
    std::string handler        = "";  // dispatcher name
    std::string arg            = "";
    std::string submap         = "";
    bool        hasDescription = false;
    bool        mouse          = false;
    bool        enabled        = true;
};

// One key token drawn as a keycap: a compact glyph plus a human-readable label.
// When a key has no distinct glyph, glyph == label (e.g. "Esc").
struct KeyCap {
    std::string glyph;  // e.g. "⊞" (Super), "⏎" (Enter), "S"
    std::string label;  // e.g. "Super", "Enter", "S"
};

// One shortcut row: [mods...] + key  ->  action.
struct Shortcut {
    std::vector<KeyCap> mods;      // ordered modifiers (Super, Ctrl, Alt, Shift, ...)
    KeyCap              key;       // the main key
    std::string         category;  // group name
    std::string         action;    // human label (description with category stripped)
};

// A named group of shortcuts.
struct Category {
    std::string           name;
    std::vector<Shortcut> items;
};

} // namespace hs
