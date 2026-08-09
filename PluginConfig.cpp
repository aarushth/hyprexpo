#define WLR_USE_UNSTABLE

#include "PluginConfig.hpp"

#include "Dispatchers.hpp"
#include "globals.hpp"
#include "HyprexpoConfig.hpp"
#include <hyprland/src/config/values/types/ColorValue.hpp>
#include <hyprland/src/config/values/types/FloatValue.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>
#include <hyprland/src/config/values/types/StringValue.hpp>

static void addConfigValue(SP<Config::Values::IValue> value) {
    HyprlandAPI::addConfigValueV2(PHANDLE, value);
}

void registerHyprexpoConfigValues() {
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:gaps_in", "inner gaps", HyprexpoConfig::GAPS_IN_DEFAULT));
    addConfigValue(makeShared<Config::Values::CColorValue>("plugin:hyprexpo:bg_col", "background color", HyprexpoConfig::BG_COL_DEFAULT));

    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:gesture_distance", "gesture distance", HyprexpoConfig::GESTURE_DISTANCE_DEFAULT));
    addConfigValue(createCancelKeyConfig());
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:show_cursor", "show cursor during overview", HyprexpoConfig::SHOW_CURSOR_DEFAULT));
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:show_pinned_windows", "show pinned windows in previews", HyprexpoConfig::SHOW_PINNED_WINDOWS_DEFAULT));

    // keyboard navigation + styling
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:keynav_enable", "key navigation enable", HyprexpoConfig::KEYNAV_ENABLE_DEFAULT));
    // Border configuration - supports both solid colors and gradients
    // Solid: rgb(rrggbb) or 0xAARRGGBB
    // Gradient: rgba(rrggbbaa) rgba(rrggbbaa) 45deg
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:border_width", "border width", HyprexpoConfig::BORDER_WIDTH_DEFAULT));
    addConfigValue(makeShared<Config::Values::CStringValue>("plugin:hyprexpo:border_color", "border color", HyprexpoConfig::BORDER_COLOR_DEFAULT));           // default border (unused tiles)
    addConfigValue(makeShared<Config::Values::CStringValue>("plugin:hyprexpo:border_color_current", "current border color", HyprexpoConfig::BORDER_COLOR_CURRENT_DEFAULT));
    addConfigValue(makeShared<Config::Values::CStringValue>("plugin:hyprexpo:border_color_focus", "focus border color", HyprexpoConfig::BORDER_COLOR_FOCUS_DEFAULT));
    addConfigValue(makeShared<Config::Values::CStringValue>("plugin:hyprexpo:border_color_hover", "hover border color", HyprexpoConfig::BORDER_COLOR_HOVER_DEFAULT));
    // Deprecated but supported for backwards compatibility
    addConfigValue(makeShared<Config::Values::CStringValue>("plugin:hyprexpo:border_style", "border style", HyprexpoConfig::BORDER_STYLE_DEFAULT));     // ignored, auto-detected from format

    // Drag/drop window movement styling. Empty border specs inherit the focused tile border.
    addConfigValue(makeShared<Config::Values::CColorValue>("plugin:hyprexpo:drag_drop_proxy_color", "drag/drop proxy color", HyprexpoConfig::DRAG_DROP_PROXY_COLOR_DEFAULT));
    addConfigValue(makeShared<Config::Values::CColorValue>("plugin:hyprexpo:drag_drop_proxy_active_color", "drag/drop active proxy color", HyprexpoConfig::DRAG_DROP_PROXY_ACTIVE_COLOR_DEFAULT));
    addConfigValue(makeShared<Config::Values::CStringValue>("plugin:hyprexpo:drag_drop_proxy_border_color", "drag/drop proxy border color", HyprexpoConfig::DRAG_DROP_PROXY_BORDER_COLOR_DEFAULT));
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:drag_drop_proxy_border_width", "drag/drop proxy border width", HyprexpoConfig::DRAG_DROP_PROXY_BORDER_WIDTH_DEFAULT));
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:drag_drop_proxy_rounding", "drag/drop proxy rounding", HyprexpoConfig::DRAG_DROP_PROXY_ROUNDING_DEFAULT));
    addConfigValue(makeShared<Config::Values::CStringValue>("plugin:hyprexpo:drag_drop_source_border_color", "drag/drop source border color", HyprexpoConfig::DRAG_DROP_SOURCE_BORDER_COLOR_DEFAULT));
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:drag_drop_source_border_width", "drag/drop source border width", HyprexpoConfig::DRAG_DROP_SOURCE_BORDER_WIDTH_DEFAULT));

    // tile rounding (rounded corners for workspace previews)
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:tile_rounding", "tile rounding", HyprexpoConfig::TILE_ROUNDING_DEFAULT));
    addConfigValue(makeShared<Config::Values::CFloatValue>("plugin:hyprexpo:tile_rounding_power", "tile rounding power", HyprexpoConfig::TILE_ROUNDING_POWER_DEFAULT));
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:tile_rounding_focus", "focus tile rounding", HyprexpoConfig::TILE_ROUNDING_FOCUS_DEFAULT));
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:tile_rounding_current", "current tile rounding", HyprexpoConfig::TILE_ROUNDING_CURRENT_DEFAULT));
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:tile_rounding_hover", "hover tile rounding", HyprexpoConfig::TILE_ROUNDING_HOVER_DEFAULT));

    // gaps
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:gaps_out", "outer gaps", HyprexpoConfig::GAPS_OUT_DEFAULT));
    // Deprecated: use border_color_* instead (supports both solid and gradient)
    addConfigValue(makeShared<Config::Values::CStringValue>("plugin:hyprexpo:border_grad_current", "current border gradient", HyprexpoConfig::BORDER_GRAD_CURRENT_DEFAULT));
    addConfigValue(makeShared<Config::Values::CStringValue>("plugin:hyprexpo:border_grad_focus", "focus border gradient", HyprexpoConfig::BORDER_GRAD_FOCUS_DEFAULT));
    addConfigValue(makeShared<Config::Values::CStringValue>("plugin:hyprexpo:border_grad_hover", "hover border gradient", HyprexpoConfig::BORDER_GRAD_HOVER_DEFAULT));
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:keynav_wrap_h", "key navigation horizontal wrap", HyprexpoConfig::KEYNAV_WRAP_H_DEFAULT));
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:keynav_wrap_v", "key navigation vertical wrap", HyprexpoConfig::KEYNAV_WRAP_V_DEFAULT));
    // default off: spatial moves by default
    addConfigValue(makeShared<Config::Values::CIntValue>("plugin:hyprexpo:keynav_reading_order", "key navigation reading order", HyprexpoConfig::KEYNAV_READING_ORDER_DEFAULT));
}
