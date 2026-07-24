#include "OverlayRenderer.hpp"

#include <algorithm>

#include <src/helpers/Color.hpp>
#include <src/render/Renderer.hpp>
#include <src/render/pass/RectPassElement.hpp>
#include <src/render/pass/TexPassElement.hpp>

namespace hs {

namespace {

struct RGB {
    float r, g, b;
};

// Flat dark theme. Alpha is folded in per-draw via the fade value.
CHyprColor rectColor(RectRole role, float a) {
    switch (role) {
        case RectRole::Scrim: return CHyprColor(0.0, 0.0, 0.0, 0.45 * a);
        case RectRole::Panel: return CHyprColor(0.11, 0.12, 0.15, 0.94 * a);
        case RectRole::Card:  return CHyprColor(0.16, 0.17, 0.21, 0.96 * a);
        case RectRole::KeyCap:return CHyprColor(0.24, 0.25, 0.30, 1.0 * a);
    }
    return CHyprColor(0.0, 0.0, 0.0, a);
}

RGB textColor(TextRole role) {
    switch (role) {
        case TextRole::Title:    return {0.55, 0.78, 1.0};
        case TextRole::KeyGlyph: return {0.96, 0.96, 0.98};
        case TextRole::Plus:     return {0.5, 0.52, 0.58};
        case TextRole::ChordSep: return {0.55, 0.78, 1.0};
        case TextRole::Action:   return {0.82, 0.84, 0.88};
    }
    return {1.0, 1.0, 1.0};
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
        d.color = rectColor(rn.role, a);
        d.round = static_cast<int>(rn.radius * scale);
        pass.add(makeUnique<CRectPassElement>(d));
    }

    for (const TextNode& tn : tree.texts) {
        if (tn.text.empty())
            continue;

        const RGB col = textColor(tn.role);
        Vec2      sz;
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
}

} // namespace hs
