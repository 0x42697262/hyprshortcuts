#pragma once

#include <string>
#include <unordered_map>

#include <src/render/Texture.hpp>

#include "../domain/Layout.hpp" // Vec2

namespace hs {

// Rasterises text with pango/cairo and caches the resulting GL textures.
//
// measure() needs no GL context (pure cairo) and is what Layout uses.
// textureFor() creates/caches a CGLTexture and MUST be called with a live GL
// context (i.e. from inside the render hook).
class TextRenderer {
  public:
    explicit TextRenderer(std::string fontFamily = "Sans");

    // Change the font family. Drops the cached textures so the new font takes
    // effect on the next draw. No-op if the family is unchanged.
    void setFont(std::string fontFamily);

    // Pixel size of `text` at `px`. Safe to call without a GL context.
    // maxWidthPx > 0 constrains the layout and ellipsizes (…) at that width.
    Vec2 measure(const std::string& text, double px, double maxWidthPx = 0) const;

    // Cached texture of `text` at `px` in the given colour (0..1). The cached
    // texture's pixel size is written to outSize. Requires a live GL context.
    // maxWidthPx > 0 ellipsizes the text to that width so it never exceeds its box.
    SP<Render::ITexture> textureFor(const std::string& text, double px, float r, float g, float b,
                            Vec2& outSize, double maxWidthPx = 0);

    // Drop all cached textures (e.g. on font change / shutdown).
    void clear();

  private:
    struct Entry {
        SP<Render::ITexture> tex;
        Vec2         size;
    };

    std::string                          m_font;
    std::unordered_map<std::string, Entry> m_cache;
};

} // namespace hs
