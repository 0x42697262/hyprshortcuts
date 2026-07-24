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

    m_reloadedListener = bus->m_events.config.reloaded.listen([this] { refresh(); });

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

    rebuildModel();
    return true;
}

void OverlayController::dispatchCommand(const std::string& name) {
    m_registry.dispatch(name, *this);
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
    LayoutMetrics m;
    m.screenW = mon->m_size.x;
    m.screenH = mon->m_size.y;
    m_tree    = computeLayout(
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

    m_renderer.draw(m_tree, cur, m_anim, m_text);

    // Only keep requesting frames while the fade is still in motion.
    const bool animating = (m_visible && m_anim < 1.0) || (!m_visible && m_anim > 0.0);
    if (animating && g_pHyprRenderer)
        g_pHyprRenderer->damageMonitor(cur);
}

} // namespace hs
