#include "KeySymbols.hpp"

#include <algorithm>
#include <array>
#include <cctype>

namespace hs {

namespace {

// Hyprland/X11 modmask bits.
constexpr uint32_t MOD_SHIFT = 1 << 0;  // 1
constexpr uint32_t MOD_CAPS  = 1 << 1;  // 2
constexpr uint32_t MOD_CTRL  = 1 << 2;  // 4
constexpr uint32_t MOD_ALT   = 1 << 3;  // 8   (Mod1)
constexpr uint32_t MOD_MOD3  = 1 << 5;  // 32  (Hyper by convention)
constexpr uint32_t MOD_SUPER = 1 << 6;  // 64  (Mod4)
constexpr uint32_t MOD_MOD5  = 1 << 7;  // 128 (mode_switch by convention)

struct ModEntry {
    uint32_t    bit;
    const char* glyph;
    const char* label;
};

// Display order, most significant first.
constexpr std::array<ModEntry, 7> kMods{{
    {MOD_SUPER, "⌘", "Super"},   // ⌘  (command-like logo reads well as "meta")
    {MOD_CTRL, "⌃", "Ctrl"},     // ⌃
    {MOD_ALT, "⌥", "Alt"},       // ⌥
    {MOD_SHIFT, "⇧", "Shift"},   // ⇧
    {MOD_MOD3, "✦", "Hyper"},    // ✦
    {MOD_MOD5, "↹", "Mode"},     // ↹
    {MOD_CAPS, "⇪", "CapsLk"},   // ⇪
}};

std::string upper(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return out;
}

// Known keysym -> {glyph, label}. Keyed by uppercased keysym name.
struct KeyEntry {
    const char* name;
    const char* glyph;
    const char* label;
};

constexpr std::array<KeyEntry, 24> kKeys{{
    {"RETURN", "⏎", "Enter"},      // ⏎
    {"KP_ENTER", "⏎", "Enter"},
    {"SPACE", "␣", "Space"},       // ␣
    {"TAB", "⇥", "Tab"},           // ⇥
    {"ESCAPE", "⎋", "Esc"},        // ⎋
    {"BACKSPACE", "⌫", "Bksp"},    // ⌫
    {"DELETE", "⌦", "Del"},        // ⌦
    {"LEFT", "←", "Left"},         // ←
    {"RIGHT", "→", "Right"},       // →
    {"UP", "↑", "Up"},             // ↑
    {"DOWN", "↓", "Down"},         // ↓
    {"HOME", "⇱", "Home"},         // ⇱
    {"END", "⇲", "End"},           // ⇲
    {"PRIOR", "⇞", "PgUp"},        // ⇞
    {"PAGE_UP", "⇞", "PgUp"},
    {"NEXT", "⇟", "PgDn"},         // ⇟
    {"PAGE_DOWN", "⇟", "PgDn"},
    {"PRINT", "PrtSc", "Print"},
    {"INSERT", "Ins", "Insert"},
    {"MENU", "≣", "Menu"},         // ≣
    {"MINUS", "-", "Minus"},
    {"EQUAL", "=", "Equal"},
    {"SLASH", "/", "Slash"},
    {"COMMA", ",", "Comma"},
}};

KeyCap mouseCap(const std::string& key) {
    // key looks like "mouse:272"
    const auto pos = key.find(':');
    const std::string codeStr = pos == std::string::npos ? "" : key.substr(pos + 1);
    if (codeStr == "272")
        return {"LMB", "Left Click"};
    if (codeStr == "273")
        return {"RMB", "Right Click"};
    if (codeStr == "274")
        return {"MMB", "Middle Click"};
    return {"MB" + codeStr, "Mouse " + codeStr};
}

} // namespace

std::vector<KeyCap> decodeModmask(uint32_t modmask) {
    std::vector<KeyCap> out;
    for (const auto& m : kMods) {
        if (modmask & m.bit)
            out.push_back({m.glyph, m.label});
    }
    return out;
}

bool modByName(const std::string& name, KeyCap& out) {
    // Accepts the modifier tokens hyprchord emits in a step repr (lowercase),
    // plus common aliases. The key token of a step will not match -> false.
    static const std::array<std::pair<const char*, size_t>, 9> aliases{{
        {"super", 0}, {"mod4", 0}, {"win", 0}, // -> Super
        {"ctrl", 1},  {"control", 1},           // -> Ctrl
        {"alt", 2},   {"mod1", 2},              // -> Alt
        {"shift", 3},
        {"hyper", 4}, // MOD3
    }};
    const std::string up = upper(name);
    for (const auto& [alias, idx] : aliases) {
        if (up == upper(alias)) {
            out = {kMods[idx].glyph, kMods[idx].label};
            return true;
        }
    }
    return false;
}

KeyCap normalizeKey(const std::string& key, const std::string& displayKey) {
    const std::string& raw = !displayKey.empty() ? displayKey : key;
    if (raw.empty())
        return {"?", "?"};

    if (raw.rfind("mouse:", 0) == 0)
        return mouseCap(raw);

    const std::string up = upper(raw);
    for (const auto& e : kKeys) {
        if (up == e.name)
            return {e.glyph, e.label};
    }

    // Single character (letter/digit/punct): uppercase glyph, same label.
    if (raw.size() == 1) {
        const std::string u = upper(raw);
        return {u, u};
    }

    // Unknown multi-char keysym: show it verbatim (e.g. "F5", "XF86Calculator").
    return {raw, raw};
}

} // namespace hs
