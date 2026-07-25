#include "OverlayRenderer.hpp"

#include <algorithm>
#include <format>

#include <src/helpers/Color.hpp>
#include <src/render/Renderer.hpp>
#include <src/render/pass/RectPassElement.hpp>
#include <src/render/pass/TexPassElement.hpp>

namespace hs {

namespace {

constexpr double kPageIndicatorPx  = 13.0; // font size of the "i / n" indicator
constexpr double kPageIndicatorGap = 8.0;  // gap below the panel, in layout px

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

        const RGBA col = textColor(m_theme, tn.role);
        Vec2       sz;
        // Render at physical resolution (px * scale) so text stays crisp.
        auto tex = text.textureFor(tn.text, tn.px * scale, col.r, col.g, col.b, sz);
        if (!tex)
            continue;

        const CBox   area = scaledBox(tn.rect, scale);
        const double x    = isCentered(tn.role) ? area.x + (area.w - sz.w) / 2.0 : area.x;
        const double y    = area.y + (area.h - sz.h) / 2.0;

        CTexPassElement::SRenderData d;
        d.tex = tex;
        d.box = CBox{x, y, sz.w, sz.h};
        d.a   = a;
        pass.add(makeUnique<CTexPassElement>(d));
    }

    // Page indicator ("2 / 3"), centered just below the panel, when paginated.
    if (tree.pageCount > 1) {
        const std::string label = std::format("{} / {}", tree.pageIndex + 1, tree.pageCount);
        const RGBA        col   = m_theme.separator;
        Vec2              sz;
        auto tex = text.textureFor(label, kPageIndicatorPx * scale, col.r, col.g, col.b, sz);
        if (tex) {
            const double cx = (tree.panel.x + tree.panel.w / 2.0) * scale;
            const double y  = (tree.panel.y + tree.panel.h + kPageIndicatorGap) * scale;
            CTexPassElement::SRenderData d;
            d.tex = tex;
            d.box = CBox{cx - sz.w / 2.0, y, sz.w, sz.h};
            d.a   = a;
            pass.add(makeUnique<CTexPassElement>(d));
        }
    }
}

} // namespace hs
