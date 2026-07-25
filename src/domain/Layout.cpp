#include "Layout.hpp"

#include <algorithm>

namespace hs {

namespace {

int columnCount(const LayoutMetrics& m, size_t cardCount) {
    const double maxInner = m.screenW * m.maxPanelWidthFrac - 2 * m.panelPad;
    int          fit      = static_cast<int>((maxInner + m.colGap) / (m.columnWidth + m.colGap));
    fit                   = std::clamp(fit, 1, m.maxColumns);
    // Never make more columns than we have cards to fill them.
    return std::max(1, std::min<int>(fit, static_cast<int>(std::max<size_t>(1, cardCount))));
}

double cardHeight(const Category& c, const LayoutMetrics& m) {
    const size_t n = c.items.size();
    double       h = 2 * m.cardPadY + m.titleHeight + m.titleGap;
    if (n > 0)
        h += n * m.rowHeight + (n - 1) * m.rowGap;
    return h;
}

// The string drawn on a keycap for the configured style.
const std::string& capText(const KeyCap& cap, KeyCapStyle style) {
    return style == KeyCapStyle::Text ? cap.label : cap.glyph;
}

double keycapWidth(const std::string& glyph, const LayoutMetrics& m, const TextMeasure& measure) {
    const double textW = measure(glyph, m.glyphPx).w;
    return std::max(m.keycapMinW, textW + 2 * m.keycapPadX);
}

// Horizontal space the keycaps + joiners of a shortcut row consume (no action).
// Mirrors the x advances in the draw loop so a column can be sized to fit them.
double rowKeycapWidth(const Shortcut& s, const LayoutMetrics& m, const TextMeasure& measure) {
    double x = 0;
    for (size_t si = 0; si < s.steps.size(); ++si) {
        if (si > 0)
            x += m.keycapGap + measure("›", m.plusPx).w + m.keycapGap;
        const auto&  step   = s.steps[si];
        const size_t tokens = step.mods.size() + 1;
        for (size_t t = 0; t < tokens; ++t) {
            if (t > 0)
                x += m.keycapGap + measure("+", m.plusPx).w + m.keycapGap;
            const KeyCap& cap = t < step.mods.size() ? step.mods[t] : step.key;
            x += keycapWidth(capText(cap, m.keyStyle), m, measure);
        }
    }
    return x;
}

// Column width that fits the widest card content (keycap row + a bounded action,
// or the title), clamped to [m.columnWidth, single-column max]. Keeps the fixed
// width as a floor so normal sheets are unchanged; only grows when content needs
// it, so keycaps never march past the card's rounded edge.
double effectiveColumnWidth(const std::vector<Category>& cats, const LayoutMetrics& m,
                            const TextMeasure& measure) {
    double need = 0;
    for (const auto& c : cats) {
        need = std::max(need, measure(c.name, m.titlePx).w);
        for (const auto& s : c.items) {
            const double action = std::min(measure(s.action, m.actionPx).w, m.actionMaxW);
            need = std::max(need, rowKeycapWidth(s, m, measure) + m.keyToActionGap + action);
        }
    }
    const double maxColW = m.screenW * m.maxPanelWidthFrac - 2 * m.panelPad;
    return std::clamp(need + 2 * m.cardPadX, m.columnWidth, std::max(m.columnWidth, maxColW));
}

} // namespace

LayoutTree computeLayout(const std::vector<Category>& cats, const LayoutMetrics& mIn,
                         const TextMeasure& measure) {
    LayoutTree tree;

    // Grow the column to fit its content (keeping mIn.columnWidth as the floor).
    LayoutMetrics m = mIn;
    m.columnWidth   = effectiveColumnWidth(cats, mIn, measure);

    const int cols = columnCount(m, cats.size());

    // Greedily assign cards to the currently shortest column (balances heights
    // while keeping reading order roughly top-to-bottom, left-to-right).
    std::vector<std::vector<const Category*>> columns(cols);
    std::vector<double>                       colHeights(cols, 0.0);
    for (const auto& c : cats) {
        int shortest = 0;
        for (int i = 1; i < cols; ++i)
            if (colHeights[i] < colHeights[shortest])
                shortest = i;
        columns[shortest].push_back(&c);
        colHeights[shortest] += cardHeight(c, m) + m.cardGap;
    }

    const double contentW = cols * m.columnWidth + (cols - 1) * m.colGap;
    double       contentH = 0;
    for (double h : colHeights)
        contentH = std::max(contentH, h > 0 ? h - m.cardGap : 0.0); // trailing cardGap not real

    // Panel may be floored to a shared size (see minPanelW/H); center the content
    // block inside it so smaller pages sit in the middle rather than top-left.
    const double panelW = std::max(contentW + 2 * m.panelPad, m.minPanelW);
    const double panelH = std::max(contentH + 2 * m.panelPad, m.minPanelH);
    const double panelX = (m.screenW - panelW) / 2.0;
    const double panelY = (m.screenH - panelH) / 2.0;
    tree.panel          = {panelX, panelY, panelW, panelH};

    const double originX = panelX + m.panelPad + (panelW - 2 * m.panelPad - contentW) / 2.0;
    const double originY = panelY + m.panelPad + (panelH - 2 * m.panelPad - contentH) / 2.0;

    // Scrim (full screen) then panel background.
    tree.rects.push_back({{0, 0, m.screenW, m.screenH}, RectRole::Scrim, 0});
    tree.rects.push_back({tree.panel, RectRole::Panel, m.panelRadius});

    for (int ci = 0; ci < cols; ++ci) {
        const double colX = originX + ci * (m.columnWidth + m.colGap);
        double       y    = originY;

        for (const Category* c : columns[ci]) {
            const double h = cardHeight(*c, m);
            const Rect   card{colX, y, m.columnWidth, h};
            tree.rects.push_back({card, RectRole::Card, m.cardRadius});

            // Title.
            tree.texts.push_back({{card.x + m.cardPadX, card.y + m.cardPadY,
                                   m.columnWidth - 2 * m.cardPadX, m.titleHeight},
                                  c->name, m.titlePx, TextRole::Title});

            double rowY = card.y + m.cardPadY + m.titleHeight + m.titleGap;
            for (const auto& s : c->items) {
                double x = card.x + m.cardPadX;

                for (size_t si = 0; si < s.steps.size(); ++si) {
                    if (si > 0) {
                        // Chord separator between steps (e.g. SUPER+X › F).
                        const double sepW = measure("›", m.plusPx).w;
                        tree.texts.push_back({{x + m.keycapGap, rowY, sepW, m.rowHeight}, "›",
                                              m.plusPx, TextRole::ChordSep});
                        x += m.keycapGap + sepW + m.keycapGap;
                    }

                    // Tokens within a step: mods... then the key.
                    std::vector<const KeyCap*> tokens;
                    for (const auto& mod : s.steps[si].mods)
                        tokens.push_back(&mod);
                    tokens.push_back(&s.steps[si].key);

                    for (size_t t = 0; t < tokens.size(); ++t) {
                        if (t > 0) {
                            const double plusW = measure("+", m.plusPx).w;
                            tree.texts.push_back({{x + m.keycapGap, rowY, plusW, m.rowHeight}, "+",
                                                  m.plusPx, TextRole::Plus});
                            x += m.keycapGap + plusW + m.keycapGap;
                        }
                        const std::string& capStr = capText(*tokens[t], m.keyStyle);
                        const double       capW   = keycapWidth(capStr, m, measure);
                        const Rect cap{x, rowY + (m.rowHeight - m.keycapH) / 2.0, capW, m.keycapH};
                        tree.rects.push_back({cap, RectRole::KeyCap, m.keycapRadius});
                        tree.texts.push_back({cap, capStr, m.glyphPx, TextRole::KeyGlyph});
                        x += capW;
                    }
                }

                x += m.keyToActionGap;
                const double actionW = std::max(0.0, card.x + m.columnWidth - m.cardPadX - x);
                tree.texts.push_back(
                    {{x, rowY, actionW, m.rowHeight}, s.action, m.actionPx, TextRole::Action});

                rowY += m.rowHeight + m.rowGap;
            }

            y += h + m.cardGap;
        }
    }

    return tree;
}

std::vector<LayoutTree> computePages(const std::vector<Category>& cats, const LayoutMetrics& mIn,
                                     const TextMeasure& measure) {
    // Decide the column count once (by width) so every page looks consistent,
    // then greedily assign cards to the shortest column of the current page.
    // When a card would push the current page past the usable height, it starts
    // a new page. computeLayout re-positions each page's cards (same masonry),
    // so this only decides page boundaries.
    // Size columns to the whole set once, so every page shares one width (and so
    // the page-break column count below matches what computeLayout will use).
    LayoutMetrics m = mIn;
    m.columnWidth   = effectiveColumnWidth(cats, mIn, measure);

    const int    cols    = columnCount(m, cats.size());
    const double usableH = m.screenH * m.maxPanelHeightFrac - 2 * m.panelPad;

    std::vector<std::vector<const Category*>> pages;
    std::vector<double>                       colH;
    const auto startPage = [&] {
        pages.emplace_back();
        colH.assign(cols, 0.0);
    };
    startPage();

    for (const auto& c : cats) {
        const double h = cardHeight(c, m);

        int shortest = 0;
        for (int i = 1; i < cols; ++i)
            if (colH[i] < colH[shortest])
                shortest = i;

        const bool   pageEmpty = std::all_of(colH.begin(), colH.end(), [](double v) { return v == 0.0; });
        const double added     = (colH[shortest] > 0 ? m.cardGap : 0.0) + h;
        if (!pageEmpty && colH[shortest] + added > usableH) {
            startPage();
            shortest = 0;
        }
        colH[shortest] += (colH[shortest] > 0 ? m.cardGap : 0.0) + h;
        pages.back().push_back(&c);
    }

    std::vector<std::vector<Category>> subsets(pages.size());
    for (size_t i = 0; i < pages.size(); ++i)
        for (const Category* c : pages[i])
            subsets[i].push_back(*c);

    const auto build = [&](const LayoutMetrics& metrics) {
        std::vector<LayoutTree> out;
        out.reserve(subsets.size());
        for (size_t i = 0; i < subsets.size(); ++i) {
            LayoutTree t = computeLayout(subsets[i], metrics, measure);
            t.pageIndex  = static_cast<int>(i);
            t.pageCount  = static_cast<int>(subsets.size());
            out.push_back(std::move(t));
        }
        return out;
    };

    std::vector<LayoutTree> trees = build(m);
    if (trees.size() <= 1)
        return trees;

    // Pin every page to the largest panel so flipping pages doesn't resize the
    // window; content stays centered within it (see computeLayout).
    LayoutMetrics uniform = m;
    for (const auto& t : trees) {
        uniform.minPanelW = std::max(uniform.minPanelW, t.panel.w);
        uniform.minPanelH = std::max(uniform.minPanelH, t.panel.h);
    }
    return build(uniform);
}

} // namespace hs
