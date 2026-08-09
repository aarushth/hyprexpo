# HyprExpo

HyprExpo is a maintained Hyprland plugin for an expose-style workspace overview: keyboard and pointer selection, drag-and-drop window movement between workspaces (including across monitors), configurable gaps and borders, and Lua-configured gestures.

If you experience any bugs, you are encouraged to [open an issue](https://github.com/sandwichfarm/hyprexpo/issues/new). Information I can use to reproduce a bug is appreciated.

[Announcement Post](https://www.reddit.com/r/hyprland/comments/1o30dsg/hyprexpoplus_outer_gaps_keyboard_navigation_and/)

## History

HyprExpo continues the original expose-style workspace overview plugin from the Hyprland plugins ecosystem. After [the upstream plugin was retired](https://github.com/hyprwm/hyprland-plugins/pull/507#issuecomment-4433386463) from official plugins, this fork signaled continuation and intends to chase Hyprland releases.

Born from [a PR to the old official HyprExpo](https://github.com/hyprwm/hyprland-plugins/pull/507) and formerly known as HyprExpo+ (`hyprexpo-plus`), this has become the home for practical additions that make the overview more usable day to day: multi-monitor awareness, keyboard navigation, drag-and-drop window movement, and configurable gaps and borders.
See the [upstream retirement context](https://github.com/hyprwm/hyprland-plugins/pull/663) and the [original launch announcement of this plugin](https://www.reddit.com/r/hyprland/comments/1o30dsg/hyprexpoplus_outer_gaps_keyboard_navigation_and/) for the project's well established background.

## Related

- https://github.com/colonelpanic8/hyprexpo - Another HyprExpo fork

____

## How It Works

Opening the overview shows a single square grid containing **every active workspace across every monitor** — "active" meaning it has windows, or it's the workspace currently shown on some monitor (so you never lose track of where you are, even on an empty workspace). Special workspaces are never shown. The grid always includes at least one empty, creatable slot unless the workspace you opened the overview from is itself already empty.

The grid auto-sizes to fit: `columns = rows = ceil(sqrt(N))`, with a floor of 2x2.

- **Selecting a tile** on your own monitor switches you to that workspace.
- **Selecting a tile from another monitor** brings that workspace to the monitor you're on, swapping it with whatever was there.
- **Selecting an empty tile** creates a fresh workspace and switches to it.
- **Dragging a window** from one tile to another moves it to that workspace — this works across monitors too.
- **Keyboard navigation** (arrow keys or `next`/`previous`) can land on any tile, including empty ones.

## Install

### hyprpm

```bash
hyprpm add https://github.com/sandwichfarm/hyprexpo
hyprpm enable hyprexpo
hyprpm reload
```

The repository name in `hyprpm.toml` is `hyprexpo`, and the built plugin output is `hyprexpo.so`.

### Build From Source

Install a C++23 compiler, `pkg-config`, Hyprland development headers, and these pkg-config packages:

```text
hyprland pixman-1 libdrm pangocairo libinput libudev wayland-server xkbcommon lua5.4
```

The build prefers the `lua5.4` pkg-config module and falls back to `lua` for
distributions such as Fedora where `lua-devel` exposes the generic module name.

Build with the Makefile:

```bash
git clone https://github.com/sandwichfarm/hyprexpo
cd hyprexpo
make all
```

For day-to-day development, prefer a disposable nested Hyprland session. This
matches Hyprland's plugin development guidance: build the plugin, load it by
absolute path with `hyprctl plugin load`, then unload and load again after
changes.

```bash
./scripts/run-nested.sh
```

`scripts/dev-watch.sh` rebuilds and relaunches that nested session automatically on source changes.

If you already have a disposable Hyprland session running, build to a
user-owned cache path and load or reload that `.so` directly:

```bash
make dev-load
make dev-reload
```

Only replace the hyprpm-managed copy when you intentionally want the installed
plugin to point at this checkout's build:

```bash
make install
hyprpm reload
```

If your distro or install path stores hyprpm artifacts under a root-owned cache,
keep privilege at the command line instead of baking `sudo` into the Makefile:

```bash
sudo make install INSTALL_USER="$USER"
hyprpm reload
```

Use `install` or `make install`, not plain `cp`, when replacing a loaded `.so`.
Hyprland maps plugin files into the running process, and overwriting that file
in place can corrupt the live mapping.

Other build entry points:

```bash
meson setup build
meson compile -C build
```

```bash
cmake -S . -B build
cmake --build build
```

Run the unit tests (pure logic, no Hyprland instance needed):

```bash
make test
```

### Nix

Nix users should build HyprExpo through the Nix Hyprland plugin path instead of mixing a `hyprpm` artifact into a Nix-managed Hyprland session. This repository includes `default.nix`, which uses `hyprlandPlugins.mkHyprlandPlugin` so the plugin follows the Hyprland input supplied by the caller.

Hyprland plugins are ABI-sensitive. Keep the plugin build and running Hyprland revision aligned.

## Quick Config

Add the plugin block to your Hyprland config:

```ini
plugin {
    hyprexpo {
        gaps_in = 5
        gaps_out = 0
        bg_col = rgb(111111)
        gesture_distance = 200
        cancel_key = escape
        show_cursor = 1
        show_pinned_windows = 0
    }
}
```

For `hyprland.lua`, use `hl.config()`:

```lua
hl.config({
    plugin = {
        hyprexpo = {
            gaps_in = 5,
            gaps_out = 0,
            bg_col = "rgb(111111)",
            gesture_distance = 200,
            cancel_key = "escape",
            show_cursor = 1,
        },
    },
})
```

Add a dispatcher binding:

```ini
bind = SUPER, g, hyprexpo:expo, toggle
```

Or in Lua:

```lua
hl.bind("SUPER + G", function()
    hl.plugin.hyprexpo.expo("toggle")
end)
```

## Keyboard Navigation

Keyboard navigation is enabled by default (`keynav_enable = 1`). While the overview is open, HyprExpo activates a submap named `hyprexpo` and resets it when the overview closes. Bind the submap's keys yourself:

```ini
plugin {
    hyprexpo {
        keynav_enable = 1
        keynav_wrap_h = 1
        keynav_wrap_v = 1
        keynav_reading_order = 0
    }
}

submap = hyprexpo
    bind = , left,     hyprexpo:kb_focus, left
    bind = , right,    hyprexpo:kb_focus, right
    bind = , up,       hyprexpo:kb_focus, up
    bind = , down,     hyprexpo:kb_focus, down
    bind = , tab,      hyprexpo:kb_focus, next
    bind = SHIFT, tab, hyprexpo:kb_focus, previous
    bind = , return,   hyprexpo:kb_confirm
    bind = , escape,   hyprexpo:expo, cancel
    bind = , 1,        hyprexpo:kb_selecti, 1
    bind = , 2,        hyprexpo:kb_selecti, 2
    bind = , 3,        hyprexpo:kb_selecti, 3
    bind = , 4,        hyprexpo:kb_selecti, 4
    bind = , 5,        hyprexpo:kb_selecti, 5
    bind = , 6,        hyprexpo:kb_selecti, 6
    bind = , 7,        hyprexpo:kb_selecti, 7
    bind = , 8,        hyprexpo:kb_selecti, 8
    bind = , 9,        hyprexpo:kb_selecti, 9
    bind = , 0,        hyprexpo:kb_selecti, 10
submap = reset
```

For `hyprland.lua`, define the same submap in Lua instead of a `submap = hyprexpo` block in `hyprland.conf` (the two config systems don't share submaps):

```lua
hl.define_submap("hyprexpo", function()
    hl.bind("h",      function() hl.plugin.hyprexpo.kb_focus("left") end)
    hl.bind("l",      function() hl.plugin.hyprexpo.kb_focus("right") end)
    hl.bind("k",      function() hl.plugin.hyprexpo.kb_focus("up") end)
    hl.bind("j",      function() hl.plugin.hyprexpo.kb_focus("down") end)
    hl.bind("tab",       function() hl.plugin.hyprexpo.kb_focus("next") end)
    hl.bind("SHIFT+tab", function() hl.plugin.hyprexpo.kb_focus("previous") end)
    hl.bind("return", function() hl.plugin.hyprexpo.kb_confirm() end)
    hl.bind("escape", function() hl.plugin.hyprexpo.expo("cancel") end)
end)
```

`next`/`previous` always move through the grid in row-major reading order and always wrap (last tile of a row → first tile of the next row; last tile overall → first tile), regardless of `keynav_wrap_h`/`keynav_wrap_v`, which only affect the directional `left`/`right`/`up`/`down` moves. Every tile — including empty ones — is a valid keyboard focus target.

`cancel_key` (default `escape`) closes the overview without switching, independent of the submap. It also works from outside the submap. Use comma-separated key names for multiple cancel keys, or `none`/`off` to disable it:

```ini
plugin {
    hyprexpo {
        cancel_key = escape, q
    }
}
```

Bare `1`-`9` digit keys always select that workspace by ID while the overview is open, even without a submap bind.

## Dispatchers

Main dispatcher:

```ini
bind = SUPER, g, hyprexpo:expo, toggle
```

| argument | description |
| --- | --- |
| `toggle` | show overview if hidden, hide it if shown |
| `cancel` | hide overview without switching workspaces |
| `off`, `close`, `disable` | hide overview |
| `select` | select the hovered workspace |
| `bring` | move the top mapped window from the hovered workspace into the current workspace, without leaving your workspace |
| `1`..`9` | select that workspace by ID |
| anything else | opens the overview if it's closed; no-op if it's already open |

Keyboard dispatchers, active while the overview is open:

| dispatcher | argument | description |
| --- | --- | --- |
| `hyprexpo:kb_focus` | `left`, `right`, `up`, `down`, `next`, `previous` | move keyboard focus across tiles |
| `hyprexpo:kb_confirm` | none | select the focused tile |
| `hyprexpo:kb_selecti` | 1-based visible index | select by visible tile position |
| `hyprexpo:kb_selectn` | workspace ID | select by workspace ID; `0` maps to workspace `10` |
| `hyprexpo:kb_select` | token | select by token (`1`-`9`, `0`, then `a`-`z` by visible tile order) |
| `hyprexpo:move_window` | `source target [address]` | move a window between 1-based visible tile indices; defaults to the top mapped window on the source tile if no address is given |

Hyprland may briefly report invalid dispatcher messages during startup if binds are parsed before plugins are loaded. Those messages are cosmetic; the dispatchers work once the plugin is loaded.

## Configuration Options

Use the short key names inside a `plugin { hyprexpo { ... } }` block, or the fully qualified `plugin:hyprexpo:*` names elsewhere.

### Behavior

| key | type | description | default |
| --- | --- | --- | --- |
| `gaps_in` | int | spacing between tiles in pixels | `5` |
| `gaps_out` | int | outer margin around the grid in pixels | `0` |
| `bg_col` | color | grid background color | `0xFF111111` |
| `gesture_distance` | int | swipe distance considered complete | `200` |
| `cancel_key` | string | comma-separated key names that close overview without selecting; `none` or `off` disables | `escape` |
| `show_cursor` | bool int | keep the cursor visible while overview is open | `1` |
| `show_pinned_windows` | bool int | render pinned/PiP windows in workspace preview thumbnails; default `0` hides them from previews only | `0` |

Pinned windows, including browser Picture-in-Picture windows, stay pinned and visible in normal Hyprland regardless of this setting. It only controls whether they additionally appear in every overview tile's thumbnail.

### Keyboard Navigation

| key | type | description | default |
| --- | --- | --- | --- |
| `keynav_enable` | bool int | enable keyboard navigation and the overview submap | `1` |
| `keynav_wrap_h` | bool int | wrap horizontally at row edges (`left`/`right` only) | `1` |
| `keynav_wrap_v` | bool int | wrap vertically at column edges (`up`/`down` only) | `1` |
| `keynav_reading_order` | bool int | use row-major horizontal movement instead of spatial movement for `left`/`right` | `0` |

### Tile Appearance

| key | type | description | default |
| --- | --- | --- | --- |
| `tile_rounding` | int | corner radius in pixels for workspace previews | `0` |
| `tile_rounding_power` | float | rounding curve exponent | `2.0` |
| `tile_rounding_focus` | int | focused tile radius; `-1` inherits `tile_rounding` | `-1` |
| `tile_rounding_current` | int | current tile radius; `-1` inherits `tile_rounding` | `-1` |
| `tile_rounding_hover` | int | hovered tile radius; `-1` inherits `tile_rounding` | `-1` |
| `border_width` | int | border thickness in pixels | `2` |
| `border_color` | string | default border for non-highlighted tiles; solid color or gradient | empty |
| `border_color_current` | string | current workspace tile border; solid color or gradient | `rgb(66ccff)` |
| `border_color_focus` | string | keyboard-focused tile border; solid color or gradient | `rgb(ffcc66)` |
| `border_color_hover` | string | pointer-hovered tile border; solid color or gradient | `rgb(aabbcc)` |
| `border_grad_current` | string | deprecated fallback; use `border_color_current` | empty |
| `border_grad_focus` | string | deprecated fallback; use `border_color_focus` | empty |
| `border_grad_hover` | string | deprecated fallback; use `border_color_hover` | empty |
| `border_style` | string | deprecated compatibility key; border style is auto-detected from the color format | `simple` |

Solid values:

```ini
bg_col = rgb(111111)
border_color_current = rgb(66ccff)
```

Gradient values:

```ini
border_color_current = rgba(33ccffee) rgba(00ff99ee) 45deg
```

### Drag-Drop Window Styling

These options style the visual feedback shown while dragging a window between workspace previews: the under-pointer drag proxy, and, while the pointer is over a valid target tile, a positional landing proxy inside that tile. Empty border values inherit the focused tile border, so the default look is unchanged until you opt in.

| key | type | description | default |
| --- | --- | --- | --- |
| `drag_drop_proxy_color` | color | translucent dragged-window proxy before the move threshold is crossed | `0x24EDB342` |
| `drag_drop_proxy_active_color` | color | dragged-window proxy after movement is active | `0x3DEDB342` |
| `drag_drop_proxy_border_color` | string | proxy border; solid color or gradient; empty inherits `border_color_focus` | empty |
| `drag_drop_proxy_border_width` | int | proxy border width; `-1` inherits `max(2, border_width + 1)`, `0` disables | `-1` |
| `drag_drop_proxy_rounding` | int | proxy corner radius in pixels; `-1` inherits the automatic focused-tile rounding | `-1` |
| `drag_drop_source_border_color` | string | source workspace border while a drag/drop move is active; empty inherits focus border | empty |
| `drag_drop_source_border_width` | int | source workspace border width while dragging; `-1` inherits `border_width`, `0` disables | `-1` |

```ini
plugin {
    hyprexpo {
        drag_drop_proxy_color = rgba(66ccff22)
        drag_drop_proxy_active_color = rgba(66ccff44)
        drag_drop_proxy_border_color = rgba(66ccffee) rgba(ffcc66ee) 45deg
        drag_drop_source_border_color = rgb(ffcc66)
        drag_drop_proxy_border_width = 3
        drag_drop_proxy_rounding = 10
    }
}
```

### Safe Failure Behavior

Invalid border colors, gradient values, and bool-int options are expected to fail safely: the plugin logs the invalid value and falls back to a default instead of crashing Hyprland during render.

## Lua API

HyprExpo exposes helpers under `hl.plugin.hyprexpo` when Hyprland is using Lua config support:

```lua
hl.plugin.hyprexpo.expo("toggle")
hl.plugin.hyprexpo.expo("cancel")
hl.plugin.hyprexpo.kb_focus("left")
hl.plugin.hyprexpo.kb_focus("next")
hl.plugin.hyprexpo.kb_confirm()
hl.plugin.hyprexpo.kb_selecti(1)
hl.plugin.hyprexpo.kb_selectn(1)
hl.plugin.hyprexpo.kb_select("1")
```

`hl.plugin.hyprexpo` is the Lua helper namespace for dispatchers and gestures; it is not the configuration block. Configure options with `hl.config()` and a nested `plugin.hyprexpo` table, as shown in [Quick Config](#quick-config).

Arguments are validated strictly — fractional numbers, booleans, tables where strings are expected, and partial numeric strings are rejected instead of silently coerced:

| function | accepted arguments |
| --- | --- |
| `expo` | string, defaulting to `"toggle"` when omitted |
| `kb_focus` | string |
| `kb_confirm` | none |
| `kb_selecti` | Lua integer or exact integer string |
| `kb_selectn` | Lua integer or exact integer string |
| `kb_select` | string |

### Gestures

```lua
hl.plugin.hyprexpo.gesture({
    fingers = 4,
    direction = "up",
    action = "expo",
})
```

| field | type | required | description |
| --- | --- | --- | --- |
| `fingers` | integer | yes | number of fingers |
| `direction` | string | yes | swipe direction |
| `action` | string | no | `expo` or `unset`; defaults to `expo` |
| `mods` | string | no | modifier expression passed to Hyprland |
| `scale` | number | no | gesture scale; defaults to `1.0` |
| `disable_inhibit` | boolean | no | whether to bypass inhibit handling |

## Compatibility

Hyprland checks plugin API compatibility when loading a plugin. If HyprExpo was built against an incompatible Hyprland revision, loading fails with a visible API/hash mismatch instead of silently running against the wrong ABI — keep the plugin build and the running Hyprland revision aligned.

Release builds attach `release-provenance.txt` next to `hyprexpo.so`, recording `hyprctl version`, `pkg-config --modversion hyprland`, the Lua pkg-config version, the compiler version, and `ldd -r hyprexpo.so`.

## Troubleshooting

**Plugin load fails with an API or hash mismatch** — rebuild HyprExpo against the same Hyprland revision that is running, then reload the plugin.

**Plugin load fails because dependencies are missing** — install the build dependencies listed above and rebuild. Current linked runtime dependencies include Lua (`lua5.4` or `lua` through pkg-config), `pangocairo`, and `xkbcommon`.

**Invalid config values** — invalid border colors or bool-int options are logged and fall back safely; check the format against the [Configuration Options](#configuration-options) tables above.

**Replacing a loaded plugin crashes Hyprland** — for development, prefer `./scripts/run-nested.sh` or `make dev-reload` so Hyprland loads a fresh user-owned build by absolute path. If you intentionally replace an installed plugin file, use `make install` or `install`, not `cp` — Hyprland maps the plugin into the running process, and overwriting the file in place can corrupt the live mapping.

## Development

Unit tests cover pure logic (grid sizing, tile hit-testing, color/gradient parsing, drop-intent geometry) and don't require a running Hyprland instance:

```bash
make test
```

For interactive testing, `./scripts/run-nested.sh` launches a disposable nested Hyprland session with a fresh user-owned build. Useful binds in that session:

- `F10` for overview
- `SUPER+Return` for a terminal
- `SUPER+1..9` for workspaces
- `SUPER+SHIFT+1..9` to move a window
- `SUPER+Q` to close a window
- `SUPER+SHIFT+Q` to exit

`./scripts/dev-watch.sh` rebuilds and relaunches that session automatically on source changes.

Before publishing a release, exercise at minimum: toggling the overview, cancelling with `cancel_key`, keyboard focus + confirm (including `next`/`previous` and landing on an empty tile), pointer/touch selection, drag-and-drop between two tiles on the same monitor and across two different monitors, the pinned-window preview toggle, and a Lua gesture registration.

## Releasing

See [RELEASING.md](RELEASING.md) for how versions are cut and published.
