#pragma once

#include <optional>
#include <string>

#include "Types.hpp"

namespace hs {

struct CategoryLabel {
    std::string category;
    std::string label;
};

// Split a description of the form "Category: Label" on the FIRST ':'. If there
// is no colon, or either side is empty after trimming, the whole (trimmed)
// description becomes the label under `defaultCategory`.
CategoryLabel splitCategory(const std::string& description,
                            const std::string& defaultCategory = "General");

// Parse a single hyprchord step repr ("super+shift+s") into a Step: everything
// before the last '+' is modifiers, the final token is the key.
Step parseStep(const std::string& repr);

// Reconstruct the leading steps of a hyprchord chain from its submap name
// ("hc:super+x;a" -> [SUPER+X, A]). Returns empty for a non-hyprchord submap.
std::vector<Step> chordPrefixSteps(const std::string& submap);

// Turn a raw bind into a Shortcut, or nullopt if it should be omitted from the
// cheatsheet (disabled, no usable description, or hyprchord chain/abort
// machinery). hyprchord chain binds (submap "hc:...") are expanded into their
// full multi-step sequence.
std::optional<Shortcut> toShortcut(const RawBind& bind,
                                   const std::string& defaultCategory = "General");

} // namespace hs
