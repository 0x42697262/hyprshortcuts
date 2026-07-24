#include "BindSource.hpp"

#include <src/managers/KeybindManager.hpp>

namespace hs {

std::vector<RawBind> readLiveBinds() {
    std::vector<RawBind> out;
    if (!g_pKeybindManager)
        return out;

    for (const auto& kb : g_pKeybindManager->m_keybinds) {
        if (!kb)
            continue;

        RawBind b;
        b.modmask        = kb->modmask;
        b.key            = kb->key;
        b.displayKey     = kb->displayKey;
        b.description    = kb->description;
        b.handler        = kb->handler;
        b.arg            = kb->arg;
        b.submap         = kb->submap.name;
        b.hasDescription = kb->hasDescription;
        b.mouse          = kb->mouse;
        b.enabled        = kb->enabled;
        out.push_back(std::move(b));
    }

    return out;
}

} // namespace hs
