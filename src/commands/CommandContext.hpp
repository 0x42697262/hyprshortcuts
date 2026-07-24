#pragma once

namespace hs {

// The surface commands act on. OverlayController implements it for real; tests
// implement a mock. Commands NEVER touch Hyprland directly — only this
// interface — which keeps the whole command layer unit-testable.
struct ICommandContext {
    virtual ~ICommandContext() = default;

    virtual void show()             = 0;
    virtual void hide()             = 0;
    virtual void toggle()           = 0;
    virtual void refresh()          = 0; // re-read binds and rebuild the layout
    virtual bool isVisible() const  = 0;
};

} // namespace hs
