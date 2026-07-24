#include <src/plugins/PluginAPI.hpp>

#include <stdexcept>

#include "OverlayController.hpp"
#include "globals.hpp"

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    if (!hs::g_overlay.init(handle)) {
        HyprlandAPI::addNotificationV2(handle,
                                       {{"text", std::string{"[hyprshortcuts] failed to initialize"}},
                                        {"time", uint64_t{10000}},
                                        {"color", CHyprColor{0}},
                                        {"icon", ICON_ERROR}});
        throw std::runtime_error("hyprshortcuts: initialization failed");
    }

    HyprlandAPI::addNotificationV2(
        handle, {{"text", std::string{"[hyprshortcuts] loaded — bind a key to hyprshortcuts:toggle"}},
                 {"time", uint64_t{5000}},
                 {"color", CHyprColor{0}},
                 {"icon", ICON_OK}});

    return {"hyprshortcuts", "Keybind cheatsheet overlay (like AwesomeWM's helper)", "chicken",
            "0.1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    hs::g_overlay.shutdown();
}
