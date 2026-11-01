# hyprexpo

A workspace overview plugin for Hyprland. It shows one grid containing the workspaces from **all** your monitors, so you can jump between them, pull a workspace onto the current monitor, or drag windows from one workspace to another, with the keyboard, mouse, touchscreen or touchpad.

Tracks **Hyprland 0.57.0**. Hyprland 0.57 is configured in Lua, so all configuration here is Lua too.

## How it works

The overview opens on the monitor under the cursor. Other monitors keep showing their normal contents.

**What's in the grid**

- Every workspace that has windows, plus the workspace currently shown on each monitor, even if it's empty.
- Named workspaces come first, then numbered workspaces in ascending order.
- Special workspaces are never shown.
- Any leftover tiles at the end are empty. There is always at least one, unless the workspace you opened from is already empty and can serve that purpose.

The grid is always square: its side is `ceil(sqrt(N))` tiles, with a minimum of 2x2.

**What selecting a tile does**

| tile | result |
| --- | --- |
| Workspace on this monitor | Switches to it |
| Workspace shown on another monitor | Swaps it with the workspace on this monitor |
| Workspace on another monitor, not currently shown there | Moves it to this monitor and switches to it |
| Empty tile | Switches to the next empty numbered workspace |

**Mouse and touch**

- Click a tile with any mouse button to select it.
- Drag a window onto another tile to move it there, including onto empty tiles and workspaces from other monitors. A drag starts after 12px of movement, and the overview stays open afterwards so you can keep moving windows.
- Tap a tile on a touchscreen to select it. Dragging windows by touch isn't supported.

**Keyboard**

