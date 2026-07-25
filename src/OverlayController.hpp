#pragma once

#include <vector>

#include <hyprutils/signal/Signal.hpp>

#include <src/SharedDefs.hpp>
#include <src/config/values/ConfigValues.hpp>
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

    // Run a registered command by name (used by the dispatchers and the Lua
    // functions). No-op if the name is unknown.
    void dispatchCommand(const std::string& name);

    // ICommandContext
    void show() override;
    void hide() override;
    void toggle() override;
    void refresh() override;
    bool isVisible() const override;

  private:
    bool   registerConfig(HANDLE handle);  // create + register all config values
    void   readConfig();                   // config values -> metrics/theme/font
    void   rebuildModel();                 // live binds -> grouped categories
    void   rebuildLayout(PHLMONITOR mon);  // categories -> pages for a monitor
    void   flipPage(int delta);            // advance the shown page (wraps)
    bool   onKeyEvent(uint32_t keycode);   // returns true if the key was consumed for paging
    void   onRenderStage(eRenderStage stage);
    void   onKeyPressed();
    void   damageTarget();
    double nowMs() const;

    CommandRegistry m_registry;
    TextRenderer    m_text;
    OverlayRenderer m_renderer;

    std::vector<Category>   m_cats;
    std::vector<LayoutTree> m_pages;     // one LayoutTree per page (>=1)
    size_t                  m_page = 0;  // currently shown page
    LayoutMetrics           m_metrics;   // base metrics from config (no screen size)
    Vec2                    m_treeSize;  // screen size the current pages were built for

    // Config values (plugin:hyprshortcuts:*). Registered in init, read on reload.
    SP<Config::Values::CStringValue> m_cfgKeyStyle;
    SP<Config::Values::CStringValue> m_cfgFont;
    SP<Config::Values::CIntValue>    m_cfgMaxColumns;
    SP<Config::Values::CColorValue>  m_cfgScrim, m_cfgPanel, m_cfgCard, m_cfgKeycap;
    SP<Config::Values::CColorValue>  m_cfgTitle, m_cfgAction, m_cfgKey, m_cfgSeparator;
    SP<Config::Values::CIntValue>    m_cfgPanelRounding, m_cfgCardRounding, m_cfgKeycapRounding;

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
