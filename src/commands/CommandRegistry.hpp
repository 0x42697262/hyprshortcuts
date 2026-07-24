#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ICommand.hpp"

namespace hs {

// Maps command names to commands and dispatches by name. No Hyprland deps.
class CommandRegistry {
  public:
    void registerCommand(std::unique_ptr<ICommand> cmd);

    // Execute the named command against ctx. Returns false if no such command.
    bool dispatch(const std::string& name, ICommandContext& ctx);

    ICommand*                find(const std::string& name) const;
    std::vector<std::string> names() const;
    size_t                   size() const { return m_commands.size(); }

  private:
    std::unordered_map<std::string, std::unique_ptr<ICommand>> m_commands;
};

} // namespace hs