Keyboard focus starts on the current tile and can land on any tile, including empty ones. Confirming selects the focused tile. See [Keyboard navigation](#keyboard-navigation).

**Touchpad**

Register a [gesture](#gestures) and swipe to open the overview progressively. Releasing past the halfway point commits; releasing before it cancels. Swiping again while it's open closes it on the hovered tile.

## Install

### hyprpm

```bash
hyprpm add https://github.com/aarushth/hyprexpo
hyprpm enable hyprexpo
```

Then have Hyprland load enabled plugins at startup from your `hyprland.lua`:

```lua
hl.on("hyprland.start", function()
	hl.exec_cmd("hyprpm reload")
end)
```

### Build from source

You need a C++23 compiler, `pkg-config`, Hyprland's development headers, and these pkg-config packages:

```text
hyprland pixman-1 libdrm pangocairo libinput libudev wayland-server xkbcommon lua5.4
```

The build uses `lua5.4` if available and otherwise falls back to `lua`, which is the module name on distributions such as Fedora.

```bash
git clone https://github.com/aarushth/hyprexpo
cd hyprexpo
make all
```

Load the result from your `hyprland.lua`, using an absolute path:

```lua
hl.plugin.load("/absolute/path/to/hyprexpo.so")
```

Meson and CMake builds are also available:

```bash
meson setup build && meson compile -C build
cmake -S . -B build && cmake --build build
```


## Quick start

This is a complete setup: **Alt+Tab** opens the overview, further Tabs cycle through tiles, and **releasing Alt** switches to the focused tile, like a normal window switcher.

```lua
hl.config({
	plugin = {
		hyprexpo = {
			gaps_in = 3,
			gaps_out = 3,
			bg_col = "rgb(111111)",
			border_width = 3,
			-- Transparent, so the only visible border is the keyboard focus
			border_color_current = "rgba(11111100)",
			border_color_hover = "rgba(11111100)",
			border_color_focus = "rgba(4B09F5ee) rgba(02C939ee) 45deg",
		},
	},
})

-- Open the overview
hl.bind("ALT + TAB", function()
	hl.plugin.hyprexpo.expo("toggle")
end, { non_consuming = true })

hl.bind("ALT + SHIFT + TAB", function()
	hl.plugin.hyprexpo.expo("toggle")
end, { non_consuming = true })

-- Active only while the overview is open
hl.define_submap("hyprexpo", function()
	hl.bind("ALT + TAB", function()
		hl.plugin.hyprexpo.kb_focus("next")
	end, { non_consuming = true })
	hl.bind("ALT + SHIFT + TAB", function()
		hl.plugin.hyprexpo.kb_focus("previous")
	end, { non_consuming = true })

	hl.bind("ALT + h", function()
		hl.plugin.hyprexpo.kb_focus("left")
	end)
	hl.bind("ALT + l", function()
		hl.plugin.hyprexpo.kb_focus("right")
	end)
	hl.bind("ALT + k", function()
		hl.plugin.hyprexpo.kb_focus("up")
	end)
	hl.bind("ALT + j", function()
		hl.plugin.hyprexpo.kb_focus("down")
	end)

	hl.bind("return", function()
		hl.plugin.hyprexpo.kb_confirm()
	end)

	-- Releasing Alt confirms the focused tile
	hl.bind("ALT_L", function()
		hl.plugin.hyprexpo.kb_confirm()
	end, { release = true, submap_universal = true, ignore_mods = true, transparent = true })
end)
```

Prefer a simple toggle? Bind it like any other action, and click a tile or use `return` to choose:

```lua
hl.bind("SUPER + G", function()
	hl.plugin.hyprexpo.expo("toggle")
end)
```

From a shell or script:

```bash
hyprctl eval 'hl.plugin.hyprexpo.expo("toggle")'
```

## Keyboard navigation

While the overview is open, hyprexpo activates a submap named `hyprexpo`, which you define yourself with `hl.define_submap`, and leaves it on close. That's why the binds in [Quick start](#quick-start) only apply inside the overview. Closing always returns to the default submap, even if you opened the overview from a different one. Set `keynav_enable = 0` to stop hyprexpo entering the submap.

**Moving focus**

- `left`, `right`, `up` and `down` move across the grid and respect `keynav_wrap_h` and `keynav_wrap_v`.
- `next` and `previous` step through tiles in reading order and always wrap, ignoring those options.

**Keys handled before your binds**

These work without any binding, and they reach hyprexpo first, so any bind you put on them in the submap won't fire.

- **Cancel keys:** `cancel_key` (default `escape`) closes the overview without switching.
- **Digits `1`-`9`**, including the numpad and with any modifiers held: select the workspace with that number. If it isn't in the grid, the overview closes and Hyprland switches to that workspace anyway.

The `0` key isn't handled this way, so you can bind it, for example to `kb_selectn(0)`, which selects workspace 10.

Note that closing with `expo("toggle")` stays on the current workspace even if you've moved keyboard focus. Use `kb_confirm` to go to the focused tile.

## Lua API

Everything is called through `hl.plugin.hyprexpo`, which holds functions only. Options go in `hl.config()`, as in [Quick start](#quick-start).

A failed call raises a Lua error. Arguments are checked strictly: fractional numbers, booleans and partial numeric strings are rejected, not coerced.

### `expo(arg)`

`arg` is case-insensitive and defaults to `"toggle"`.

| argument | when closed | when open |
| --- | --- | --- |
| `"toggle"` | open | close, staying on the current workspace |
| `"cancel"` | nothing | close, staying on the current workspace |
| `"off"`, `"close"`, `"disable"` | nothing | close, staying on the current workspace |
| `"select"` | nothing | select the hovered tile |
| `"bring"` | nothing | move the top window of the hovered tile to your current workspace, then close without switching |
| `"1"` to `"9"` | switch to that workspace | select that workspace, or close and switch to it if it isn't in the grid |
| anything else | open | nothing |

### Selection and focus

These do nothing while the overview is closed.

| function | argument | description |
| --- | --- | --- |
| `kb_focus(dir)` | `"left"`, `"right"`, `"up"`, `"down"`, `"next"`, `"previous"` | Move keyboard focus |
| `kb_confirm()` | none | Select the focused tile |
| `kb_selectn(n)` | integer or integer string | Select workspace number `n`; `0` means 10. Does nothing if it isn't in the grid |
| `kb_selecti(i)` | integer or integer string, 1 or higher | Select the `i`th workspace tile, skipping empty tiles |
| `kb_select(token)` | string | Like `kb_selecti`, using a single-character token: `1`-`9`, then `0`, then `a`-`z` for tiles 11 onward |

### Gestures

```lua
if hl.plugin.hyprexpo then
	hl.plugin.hyprexpo.gesture({ fingers = 4, direction = "up" })
end
```

Put this at the top level of your config, not in a `hyprland.start` handler. The plugin usually isn't loaded when your config first runs, so the check skips the call that time. hyprexpo then reloads your config when it loads, and the gesture registers.

| field | type | required | description |
| --- | --- | --- | --- |
| `fingers` | integer | yes | 2 to 9 |
| `direction` | string | yes | A Hyprland gesture direction such as `up`, `down`, `left`, `right`, `vertical`, `horizontal` or `swipe` |
| `action` | string | no | `expo` (default), or `unset` to remove a matching gesture |
| `mods` | string | no | Modifier keys that must be held |
| `scale` | number | no | Gesture speed multiplier, clamped to 0.1-10; default `1.0` |
| `disable_inhibit` | boolean | no | Passed to Hyprland's gesture handling; default `false` |

## Configuration

All options go under `plugin.hyprexpo` in `hl.config()`. Options marked "0 or 1" are off at `0` and on at `1`.

### General

| option | type | default | description |
| --- | --- | --- | --- |
| `gaps_in` | int | `5` | Space between tiles, in pixels. Read when the overview opens |
| `gaps_out` | int | `0` | Margin around the grid, in pixels |
| `bg_col` | color | `0xFF111111` | Background behind the tiles; alpha is ignored. Read when the overview opens |
| `gesture_distance` | int | `200` | Swipe distance for a complete open or close |
| `cancel_key` | string | `escape` | Comma-separated key names that close without switching, such as `"escape, q"`. Use `none`, `off`, `disable` or an empty value to turn this off |
| `show_cursor` | 0 or 1 | `1` | Force the cursor visible while the overview is open. At `0` hyprexpo leaves the cursor alone |
| `show_pinned_windows` | 0 or 1 | `0` | Include pinned windows, such as browser Picture-in-Picture, in tile previews. This only affects previews; pinned windows behave normally outside the overview |

### Keyboard

| option | type | default | description |
| --- | --- | --- | --- |
| `keynav_enable` | 0 or 1 | `1` | Enter the `hyprexpo` submap while the overview is open |
| `keynav_wrap_h` | 0 or 1 | `1` | `left` and `right` wrap at row edges |
| `keynav_wrap_v` | 0 or 1 | `1` | `up` and `down` wrap at column edges |
| `keynav_reading_order` | 0 or 1 | `0` | `left` and `right` continue onto the adjacent row instead of staying in the same one. They wrap at the ends of the grid only if both wrap options are on |

### Tile appearance

| option | type | default | description |
| --- | --- | --- | --- |
| `tile_rounding` | int | `0` | Corner radius in pixels |
| `tile_rounding_power` | float | `2.0` | Rounding curve exponent |
| `tile_rounding_focus` | int | `-1` | Radius of the keyboard-focused tile; `-1` uses `tile_rounding` |
| `tile_rounding_current` | int | `-1` | Radius of the current workspace's tile; `-1` uses `tile_rounding` |
| `tile_rounding_hover` | int | `-1` | Radius of the hovered tile; `-1` uses `tile_rounding` |
| `border_width` | int | `2` | Border thickness in pixels; the minimum is 1 |
| `border_color_current` | string | `rgb(66ccff)` | Border of the current workspace's tile |
| `border_color_focus` | string | `rgb(ffcc66)` | Border of the keyboard-focused tile |
| `border_color_hover` | string | `rgb(aabbcc)` | Border of the hovered tile |

Only highlighted tiles get a border. When one tile is in several states, the priority is focus, then current, then hover. Focus starts on the current tile, so `border_color_current` doesn't show until focus moves away.

Border colors take a solid color or a two-color gradient:

```lua
border_color_current = "rgb(66ccff)",
border_color_focus = "rgba(33ccffee) rgba(00ff99ee) 45deg",
```

To hide a border, use a fully transparent color, as in [Quick start](#quick-start). `border_width = 0` still draws a 1px border.

### Drag and drop

While you drag a window, a proxy follows the pointer. Over another tile, a second proxy shows where the window will land.

| option | type | default | description |
| --- | --- | --- | --- |
| `drag_drop_proxy_color` | color | `0x24EDB342` | Proxy fill before the 12px drag threshold |
| `drag_drop_proxy_active_color` | color | `0x3DEDB342` | Proxy fill once dragging, and the landing proxy's fill |
| `drag_drop_proxy_border_color` | string | empty | Proxy border, solid or gradient; empty uses `border_color_focus` |
| `drag_drop_proxy_border_width` | int | `-1` | `-1` uses `border_width + 1` with a minimum of 2; `0` disables |
| `drag_drop_proxy_rounding` | int | `-1` | Proxy corner radius in pixels; `-1` follows the focused tile's rounding |
| `drag_drop_source_border_color` | string | empty | Border of the tile being dragged from; empty uses `border_color_focus` |
| `drag_drop_source_border_width` | int | `-1` | `-1` uses `border_width`; `0` disables |

### Legacy options

These are still accepted so old configs load, but shouldn't be used in new ones.

| option | status |
| --- | --- |
| `border_grad_current`, `border_grad_focus`, `border_grad_hover` | Used only when the matching `border_color_*` is empty. `border_color_*` accepts gradients directly |
| `border_color` | No effect |
| `border_style` | No effect; solid or gradient is detected from the color value |

## Troubleshooting

**The plugin won't load, with an API or hash mismatch.** The plugin must be built against the exact Hyprland revision you're running. Rebuild it and load it again. If `pkg-config --modversion hyprland` doesn't print your running version, an old `hyprland.pc` is being picked up first. Remove it or point `PKG_CONFIG_PATH` at the right headers.

**Errors appear at startup, then go away.** Plugins usually load after your config first runs. hyprexpo reloads the config as soon as it loads, so anything that depends on it takes effect at that point.

**Keyboard binds do nothing while the overview is open.** Check that `hl.define_submap("hyprexpo", ...)` is in your config and that `keynav_enable` is on.

**A border doesn't appear.** An invalid solid color is logged to the Hyprland log and that border isn't drawn. An invalid gradient isn't drawn either, and isn't logged. Check the value against the formats in [Tile appearance](#tile-appearance).

**Hyprland crashed after replacing the plugin file.** Hyprland maps the plugin into its running process, so overwriting the file in place corrupts it. Use `make install` or `install`, never `cp`.

## Development

```bash
make test         # unit tests; no running Hyprland needed
make dev-load     # build to ~/.cache/hyprexpo and load it into the running session
make dev-reload   # rebuild, unload and load again
make install      # replace the hyprpm-managed copy, then run hyprpm reload
```

`make install` may need `sudo make install INSTALL_USER="$USER"` where hyprpm's cache is owned by root.

`scripts/run-nested.sh` and `scripts/dev-watch.sh` generate a hyprlang config for a nested session, so they need updating to Lua before they'll work on Hyprland 0.57. Until then, test in a session you don't mind losing.

Before a release, check at least:

- opening, closing and cancelling
- keyboard focus and confirm, including landing on an empty tile
- digit selection
- clicking and touch selection
- dragging a window between tiles on one monitor, across monitors, and onto an empty tile
- selecting a workspace from another monitor
- the pinned window preview option
- gesture registration

See [RELEASING.md](RELEASING.md) for how versions are cut and published.

## Credits

This is a fork of [sandwichfarm/hyprexpo](https://github.com/sandwichfarm/hyprexpo) (formerly HyprExpo+), which continued the original hyprexpo after it was [retired](https://github.com/hyprwm/hyprland-plugins/pull/663) from [hyprland-plugins](https://github.com/hyprwm/hyprland-plugins). Thanks to everyone who worked on both.

Bugs and suggestions are welcome in the [issue tracker](https://github.com/aarushth/hyprexpo/issues). See [LICENSE](LICENSE) for license terms.
