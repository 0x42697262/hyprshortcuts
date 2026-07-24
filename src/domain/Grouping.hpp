#pragma once

#include <string>
#include <vector>

#include "Types.hpp"

namespace hs {

// Group shortcuts into categories. Categories are sorted alphabetically
// (case-insensitive); the default/fallback category (`defaultCategory`) always
// sorts LAST so grouped-but-uncategorised binds land at the end. Items within a
// category preserve their input order (stable).
std::vector<Category> groupByCategory(const std::vector<Shortcut>& shortcuts,
                                      const std::string& defaultCategory = "General");

} // namespace hs
