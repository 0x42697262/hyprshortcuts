#pragma once

#include <vector>

#include <hyprutils/signal/Signal.hpp>

#include <src/SharedDefs.hpp>
#include <src/desktop/DesktopTypes.hpp>

#include "commands/CommandContext.hpp"
#include "commands/CommandRegistry.hpp"
#include "domain/Layout.hpp"
#include "domain/Types.hpp"
#include "globals.hpp"
#include "render/OverlayRenderer.hpp"
#include "render/TextRenderer.hpp"

namespace hs {

// The plugin's central singleton (cf. hyprchord's g_chordManager). Owns the
// render/key/reload subscriptions, the command dispatchers, and the visibility/
// fade state, and implements ICommandContext so commands act on it.
class OverlayController : public ICommandContext {
  public:
    bool init(HANDLE handle);
    void shutdown();

    // ICommandContext
    void show() override;
    void hide() override;
    void toggle() override;
    void refresh() override;
    bool isVisible() const override;

  private:
    void   rebuildModel();                // live binds -> grouped categories
    void   rebuildLayout(PHLMONITOR mon); // categories -> LayoutTree for a monitor
    void   onRenderStage(eRenderStage stage);
    void   onKeyPressed();
    void   damageTarget();
    double nowMs() const;

    CommandRegistry m_registry;
    TextRenderer    m_text;
    OverlayRenderer m_renderer;

    std::vector<Category> m_cats;
    LayoutTree            m_tree;

    bool          m_visible     = false;
    double        m_anim        = 0.0; // actual opacity 0..1 (drives fade)
    double        m_lastFrameMs = 0.0;
    double        m_openedAtMs  = 0.0;
    PHLMONITORREF m_target;        // monitor to draw the overlay on
    PHLMONITORREF m_renderMonitor; // monitor currently being rendered

    Hyprutils::Signal::CHyprSignalListener m_renderPreListener;
    Hyprutils::Signal::CHyprSignalListener m_renderStageListener;
    Hyprutils::Signal::CHyprSignalListener m_keyListener;
    Hyprutils::Signal::CHyprSignalListener m_reloadedListener;
};

inline OverlayController g_overlay;

} // namespace hs
