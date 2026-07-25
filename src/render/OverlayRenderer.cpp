#include "OverlayRenderer.hpp"

#include <algorithm>
#include <format>

#include <src/helpers/Color.hpp>
#include <src/render/Renderer.hpp>
#include <src/render/pass/RectPassElement.hpp>
#include <src/render/pass/TexPassElement.hpp>

namespace hs {

namespace {

constexpr double kPageIndicatorGap = 10.0; // gap below the panel, in layout px
constexpr double kPageDotSize      = 7.0;  // diameter of a page dot, in layout px
constexpr double kPageDotGap       = 7.0;  // gap between page dots, in layout px
constexpr int    kMaxPageDots      = 12;   // beyond this, show a numeric "i / n"
constexpr double kPageIndicatorPx  = 13.0; // font size of the numeric fallback

// Rect fill from the theme; the colour's alpha is folded with the fade value.
CHyprColor rectColor(const Theme& t, RectRole role, float a) {
    RGBA c;
    switch (role) {
        case RectRole::Scrim:  c = t.scrim; break;
        case RectRole::Panel:  c = t.panel; break;
        case RectRole::Card:   c = t.card; break;
        case RectRole::KeyCap: c = t.keycap; break;
    }
    return CHyprColor(c.r, c.g, c.b, c.a * a);
}

// Text colour from the theme (rgb; alpha comes from the fade at draw time).
RGBA textColor(const Theme& t, TextRole role) {
    switch (role) {
        case TextRole::Title:    return t.title;
        case TextRole::KeyGlyph: return t.keyGlyph;
        case TextRole::Plus:     return t.separator;
        case TextRole::ChordSep: return t.separator;
        case TextRole::Action:   return t.action;
    }
    return {1.0f, 1.0f, 1.0f};
}

CBox scaledBox(const Rect& r, double s) {
    return CBox{r.x * s, r.y * s, r.w * s, r.h * s};
}

bool isCentered(TextRole role) {
    return role == TextRole::KeyGlyph || role == TextRole::Plus || role == TextRole::ChordSep;
}

} // namespace

void OverlayRenderer::draw(const LayoutTree& tree, PHLMONITOR monitor, double fade,
                           TextRenderer& text) {
    if (!monitor || !g_pHyprRenderer)
        return;

    const float  a     = static_cast<float>(std::clamp(fade, 0.0, 1.0));
    const double scale = monitor->m_scale;
    auto&        pass  = g_pHyprRenderer->m_renderPass;

    for (const RectNode& rn : tree.rects) {
        CRectPassElement::SRectData d;
        d.box   = scaledBox(rn.rect, scale);
        d.color = rectColor(m_theme, rn.role, a);
        d.round = static_cast<int>(rn.radius * scale);
        pass.add(makeUnique<CRectPassElement>(d));
    }

    for (const TextNode& tn : tree.texts) {
        if (tn.text.empty())
            continue;

        const RGBA   col      = textColor(m_theme, tn.role);
        const CBox   area     = scaledBox(tn.rect, scale);
        const bool   centered = isCentered(tn.role);
        // Left-aligned text (title, action) is clamped to its box and ellipsized;
        // keycap glyphs are pre-sized to fit, so they render at natural width.
        const double maxW = centered ? 0.0 : area.w;
        Vec2         sz;
        // Render at physical resolution (px * scale) so text stays crisp.
        auto tex = text.textureFor(tn.text, tn.px * scale, col.r, col.g, col.b, sz, maxW);
        if (!tex)
            continue;

        const double x = centered ? area.x + (area.w - sz.w) / 2.0 : area.x;
        const double y    = area.y + (area.h - sz.h) / 2.0;

        CTexPassElement::SRenderData d;
        d.tex = tex;
        d.box = CBox{x, y, sz.w, sz.h};
        d.a   = a;
        pass.add(makeUnique<CTexPassElement>(d));
    }

    // Page indicator below the panel when paginated: a centered row of dots
    // (filled for the current page), falling back to an "i / n" label when there
    // are too many pages to show as dots.
    if (tree.pageCount > 1) {
        const double cx = (tree.panel.x + tree.panel.w / 2.0) * scale;
        const double y  = (tree.panel.y + tree.panel.h + kPageIndicatorGap) * scale;

        if (tree.pageCount <= kMaxPageDots) {
            const double totalW =
                (tree.pageCount * kPageDotSize + (tree.pageCount - 1) * kPageDotGap) * scale;
            double x = cx - totalW / 2.0;
            for (int i = 0; i < tree.pageCount; ++i) {
                const bool current = i == tree.pageIndex;
                const RGBA c       = current ? m_theme.title : m_theme.separator;
                CRectPassElement::SRectData d;
                d.box   = CBox{x, y, kPageDotSize * scale, kPageDotSize * scale};
                d.color = CHyprColor(c.r, c.g, c.b, (current ? 1.0f : 0.4f) * c.a * a);
                d.round = static_cast<int>((kPageDotSize / 2.0) * scale);
                pass.add(makeUnique<CRectPassElement>(d));
                x += (kPageDotSize + kPageDotGap) * scale;
            }
        } else {
            const std::string label = std::format("{} / {}", tree.pageIndex + 1, tree.pageCount);
            const RGBA        col   = m_theme.separator;
            Vec2              sz;
            auto tex = text.textureFor(label, kPageIndicatorPx * scale, col.r, col.g, col.b, sz);
            if (tex) {
                CTexPassElement::SRenderData d;
                d.tex = tex;
                d.box = CBox{cx - sz.w / 2.0, y, sz.w, sz.h};
                d.a   = a;
                pass.add(makeUnique<CTexPassElement>(d));
            }
        }
    }
}

} // namespace hs
