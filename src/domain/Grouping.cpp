#include "Grouping.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <unordered_set>

namespace hs {

namespace {

// A stable signature of a shortcut's visible content, for de-duplication.
std::string signature(const Shortcut& s) {
    std::string sig = s.category + "\x1f" + s.action;
    for (const auto& step : s.steps) {
        sig += "\x1e";
        for (const auto& m : step.mods)
            sig += m.label + "+";
        sig += step.key.label;
    }
    return sig;
}

std::string lower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

} // namespace

std::vector<Shortcut> dedupeShortcuts(const std::vector<Shortcut>& shortcuts) {
    std::vector<Shortcut>           out;
    std::unordered_set<std::string> seen;
    for (const auto& s : shortcuts) {
        if (seen.insert(signature(s)).second)
            out.push_back(s);
    }
    return out;
}

std::vector<Category> groupByCategory(const std::vector<Shortcut>& shortcuts,
                                      const std::string& defaultCategory) {
    std::vector<Category>                    cats;
    std::unordered_map<std::string, size_t> index; // category name -> position in cats

    for (const auto& s : shortcuts) {
        auto it = index.find(s.category);
        if (it == index.end()) {
            index[s.category] = cats.size();
            cats.push_back({s.category, {s}});
        } else {
            cats[it->second].items.push_back(s);
        }
    }

    std::stable_sort(cats.begin(), cats.end(), [&](const Category& a, const Category& b) {
        const bool aDef = a.name == defaultCategory;
        const bool bDef = b.name == defaultCategory;
        if (aDef != bDef)
            return !aDef; // default category sinks to the end
        return lower(a.name) < lower(b.name);
    });

    return cats;
}

} // namespace hs
