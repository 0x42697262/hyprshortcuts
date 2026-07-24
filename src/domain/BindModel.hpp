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

// Turn a raw bind into a Shortcut, or nullopt if it should be omitted from the
// cheatsheet (disabled, or has no usable description).
std::optional<Shortcut> toShortcut(const RawBind& bind,
                                   const std::string& defaultCategory = "General");

} // namespace hs
