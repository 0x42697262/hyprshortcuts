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

## Configuration

All appearance is configurable under a `plugin { hyprshortcuts { ... } }` block.
Values are re-read on `hyprctl reload`, so you can tweak the theme live. Colors
are `AARRGGBB` hex.

```ini
plugin {
    hyprshortcuts {
        key_style   = icons     # keycap contents: "icons" (⌘ ⏎) or "text" (Super Enter)
        font_family = Sans      # font for all overlay text
        max_columns = 4         # max columns of category cards (1–8)

        # colors (AARRGGBB)
        scrim_color     = 0x73000000   # full-screen dim behind the panel
        panel_color     = 0xF01C1F26   # panel background
        card_color      = 0xF52A2C36   # category card background
        keycap_color    = 0xFF3D404D   # keycap background
        title_color     = 0xFF8CC7FF   # category title text
        action_color    = 0xFFD1D6E0   # action/description text
        key_color       = 0xFFF5F5FA   # keycap glyph/label text
        separator_color = 0xFF808594   # the "+" joiner and chord "›"

        # corner radii (px)
        panel_rounding  = 18
        card_rounding   = 12
        keycap_rounding = 6
    }
}
```

The defaults above reproduce the built-in dark theme, so an empty block changes
nothing. Set `key_style = text` to render keycaps as word labels (`Super Enter`)
instead of glyphs — keys without a distinct glyph (e.g. `F5`) look the same
either way.

## Testing

```sh
make test
```

The correctness-critical logic (modmask decoding, key-symbol mapping, category
splitting, grouping, and layout geometry) lives in a Hyprland-free `src/domain/`
layer and is covered by GoogleTest — `make test` needs no running compositor.
See `AGENTS.md` for the architecture and how to extend it.

## Limitations / roadmap

- No pagination/scroll for very large bind sets — a very tall sheet can overflow
  the screen.
- Rendering targets the monitor under the cursor.

Colors, font, column count, and roundings are configurable (see
[Configuration](#configuration)), and the layout re-flows if the monitor
resolution changes while the overlay is up.

## Credits

Sibling to [hyprchord](../hyprchord). Built in the same plugin idiom.
