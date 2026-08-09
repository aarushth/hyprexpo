#pragma once

namespace HyprexpoConfig {
inline constexpr int         GAPS_IN_DEFAULT                 = 5;
inline constexpr int         GAPS_OUT_DEFAULT                = 0;
inline constexpr unsigned    BG_COL_DEFAULT                  = 0xFF111111;
inline constexpr int         GESTURE_DISTANCE_DEFAULT        = 200;
inline constexpr const char* CANCEL_KEY_DEFAULT              = "escape";
inline constexpr int         SHOW_CURSOR_DEFAULT             = 1;
inline constexpr int         SHOW_PINNED_WINDOWS_DEFAULT     = 0;
inline constexpr int         KEYNAV_ENABLE_DEFAULT           = 1;
inline constexpr int         KEYNAV_WRAP_H_DEFAULT           = 1;
inline constexpr int         KEYNAV_WRAP_V_DEFAULT           = 1;
inline constexpr int         KEYNAV_READING_ORDER_DEFAULT    = 0;
inline constexpr int         BORDER_WIDTH_DEFAULT            = 2;
inline constexpr const char* BORDER_COLOR_DEFAULT            = "";
inline constexpr const char* BORDER_COLOR_CURRENT_DEFAULT    = "rgb(66ccff)";
inline constexpr const char* BORDER_COLOR_FOCUS_DEFAULT      = "rgb(ffcc66)";
inline constexpr const char* BORDER_COLOR_HOVER_DEFAULT      = "rgb(aabbcc)";
inline constexpr const char* BORDER_STYLE_DEFAULT            = "simple";
inline constexpr const char* BORDER_GRAD_CURRENT_DEFAULT     = "";
inline constexpr const char* BORDER_GRAD_FOCUS_DEFAULT       = "";
inline constexpr const char* BORDER_GRAD_HOVER_DEFAULT       = "";
inline constexpr unsigned    DRAG_DROP_PROXY_COLOR_DEFAULT        = 0x24EDB342;
inline constexpr unsigned    DRAG_DROP_PROXY_ACTIVE_COLOR_DEFAULT = 0x3DEDB342;
inline constexpr const char* DRAG_DROP_PROXY_BORDER_COLOR_DEFAULT = "";
inline constexpr int         DRAG_DROP_PROXY_BORDER_WIDTH_DEFAULT = -1;
inline constexpr int         DRAG_DROP_PROXY_ROUNDING_DEFAULT     = -1;
inline constexpr const char* DRAG_DROP_SOURCE_BORDER_COLOR_DEFAULT = "";
inline constexpr int         DRAG_DROP_SOURCE_BORDER_WIDTH_DEFAULT = -1;
inline constexpr int         TILE_ROUNDING_DEFAULT           = 0;
inline constexpr float       TILE_ROUNDING_POWER_DEFAULT     = 2.0F;
inline constexpr int         TILE_ROUNDING_FOCUS_DEFAULT     = -1;
inline constexpr int         TILE_ROUNDING_CURRENT_DEFAULT   = -1;
inline constexpr int         TILE_ROUNDING_HOVER_DEFAULT     = -1;
}
