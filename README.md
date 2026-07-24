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

## Using it

Bind a key to the toggle dispatcher in `hyprland.conf`:

```ini
bind = SUPER, slash, hyprshortcuts:toggle
```

Press it to show/hide. While the overlay is up, **any key (e.g. `Escape`)
dismisses it** (the key is swallowed).

Dispatchers provided:

| Dispatcher                | Action                                   |
| ------------------------- | ---------------------------------------- |
| `hyprshortcuts:toggle`    | Show/hide the cheatsheet                 |
| `hyprshortcuts:close`     | Hide it                                  |
| `hyprshortcuts:refresh`   | Re-read binds and rebuild the layout     |

## Making your binds show up

Only binds **with a description** appear. Descriptions drive the grouping via a
`Category: Label` prefix (split on the first `:`). Use `bindd` (the description
form of `bind`):

```ini
bindd = SUPER, Return,        Apps: Terminal,       exec, kitty
bindd = SUPER, E,             Apps: Files,          exec, nautilus
bindd = SUPER SHIFT, S,       Screenshot: Region,   exec, grimblast copy area
bindd = SUPER, Q,             Window: Close,        killactive,
bindd = SUPER, F,             Window: Fullscreen,   fullscreen,
```

- No `Category:` prefix → the bind lands in a **General** group (shown last).
- Binds without any description are omitted from the sheet.

## Testing

```sh
make test
```

The correctness-critical logic (modmask decoding, key-symbol mapping, category
splitting, grouping, and layout geometry) lives in a Hyprland-free `src/domain/`
layer and is covered by GoogleTest — `make test` needs no running compositor.
See `AGENTS.md` for the architecture and how to extend it.

## Limitations / roadmap (v1)

- **Submap / chord binds** (e.g. hyprchord sequences) are not shown yet.
- The layout is computed **once per show** — it won't re-flow if the monitor
  resolution changes while the overlay is up.
- No pagination/scroll for very large bind sets.
- Colors, font, and column count are hardcoded (a sensible dark theme); config
  via `plugin { hyprshortcuts { ... } }` is planned.
- Rendering targets the monitor under the cursor.

## Credits

Sibling to [hyprchord](../hyprchord). Built in the same plugin idiom.
