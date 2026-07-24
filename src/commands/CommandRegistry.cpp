#include "CommandRegistry.hpp"

namespace hs {

void CommandRegistry::registerCommand(std::unique_ptr<ICommand> cmd) {
    if (!cmd)
        return;
    m_commands[cmd->name()] = std::move(cmd);
}

bool CommandRegistry::dispatch(const std::string& name, ICommandContext& ctx) {
    ICommand* cmd = find(name);
    if (!cmd)
        return false;
    cmd->execute(ctx);
    return true;
}

ICommand* CommandRegistry::find(const std::string& name) const {
    auto it = m_commands.find(name);
    return it == m_commands.end() ? nullptr : it->second.get();
}

std::vector<std::string> CommandRegistry::names() const {
    std::vector<std::string> out;
    out.reserve(m_commands.size());
    for (const auto& [name, _] : m_commands)
        out.push_back(name);
    return out;
}

} // namespace hs
