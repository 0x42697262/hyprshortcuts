#pragma once

#include <vector>

#include "domain/Types.hpp"

namespace hs {

// The ONLY code that touches Hyprland's SKeybind / g_pKeybindManager. It copies
// the live keybinds into plain RawBind values so the domain layer stays pure and
// unit-testable.
std::vector<RawBind> readLiveBinds();

} // namespace hs
