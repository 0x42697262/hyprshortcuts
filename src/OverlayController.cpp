#include "OverlayController.hpp"

#include <algorithm>
#include <chrono>

#include <lua.hpp>

#include <src/Compositor.hpp>
#include <src/devices/IKeyboard.hpp>
#include <src/event/EventBus.hpp>
#include <src/plugins/PluginAPI.hpp>
#include <src/render/Renderer.hpp>

#include "BindSource.hpp"
#include "commands/Commands.hpp"
#include "domain/BindModel.hpp"
#include "domain/Grouping.hpp"

namespace hs {

namespace {
constexpr double kFadeSeconds = 0.12;
// Ignore key events for a moment after opening, so the very keypress that
// triggered the toggle dispatcher doesn't immediately close the overlay.
constexpr double kOpenGuardMs = 200.0;
} // namespace

bool OverlayController::init(HANDLE handle) {
    PHANDLE = handle;

    registerDefaultCommands(m_registry);

    if (!registerConfig(handle))
        return false;

    auto* bus = Event::bus().get();
    if (!bus)
        return false;

    m_renderPreListener = bus->m_events.render.pre.listen(
        [this](PHLMONITOR mon) { m_renderMonitor = mon; });

    m_renderStageListener = bus->m_events.render.stage.listen(
        [this](eRenderStage stage) { onRenderStage(stage); });

    m_keyListener = bus->m_events.input.keyboard.key.listen(
        [this](const IKeyboard::SKeyEvent& ev, Event::SCallbackInfo& info) {
            if (ev.state != WL_KEYBOARD_KEY_STATE_PRESSED)
                return;
            if (!m_visible)
                return;
            if (nowMs() - m_openedAtMs < kOpenGuardMs)
                return;
            hide();
            info.cancelled = true; // swallow the dismiss key
        });

    m_reloadedListener = bus->m_events.config.reloaded.listen([this] {
        readConfig();
        refresh();
    });

    // NOTE: no addDispatcherV2 — a plugin dispatcher named "hyprshortcuts:toggle"
    // is exposed by Hyprland at hl.plugin.hyprshortcuts.toggle, which would
    // SHADOW the Lua function of the same name. On Lua-native Hyprland the Lua
    // functions are the interface, so we register only those.

    // Lua config API: hl.plugin.hyprshortcuts.{toggle,refresh,close}. Hyprland
    // does NOT drop plugin Lua functions when a plugin unloads, so a stale
    // registration from a previous load would make addLuaFunction fail (leaving
    // a dangling function). Remove first, then add, so reloads re-register
    // cleanly. Non-capturing lambdas convert to the plain function pointer.
    static const std::pair<const char*, PLUGIN_LUA_FN> kLuaFns[] = {
        {"toggle", [](lua_State*) -> int { g_overlay.dispatchCommand("toggle"); return 0; }},
        {"refresh", [](lua_State*) -> int { g_overlay.dispatchCommand("refresh"); return 0; }},
        {"close", [](lua_State*) -> int { g_overlay.dispatchCommand("close"); return 0; }},
    };
    for (const auto& [name, fn] : kLuaFns) {
        HyprlandAPI::removeLuaFunction(handle, "hyprshortcuts", name);
        HyprlandAPI::addLuaFunction(handle, "hyprshortcuts", name, fn);
    }

    readConfig();
    rebuildModel();
    return true;
}

void OverlayController::dispatchCommand(const std::string& name) {
    m_registry.dispatch(name, *this);
}

bool OverlayController::registerConfig(HANDLE handle) {
    namespace CV = Config::Values;
    using CV::makeConfigValue;

    // Appearance knobs live under plugin:hyprshortcuts:*. Colours are ARGB.
    m_cfgKeyStyle = makeConfigValue<CV::CStringValue>(
        "plugin:hyprshortcuts:key_style", "keycap contents: \"icons\" (glyphs) or \"text\" (labels)",
        "icons", CV::SStringValueOptions{.validator = CV::strChoice({"icons", "text"})});
    m_cfgFont = makeConfigValue<CV::CStringValue>("plugin:hyprshortcuts:font_family",
                                                  "font family for all overlay text", "Sans");
    m_cfgMaxColumns = makeConfigValue<CV::CIntValue>("plugin:hyprshortcuts:max_columns",
                                                     "maximum number of columns of category cards", 4,
                                                     CV::SIntValueOptions{.min = 1, .max = 8});

    m_cfgScrim  = makeConfigValue<CV::CColorValue>("plugin:hyprshortcuts:scrim_color",
                                                  "full-screen dim behind the panel", 0x73000000);
    m_cfgPanel  = makeConfigValue<CV::CColorValue>("plugin:hyprshortcuts:panel_color",
                                                  "panel background", 0xF01C1F26);
    m_cfgCard   = makeConfigValue<CV::CColorValue>("plugin:hyprshortcuts:card_color",
                                                 "category card background", 0xF52A2C36);
    m_cfgKeycap = makeConfigValue<CV::CColorValue>("plugin:hyprshortcuts:keycap_color",
                                                   "keycap background", 0xFF3D404D);
    m_cfgTitle  = makeConfigValue<CV::CColorValue>("plugin:hyprshortcuts:title_color",
                                                  "category title text", 0xFF8CC7FF);
    m_cfgAction = makeConfigValue<CV::CColorValue>("plugin:hyprshortcuts:action_color",
                                                   "action/description text", 0xFFD1D6E0);
    m_cfgKey    = makeConfigValue<CV::CColorValue>("plugin:hyprshortcuts:key_color",
                                                "keycap glyph/label text", 0xFFF5F5FA);
    m_cfgSeparator = makeConfigValue<CV::CColorValue>(
        "plugin:hyprshortcuts:separator_color", "the \"+\" joiner and chord \"›\" text", 0xFF808594);

    m_cfgPanelRounding  = makeConfigValue<CV::CIntValue>("plugin:hyprshortcuts:panel_rounding",
                                                        "panel corner radius (px)", 18,
                                                        CV::SIntValueOptions{.min = 0});
    m_cfgCardRounding   = makeConfigValue<CV::CIntValue>("plugin:hyprshortcuts:card_rounding",
                                                       "card corner radius (px)", 12,
                                                       CV::SIntValueOptions{.min = 0});
    m_cfgKeycapRounding = makeConfigValue<CV::CIntValue>("plugin:hyprshortcuts:keycap_rounding",
                                                         "keycap corner radius (px)", 6,
                                                         CV::SIntValueOptions{.min = 0});

    const SP<Config::Values::IValue> all[] = {
        m_cfgKeyStyle,      m_cfgFont,          m_cfgMaxColumns, m_cfgScrim,        m_cfgPanel,
        m_cfgCard,          m_cfgKeycap,        m_cfgTitle,      m_cfgAction,       m_cfgKey,
        m_cfgSeparator,     m_cfgPanelRounding, m_cfgCardRounding, m_cfgKeycapRounding};
    for (const auto& v : all) {
        if (!HyprlandAPI::addConfigValueV2(handle, v))
            return false;
    }
    return true;
}

void OverlayController::readConfig() {
    // ARGB integer -> straight RGBA floats.
    const auto rgba = [](int64_t argb) -> RGBA {
        return {static_cast<float>((argb >> 16) & 0xFF) / 255.0f,
                static_cast<float>((argb >> 8) & 0xFF) / 255.0f,
                static_cast<float>(argb & 0xFF) / 255.0f,
                static_cast<float>((argb >> 24) & 0xFF) / 255.0f};
    };

    // Layout metrics (base; screen size is filled per-monitor in rebuildLayout).
    m_metrics.keyStyle =
        std::string{m_cfgKeyStyle->value()} == "text" ? KeyCapStyle::Text : KeyCapStyle::Icons;
    m_metrics.maxColumns    = static_cast<int>(m_cfgMaxColumns->value());
    m_metrics.panelRadius   = static_cast<double>(m_cfgPanelRounding->value());
    m_metrics.cardRadius    = static_cast<double>(m_cfgCardRounding->value());
    m_metrics.keycapRadius  = static_cast<double>(m_cfgKeycapRounding->value());

    // Theme.
    Theme theme;
    theme.scrim     = rgba(m_cfgScrim->value());
    theme.panel     = rgba(m_cfgPanel->value());
    theme.card      = rgba(m_cfgCard->value());
    theme.keycap    = rgba(m_cfgKeycap->value());
    theme.title     = rgba(m_cfgTitle->value());
    theme.action    = rgba(m_cfgAction->value());
    theme.keyGlyph  = rgba(m_cfgKey->value());
    theme.separator = rgba(m_cfgSeparator->value());
    m_renderer.setTheme(theme);

    // Font (setFont drops the texture cache only if the family actually changed).
    m_text.setFont(std::string{m_cfgFont->value()});
}

void OverlayController::shutdown() {
    m_renderPreListener.reset();
    m_renderStageListener.reset();
    m_keyListener.reset();
    m_reloadedListener.reset();
    if (PHANDLE) {
        for (const char* name : {"toggle", "refresh", "close"})
            HyprlandAPI::removeLuaFunction(PHANDLE, "hyprshortcuts", name);
    }
    m_text.clear();
    m_visible = false;
    m_anim    = 0.0;
}

double OverlayController::nowMs() const {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

void OverlayController::rebuildModel() {
    std::vector<Shortcut> shortcuts;
    for (const auto& raw : readLiveBinds()) {
        if (auto s = toShortcut(raw))
            shortcuts.push_back(std::move(*s));
    }
    m_cats = groupByCategory(dedupeShortcuts(shortcuts));
}

void OverlayController::rebuildLayout(PHLMONITOR mon) {
    if (!mon)
        return;
    LayoutMetrics m = m_metrics; // keyStyle, columns, roundings from config
    m.screenW       = mon->m_size.x;
    m.screenH       = mon->m_size.y;
    m_treeSize      = {m.screenW, m.screenH};
    m_tree          = computeLayout(
        m_cats, m, [this](const std::string& t, double px) { return m_text.measure(t, px); });
}

void OverlayController::show() {
    if (!g_pCompositor)
        return;
    PHLMONITOR mon = g_pCompositor->getMonitorFromCursor();
    if (!mon)
        return;

    m_target = mon;
    rebuildLayout(mon);

    m_visible     = true;
    m_openedAtMs  = nowMs();
    m_lastFrameMs = nowMs();
    damageTarget();
}

void OverlayController::hide() {
    m_visible = false;
    damageTarget(); // keep frames coming so the fade-out plays
}

void OverlayController::toggle() {
    if (m_visible)
        hide();
    else
        show();
}

void OverlayController::refresh() {
    rebuildModel();
    if (m_visible) {
        if (auto mon = m_target.lock())
            rebuildLayout(mon);
        damageTarget();
    }
}

bool OverlayController::isVisible() const {
    return m_visible;
}

void OverlayController::damageTarget() {
    if (auto mon = m_target.lock(); mon && g_pHyprRenderer)
        g_pHyprRenderer->damageMonitor(mon);
}

void OverlayController::onRenderStage(eRenderStage stage) {
    const bool active = m_visible || m_anim > 0.001;
    // Add our pass elements while the render pass is still being built (after
    // windows). RENDER_LAST_MOMENT is post-pass-execute, so nothing composites.
    if (!active || stage != RENDER_POST_WINDOWS)
        return;

    PHLMONITOR cur = m_renderMonitor.lock();
    PHLMONITOR tgt = m_target.lock();
    if (!cur || !tgt || cur != tgt)
        return;

    // Advance the fade based on elapsed wall time.
    const double now = nowMs();
    const double dt  = std::clamp((now - m_lastFrameMs) / 1000.0, 0.0, 0.1);
    m_lastFrameMs    = now;
    const double step = dt / kFadeSeconds;
    m_anim            = m_visible ? std::min(1.0, m_anim + step) : std::max(0.0, m_anim - step);

    if (m_anim <= 0.001 && !m_visible)
        return;

    // Re-flow if the monitor resolution changed while the overlay is up.
    if (cur->m_size.x != m_treeSize.w || cur->m_size.y != m_treeSize.h)
        rebuildLayout(cur);

    m_renderer.draw(m_tree, cur, m_anim, m_text);

    // Only keep requesting frames while the fade is still in motion.
    const bool animating = (m_visible && m_anim < 1.0) || (!m_visible && m_anim > 0.0);
    if (animating && g_pHyprRenderer)
        g_pHyprRenderer->damageMonitor(cur);
}

} // namespace hs
