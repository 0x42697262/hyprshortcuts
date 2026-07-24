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

## Categories & chords

A bind is shown only if the user described it. The category comes from a
`"Category: Label"` prefix, split on the first `:` (see `BindModel::splitCategory`).
No prefix → the default `"General"` group (sorted last). Descriptions come from
`opts.description` in the Lua config (Hyprland 0.55+ is Lua-native; `bindd`/`.conf`
is deprecated).

**hyprchord chains** (binds whose submap starts with `hc:`) are expanded into a
multi-step `Shortcut`: the leading steps are reconstructed from the submap name
(`BindModel::chordPrefixSteps`) and the final step is the bind's own key. Layout
draws steps joined by a `›` separator. hyprchord's own auto-labels (any
description starting with `hyprchords:`) are treated as "undescribed" and hidden,
as are `abort`/`chain`/catchall machinery binds. hyprchord was extended so
`hc.chord(steps, action, description)` sets a real description — see its repo.
`Grouping::dedupeShortcuts` collapses the sticky-mods duplicate a chain produces.

## Conventions

- C++26, 4-space indent, `hs::` namespace, `m_` member prefix, `.hpp/.cpp` pairs.
- Keep the domain layer header-free of Hyprland; inject collaborators (e.g. the
  `TextMeasure` callback into `Layout`) rather than reaching for globals.
- `make test` must stay green and must not require a running Hyprland.
- Match the surrounding style of the sibling `hyprchord` plugin.

## Known limits / future work

See the bottom of `README.md`. The theme, font, column count, roundings, and
keycap style (`icons`/`text`) are configurable from Lua via
`hl.config({ plugin = { hyprshortcuts = { ... } } })` — registered as
`plugin:hyprshortcuts:*` config values in
`OverlayController::registerConfig` and read in `readConfig` (called from init
and the `config.reloaded` listener). Config values feed a base `LayoutMetrics`
(`m_metrics`), the render `Theme` (`OverlayRenderer::setTheme`), and the font
(`TextRenderer::setFont`). The layout re-flows when the monitor resolution
changes (a size-diff check in `onRenderStage`). The remaining v1 gap is
pagination/scroll for oversized sheets.
