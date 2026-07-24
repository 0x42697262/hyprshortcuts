#include "OverlayRenderer.hpp"

#include <algorithm>
#include <tuple>

#include <src/helpers/Color.hpp>
#include <src/render/OpenGL.hpp>

namespace hs {

namespace {

using GL = Render::GL::CHyprOpenGLImpl;

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
        case TextRole::Title:    return {0.55, 0.78, 1.0};  // accent blue
        case TextRole::KeyGlyph: return {0.96, 0.96, 0.98}; // near-white
        case TextRole::Plus:     return {0.5, 0.52, 0.58};  // dim
        case TextRole::Action:   return {0.82, 0.84, 0.88}; // light grey
    }
    return {1.0, 1.0, 1.0};
}

CBox scaledBox(const Rect& r, double s) {
    return CBox{r.x * s, r.y * s, r.w * s, r.h * s};
}

bool isCentered(TextRole role) {
    return role == TextRole::KeyGlyph || role == TextRole::Plus;
}

} // namespace

void OverlayRenderer::draw(const LayoutTree& tree, PHLMONITOR monitor, double fade,
                           TextRenderer& text) {
    if (!monitor)
        return;

    const float  a     = static_cast<float>(std::clamp(fade, 0.0, 1.0));
    const double scale = monitor->m_scale;
    auto&        gl    = Render::GL::g_pHyprOpenGL;
    if (!gl)
        return;

    gl->blend(true);

    for (const RectNode& rn : tree.rects) {
        GL::SRectRenderData d;
        d.round = static_cast<int>(rn.radius * scale);
        gl->renderRect(scaledBox(rn.rect, scale), rectColor(rn.role, a), d);
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

        GL::STextureRenderData td;
        td.a = a;
        gl->renderTexture(tex, CBox{x, y, sz.w, sz.h}, td);
    }
}

} // namespace hs
