#include "BindModel.hpp"

#include <algorithm>

#include "KeySymbols.hpp"

namespace hs {

namespace {

std::string trim(const std::string& s) {
    const auto notSpace = [](unsigned char c) { return c != ' ' && c != '\t'; };
    auto begin = std::find_if(s.begin(), s.end(), notSpace);
    auto end   = std::find_if(s.rbegin(), s.rend(), notSpace).base();
    return begin < end ? std::string(begin, end) : std::string();
}

} // namespace

CategoryLabel splitCategory(const std::string& description, const std::string& defaultCategory) {
    const std::string desc = trim(description);
    const auto        pos  = desc.find(':');
    if (pos != std::string::npos) {
        const std::string category = trim(desc.substr(0, pos));
        const std::string label    = trim(desc.substr(pos + 1));
        if (!category.empty() && !label.empty())
            return {category, label};
    }
    return {defaultCategory, desc};
}

std::optional<Shortcut> toShortcut(const RawBind& bind, const std::string& defaultCategory) {
    if (!bind.enabled)
        return std::nullopt;
    if (!bind.hasDescription || trim(bind.description).empty())
        return std::nullopt;

    const CategoryLabel cl = splitCategory(bind.description, defaultCategory);

    Shortcut s;
    s.mods     = decodeModmask(bind.modmask);
    s.key      = normalizeKey(bind.key, bind.displayKey);
    s.category = cl.category;
    s.action   = cl.label;
    return s;
}

} // namespace hs
