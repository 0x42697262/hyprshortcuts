#pragma once

#include <memory>

#include "CommandRegistry.hpp"
#include "ICommand.hpp"

// Concrete commands. Each is trivial — it just calls one ICommandContext method
// — so they live together header-only. Add a new action here + register it in
// registerDefaultCommands().

namespace hs {

class ToggleCommand : public ICommand {
  public:
    std::string name() const override { return "toggle"; }
    std::string description() const override { return "Show/hide the keybind cheatsheet"; }
    void        execute(ICommandContext& ctx) override { ctx.toggle(); }
};

class CloseCommand : public ICommand {
  public:
    std::string name() const override { return "close"; }
    std::string description() const override { return "Hide the cheatsheet"; }
    void        execute(ICommandContext& ctx) override { ctx.hide(); }
};

class RefreshCommand : public ICommand {
  public:
    std::string name() const override { return "refresh"; }
    std::string description() const override { return "Re-read binds and rebuild the layout"; }
    void        execute(ICommandContext& ctx) override { ctx.refresh(); }
};

inline void registerDefaultCommands(CommandRegistry& registry) {
    registry.registerCommand(std::make_unique<ToggleCommand>());
    registry.registerCommand(std::make_unique<CloseCommand>());
    registry.registerCommand(std::make_unique<RefreshCommand>());
}

} // namespace hs
