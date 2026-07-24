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

double keycapWidth(const std::string& glyph, const LayoutMetrics& m, const TextMeasure& measure) {
    const double textW = measure(glyph, m.glyphPx).w;
    return std::max(m.keycapMinW, textW + 2 * m.keycapPadX);
}

} // namespace

LayoutTree computeLayout(const std::vector<Category>& cats, const LayoutMetrics& m,
                         const TextMeasure& measure) {
    LayoutTree tree;

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
    double       maxColH  = 0;
    for (double h : colHeights)
        maxColH = std::max(maxColH, h > 0 ? h - m.cardGap : 0.0); // trailing cardGap not real

    const double panelW = contentW + 2 * m.panelPad;
    const double panelH = maxColH + 2 * m.panelPad;
    const double panelX = (m.screenW - panelW) / 2.0;
    const double panelY = (m.screenH - panelH) / 2.0;
    tree.panel          = {panelX, panelY, panelW, panelH};

    // Scrim (full screen) then panel background.
    tree.rects.push_back({{0, 0, m.screenW, m.screenH}, RectRole::Scrim, 0});
    tree.rects.push_back({tree.panel, RectRole::Panel, m.panelRadius});

    for (int ci = 0; ci < cols; ++ci) {
        const double colX = panelX + m.panelPad + ci * (m.columnWidth + m.colGap);
        double       y    = panelY + m.panelPad;

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

                // Build the token sequence: mods... then the key.
                std::vector<const KeyCap*> tokens;
                for (const auto& mod : s.mods)
                    tokens.push_back(&mod);
                tokens.push_back(&s.key);

                for (size_t t = 0; t < tokens.size(); ++t) {
                    if (t > 0) {
                        // "+" joiner.
                        const double plusW = measure("+", m.plusPx).w;
                        tree.texts.push_back({{x + m.keycapGap, rowY, plusW, m.rowHeight}, "+",
                                              m.plusPx, TextRole::Plus});
                        x += m.keycapGap + plusW + m.keycapGap;
                    }
                    const double capW = keycapWidth(tokens[t]->glyph, m, measure);
                    const Rect   cap{x, rowY + (m.rowHeight - m.keycapH) / 2.0, capW, m.keycapH};
                    tree.rects.push_back({cap, RectRole::KeyCap, m.keycapRadius});
                    tree.texts.push_back({cap, tokens[t]->glyph, m.glyphPx, TextRole::KeyGlyph});
                    x += capW;
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

} // namespace hs
