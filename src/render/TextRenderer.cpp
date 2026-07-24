#include "TextRenderer.hpp"

#include <algorithm>
#include <cstdio>

#include <cairo/cairo.h>
#include <drm_fourcc.h>
#include <pango/pangocairo.h>

#include <src/render/gl/GLTexture.hpp>

namespace hs {

namespace {

// Build a configured PangoLayout on the given cairo context. Caller owns the
// returned layout (g_object_unref).
PangoLayout* makeLayout(cairo_t* cr, const std::string& font, const std::string& text, double px) {
    PangoLayout*          layout = pango_cairo_create_layout(cr);
    PangoFontDescription* desc   = pango_font_description_from_string(font.c_str());
    pango_font_description_set_absolute_size(desc, px * PANGO_SCALE);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);
    pango_layout_set_text(layout, text.c_str(), -1);
    return layout;
}

std::string cacheKey(const std::string& text, double px, float r, float g, float b) {
    // px is quantised to an int; colours to bytes — plenty for de-duping.
    char buf[64];
    std::snprintf(buf, sizeof(buf), "|%d|%02x%02x%02x", static_cast<int>(px + 0.5),
                  static_cast<int>(r * 255), static_cast<int>(g * 255), static_cast<int>(b * 255));
    return text + buf;
}

} // namespace

TextRenderer::TextRenderer(std::string fontFamily) : m_font(std::move(fontFamily)) {}

Vec2 TextRenderer::measure(const std::string& text, double px) const {
    cairo_surface_t* tmp = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t*         cr  = cairo_create(tmp);
    PangoLayout*     lay = makeLayout(cr, m_font, text, px);

    int w = 0, h = 0;
    pango_layout_get_pixel_size(lay, &w, &h);

    g_object_unref(lay);
    cairo_destroy(cr);
    cairo_surface_destroy(tmp);
    return {static_cast<double>(w), static_cast<double>(h)};
}

SP<Render::ITexture> TextRenderer::textureFor(const std::string& text, double px, float r, float g, float b,
                                      Vec2& outSize) {
    const std::string key = cacheKey(text, px, r, g, b);
    if (auto it = m_cache.find(key); it != m_cache.end()) {
        outSize = it->second.size;
        return it->second.tex;
    }

    // Measure, then rasterise into an ARGB32 surface of exactly that size.
    const Vec2 size = measure(text, px);
    const int  w    = std::max(1, static_cast<int>(size.w + 0.5));
    const int  h    = std::max(1, static_cast<int>(size.h + 0.5));

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t*         cr      = cairo_create(surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, r, g, b, 1.0);

    PangoLayout* lay = makeLayout(cr, m_font, text, px);
    pango_cairo_show_layout(cr, lay);
    g_object_unref(lay);

    cairo_surface_flush(surface);
    uint8_t* data   = cairo_image_surface_get_data(surface);
    const int stride = cairo_image_surface_get_stride(surface);

    // cairo ARGB32 is premultiplied, native-endian => DRM_FORMAT_ARGB8888 on LE.
    // keepDataCopy=true so the texture owns its pixels once we free the surface.
    auto tex = makeShared<Render::GL::CGLTexture>(DRM_FORMAT_ARGB8888, data, stride,
                                                  Vector2D{(double)w, (double)h}, true);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    outSize          = {(double)w, (double)h};
    m_cache[key]     = {tex, outSize};
    return tex;
}

void TextRenderer::clear() {
    m_cache.clear();
}

} // namespace hs
