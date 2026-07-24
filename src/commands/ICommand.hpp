#pragma once

#include <string>

#include "CommandContext.hpp"

namespace hs {

// One user action. Adding a new action = add an ICommand and register() it; the
// dispatchers and key handling never need editing (the Command pattern payoff).
struct ICommand {
    virtual ~ICommand() = default;

    virtual std::string name() const        = 0; // dispatch key, e.g. "toggle"
    virtual std::string description() const = 0;
    virtual void        execute(ICommandContext& ctx) = 0;
};

} // namespace hs
