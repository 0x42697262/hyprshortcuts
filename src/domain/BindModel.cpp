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

namespace {

std::vector<std::string> splitOn(const std::string& s, char sep) {
    std::vector<std::string> out;
    size_t                   start = 0;
    while (start <= s.size()) {
        const size_t pos = s.find(sep, start);
        if (pos == std::string::npos) {
            out.push_back(s.substr(start));
            break;
        }
        out.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return out;
}

// hyprchord submaps are named "hc:<prefix>". Detect and strip the marker.
constexpr const char* kSubmapPrefix = "hc:";

// True for binds we never want on the sheet: hyprchord's auto-generated labels
// (all start with "hyprchords:" — chain/abort/"... -> lua"/cycles) and catchall
// binds. Only binds the user deliberately described (e.g. "Apps: Terminal")
// survive, so the sheet stays meaningful instead of listing key reprs.
bool isChordMachinery(const RawBind& b) {
    if (trim(b.description).rfind("hyprchords:", 0) == 0)
        return true;
    return b.key.empty(); // catchall binds have no key
}

} // namespace

Step parseStep(const std::string& repr) {
    Step   step;
    const auto tokens = splitOn(trim(repr), '+');
    if (tokens.empty())
        return step;

    // All but the last token are modifiers; the last is the key.
    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
        KeyCap mod;
        if (modByName(tokens[i], mod))
            step.mods.push_back(mod);
    }
    step.key = normalizeKey(tokens.back());
    return step;
}

std::vector<Step> chordPrefixSteps(const std::string& submap) {
    std::vector<Step> steps;
    if (submap.rfind(kSubmapPrefix, 0) != 0)
        return steps;
    const std::string prefix = submap.substr(std::string(kSubmapPrefix).size());
    for (const auto& seg : splitOn(prefix, ';')) {
        if (!trim(seg).empty())
            steps.push_back(parseStep(seg));
    }
    return steps;
}

std::optional<Shortcut> toShortcut(const RawBind& bind, const std::string& defaultCategory) {
    if (!bind.enabled)
        return std::nullopt;
    if (!bind.hasDescription || trim(bind.description).empty())
        return std::nullopt;
    if (isChordMachinery(bind))
        return std::nullopt;

    const CategoryLabel cl = splitCategory(bind.description, defaultCategory);

    Shortcut s;
    s.category = cl.category;
    s.action   = cl.label;

    // hyprchord chain: prefix steps from the submap + this bind's key as the
    // final step (its modifiers are sticky duplicates, so the repr shows none).
    std::vector<Step> prefix = chordPrefixSteps(bind.submap);
    if (!prefix.empty()) {
        s.steps = std::move(prefix);
        s.steps.push_back({{}, normalizeKey(bind.key, bind.displayKey)});
    } else {
        s.steps.push_back({decodeModmask(bind.modmask), normalizeKey(bind.key, bind.displayKey)});
    }
    return s;
}

} // namespace hs
