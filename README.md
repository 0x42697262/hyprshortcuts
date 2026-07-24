# hyprshortcuts

An on-screen **keybind cheatsheet** for Hyprland — like AwesomeWM's hotkeys
helper. Press a key and an overlay fades in listing your keybinds as styled
keycap boxes (`⌘ + ⏎  Terminal`), grouped by category. Press any key (or the
toggle again) to dismiss.

It's a **Hyprland plugin**: it reads your binds directly from the running
compositor and draws the overlay itself — no separate app, no `hyprctl` polling.

```
┌─ Apps ─────────────┐   ┌─ Window ───────────┐
│ ⌘ ⏎    Terminal    │   │ ⌘ Q    Close       │
│ ⌘ E    Files       │   │ ⌘ F    Fullscreen  │
│ ⌘ ⇧ S  Screenshot  │   │ ⌘ ⌃ ←  Resize left │
└────────────────────┘   └────────────────────┘
```

## Building

Requires Hyprland headers matching the **exact running commit** (plugins are
ABI-coupled; use the same compiler Hyprland was built with — C++26).

```sh
# against installed headers (hyprland.pc present):
make

# against a source checkout:
make HYPRLAND_HEADERS=~/Github/Hyprland
```

Dependencies: `pangocairo` (text rendering) and, for tests, `gtest`.

## Loading

```sh
hyprctl plugin load $(pwd)/hyprshortcuts.so
hyprctl plugins list        # confirm it's loaded
```

Or with hyprpm, once pushed to a git remote:

```sh
hyprpm add <repo-url>
hyprpm enable hyprshortcuts
hyprpm reload
```

## Using it (Lua config)

Hyprland 0.55+ uses Lua config (`hyprland.lua`, `hyprlang`/`.conf` deprecated).
The plugin exposes Lua functions under `hl.plugin.hyprshortcuts`, so binding the
toggle is a one-liner (guard on the plugin being loaded):

```lua
-- config/keybinds.lua
if hl.plugin.hyprshortcuts then
    hl.bind("SUPER + SLASH", hl.plugin.hyprshortcuts.toggle)
end
```

Or, if you drive keys through hyprchords, register it as a chord:

```lua
hc.chord("SUPER + SLASH", hl.plugin.hyprshortcuts.toggle, "System: Keybind cheatsheet")
```

Press it to show/hide. While the overlay is up, **any key (e.g. `Escape`)
dismisses it** (the key is swallowed).

Lua functions: `hl.plugin.hyprshortcuts.toggle` / `.close` / `.refresh`
(re-read binds). The same actions are also plain dispatchers
(`hyprshortcuts:toggle`, `:close`, `:refresh`) if you prefer.

## Making your binds show up

Only binds **you describe** appear, so the sheet stays meaningful. The
description drives the grouping via a `Category: Label` prefix (split on the
first `:`). In Lua, a description is `opts.description`:

```lua
-- plain bind
hl.bind("SUPER + E", hl.dsp.exec_cmd("nautilus"), { description = "Apps: Files" })

-- in a keymap-table entry (bind and/or hyprchords chord)
{
    action = hl.dsp.exec_cmd(d.terminal),
    bind   = "SUPER + RETURN",
    chord  = "SUPER+RETURN",
    opts   = { description = "Apps: Terminal" },
},
```

- No `Category:` prefix → the bind lands in a **General** group (shown last).
- Binds without a description are omitted from the sheet.

### hyprchords chords

hyprchord chains show up too, rendered as their full step sequence — e.g.
`SUPER+X ; F` draws as `⌘ X › F`. This needs a hyprchords build with description
support: `hc.chord(steps, action, "Category: Label")`. Pass the description as
the third argument (this repo's sibling `hyprchord` supports it); if you register
chords from a keymap table, forward `opts.description` in your `register_one`:

```lua
hc.chord(chord, action, opts and opts.description)
```

Chords without a description are hidden (hyprchord's auto label like
`super+x ; f -> lua` is treated as "no description").

## Testing

```sh
make test
```

The correctness-critical logic (modmask decoding, key-symbol mapping, category
splitting, grouping, and layout geometry) lives in a Hyprland-free `src/domain/`
layer and is covered by GoogleTest — `make test` needs no running compositor.
See `AGENTS.md` for the architecture and how to extend it.

## Limitations / roadmap (v1)

- The layout is computed **once per show** — it won't re-flow if the monitor
  resolution changes while the overlay is up.
- No pagination/scroll for very large bind sets.
- Colors, font, and column count are hardcoded (a sensible dark theme); config
  via `plugin { hyprshortcuts { ... } }` is planned.
- Rendering targets the monitor under the cursor.

## Credits

Sibling to [hyprchord](../hyprchord). Built in the same plugin idiom.
