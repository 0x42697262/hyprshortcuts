#pragma once

#include <string>
#include <vector>

#include "Types.hpp"

namespace hs {

// Drop exact duplicate shortcuts, keeping first-seen order. hyprchord registers
// each chain step twice (sticky-mods: with and without the prefix modifiers
// held), which reconstruct to identical shortcuts — this collapses them.
std::vector<Shortcut> dedupeShortcuts(const std::vector<Shortcut>& shortcuts);

// Group shortcuts into categories. Categories are sorted alphabetically
// (case-insensitive); the default/fallback category (`defaultCategory`) always
// sorts LAST so grouped-but-uncategorised binds land at the end. Items within a
// category preserve their input order (stable).
std::vector<Category> groupByCategory(const std::vector<Shortcut>& shortcuts,
                                      const std::string& defaultCategory = "General");

} // namespace hs
