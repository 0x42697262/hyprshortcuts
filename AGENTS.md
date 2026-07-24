# AGENTS.md — hyprshortcuts

Guidance for AI agents and humans working in this repo. Read this before making
changes.

## What this is

A **Hyprland plugin** (`.so`) that draws an on-screen cheatsheet of the current
Hyprland keybinds — like AwesomeWM's hotkeys helper — with each shortcut rendered
as styled keycap boxes, grouped by category. It reads binds **in-process** from
Hyprland's keybind manager and draws the overlay itself via Hyprland's render
pipeline. There is no external process, GUI toolkit, or `hyprctl` call.

## Build / run / test

```sh
make                 # build hyprshortcuts.so against installed Hyprland headers
make HYPRLAND_HEADERS=~/Github/Hyprland   # ...or against a source checkout
make test            # build & run the unit tests (GoogleTest) — no Hyprland needed
make clean

# load into the running compositor (triggers a config reload automatically):
hyprctl plugin load $(pwd)/hyprshortcuts.so
hyprctl plugin unload $(pwd)/hyprshortcuts.so
```

Plugins are ABI-coupled to the **exact** running Hyprland commit; build with the
matching headers and the same compiler Hyprland was built with (C++26 here).

## Architecture — strict layering (dependencies point downward)

```
src/main.cpp            plugin lifecycle → hs::g_overlay
src/OverlayController    singleton; owns render/key/reload subscriptions,
                         command dispatchers, visibility + fade state.
                         Implements ICommandContext.
   │
   ├── commands/         Command pattern (interaction)
   │     ICommand, ICommandContext, CommandRegistry, Commands.hpp
   ├── render/           Hyprland-coupled drawing
   │     TextRenderer (pango/cairo → CGLTexture), OverlayRenderer (draw tree)
   ├── BindSource        the ONLY file that touches SKeybind/g_pKeybindManager;
   │                     maps live binds → domain RawBind
   └── domain/           PURE — no Hyprland headers, fully unit-tested
         Types, KeySymbols, BindModel, Grouping, Layout
```

**The golden rule: `src/domain/` and `src/commands/` never include a Hyprland
header.** They operate on plain data (`RawBind`, `Shortcut`, `Category`,
`LayoutTree`) and are compiled into the test binary on a normal host build. All
correctness-critical logic lives there so it can be tested without a compositor.
Hyprland-specific code is confined to `BindSource`, `render/`, and
`OverlayController`.

Data flow: `BindSource` → `BindModel::toShortcut` → `Grouping::groupByCategory`
→ `Layout::computeLayout` (a pure render tree) → `OverlayRenderer` draws it with
`TextRenderer` for text.

## Commands (the "commander" pattern)

Every user action is an `ICommand` executed against the `ICommandContext`
interface (implemented by `OverlayController`, mocked in tests). Commands never
touch Hyprland directly.

**To add an action:**
1. Add a class in `src/commands/Commands.hpp` implementing `ICommand`
   (`name()`, `description()`, `execute(ICommandContext&)`).
2. Register it in `registerDefaultCommands()`.
3. `OverlayController::init` auto-exposes it as the dispatcher
   `hyprshortcuts:<name>` — no dispatcher/keyhandler edits needed.
4. Add a case to `tests/test_commands.cpp`.

If the action needs a new capability on the context, add a method to
`ICommandContext` and implement it on `OverlayController` + the test mock.

## Categories

A bind is shown only if it has a description. The category comes from a
`"Category: Label"` prefix, split on the first `:` (see `BindModel::splitCategory`).
No prefix → the default `"General"` group (sorted last). Example:

```
bindd = SUPER, Return, Apps: Terminal, exec, kitty
```

## Conventions

- C++26, 4-space indent, `hs::` namespace, `m_` member prefix, `.hpp/.cpp` pairs.
- Keep the domain layer header-free of Hyprland; inject collaborators (e.g. the
  `TextMeasure` callback into `Layout`) rather than reaching for globals.
- `make test` must stay green and must not require a running Hyprland.
- Match the surrounding style of the sibling `hyprchord` plugin.

## Known limits / future work

See the bottom of `README.md`. Notably v1 ignores submap/chord binds, lays out
once per show (no live re-layout on resolution change), and hardcodes the theme.
