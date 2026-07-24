#include <gtest/gtest.h>

#include "commands/Commands.hpp"

using namespace hs;

namespace {

// Records how the commands drive the context, standing in for OverlayController.
struct MockContext : ICommandContext {
    bool visible      = false;
    int  showCalls    = 0;
    int  hideCalls    = 0;
    int  toggleCalls  = 0;
    int  refreshCalls = 0;

    void show() override {
        ++showCalls;
        visible = true;
    }
    void hide() override {
        ++hideCalls;
        visible = false;
    }
    void toggle() override {
        ++toggleCalls;
        visible = !visible;
    }
    void refresh() override { ++refreshCalls; }
    bool isVisible() const override { return visible; }
};

} // namespace

TEST(CommandRegistry, RegistersDefaultCommands) {
    CommandRegistry reg;
    registerDefaultCommands(reg);
    EXPECT_EQ(reg.size(), 3u);
    EXPECT_NE(reg.find("toggle"), nullptr);
    EXPECT_NE(reg.find("close"), nullptr);
    EXPECT_NE(reg.find("refresh"), nullptr);
}

TEST(CommandRegistry, DispatchUnknownReturnsFalse) {
    CommandRegistry reg;
    registerDefaultCommands(reg);
    MockContext ctx;
    EXPECT_FALSE(reg.dispatch("does-not-exist", ctx));
}

TEST(CommandRegistry, ToggleFlipsVisibility) {
    CommandRegistry reg;
    registerDefaultCommands(reg);
    MockContext ctx;

    EXPECT_TRUE(reg.dispatch("toggle", ctx));
    EXPECT_TRUE(ctx.visible);
    EXPECT_TRUE(reg.dispatch("toggle", ctx));
    EXPECT_FALSE(ctx.visible);
    EXPECT_EQ(ctx.toggleCalls, 2);
}

TEST(CommandRegistry, CloseHides) {
    CommandRegistry reg;
    registerDefaultCommands(reg);
    MockContext ctx;
    ctx.visible = true;

    EXPECT_TRUE(reg.dispatch("close", ctx));
    EXPECT_FALSE(ctx.visible);
    EXPECT_EQ(ctx.hideCalls, 1);
}

TEST(CommandRegistry, RefreshInvokesRefresh) {
    CommandRegistry reg;
    registerDefaultCommands(reg);
    MockContext ctx;

    EXPECT_TRUE(reg.dispatch("refresh", ctx));
    EXPECT_EQ(ctx.refreshCalls, 1);
}
