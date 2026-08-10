#include "HyprlandConfigCompat.hpp"
#define HyprlandAPI CompatHyprlandAPI
#include "OverviewInternal.hpp"
#include "HyprexpoLogic.hpp"
#include "OverviewPassElement.hpp"
#define private   public
#define protected public
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/config/shared/actions/ConfigActions.hpp>
#include <hyprland/src/config/shared/complex/ComplexDataTypes.hpp>
#include <hyprland/src/animation/WorkspaceAnimationController.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/helpers/time/Time.hpp>
#include <hyprland/src/state/WorkspaceState.hpp>
#include <hyprland/src/state/WorkspacePlacementController.hpp>
#undef private
#undef protected
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

void COverview::redrawID(int id, bool forcelowres) {
    const auto MON = pMonitor.lock();
    if (!MON)
        return;

    if (MON->m_activeWorkspace != startedOn && !closing) {
        // likely user changed.
        onWorkspaceChange();
    }

    blockOverviewRendering = true;

    Render::GL::g_pHyprOpenGL->makeEGLCurrent();
    settleWorkspaceMoveAnimations();

    id = std::clamp(id, 0, SIDE_LENGTH * SIDE_LENGTH - 1);

    auto& image = images[id];

    const auto restoreWorkspace = MON->m_activeWorkspace;

    startedOn->m_visible = false;

    captureWorkspaceTile(MON, image, startedOn);

    const auto activeWorkspace = restoreWorkspace ? restoreWorkspace : startedOn;
    MON->m_activeWorkspace = activeWorkspace;
    if (activeWorkspace) {
        activeWorkspace->m_visible = true;
        if (activeWorkspace == startedOn)
            Animation::Workspace::startAnimation(activeWorkspace, Animation::Workspace::ANIMATION_TYPE_IN, true, true);
    }

    blockOverviewRendering = false;
}

void COverview::redrawAll(bool forcelowres) {
    const auto MON = pMonitor.lock();
    if (!MON)
        return;

    for (size_t i = 0; i < (size_t)(SIDE_LENGTH * SIDE_LENGTH); ++i) {
        redrawID(i, forcelowres);
    }
}

void COverview::damage() {
    const auto MON = pMonitor.lock();
    if (!MON)
        return;

    blockDamageReporting = true;
    g_pHyprRenderer->damageMonitor(MON);
    blockDamageReporting = false;
}

void COverview::onDamageReported() {
    const auto MON = pMonitor.lock();
    if (!MON)
        return;

    damageDirty = true;

    Vector2D SIZE = size->value();

    Vector2D tileSize       = (SIZE / SIDE_LENGTH);
    const auto GAPSIZE      = (closing ? (1.0 - size->getPercent()) : size->getPercent()) * GAP_WIDTH;
    static auto* const* PGAPSO = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:gaps_out")->getDataStaticPtr();
    const float OUTER       = std::max<Hyprlang::INT>(0, **PGAPSO) * (closing ? (1.0 - size->getPercent()) : size->getPercent());
    Vector2D tileRenderSize = (SIZE - Vector2D{GAPSIZE, GAPSIZE} * (SIDE_LENGTH - 1) - Vector2D{OUTER * 2, OUTER * 2}) / SIDE_LENGTH;
    // const auto& TILE           = images[std::clamp(openedID, 0, SIDE_LENGTH * SIDE_LENGTH)];
    CBox texbox = CBox{OUTER + (openedID % SIDE_LENGTH) * tileRenderSize.x + (openedID % SIDE_LENGTH) * GAPSIZE,
                       OUTER + (openedID / SIDE_LENGTH) * tileRenderSize.y + (openedID / SIDE_LENGTH) * GAPSIZE, tileRenderSize.x, tileRenderSize.y}
                      .translate(MON->m_position);

    damage();

    blockDamageReporting = true;
    g_pHyprRenderer->damageBox(texbox);
    blockDamageReporting = false;
    MON->scheduleFrame();
}

void COverview::close(bool switchToSelection) {
    if (closing)
        return;

    // The teardown animation is now committed; lock out further swipe input so a
    // re-grabbed gesture can't rewind it (issue #57 follow-up: close replay).
    m_closeCommitted = true;

    const auto MON = pMonitor.lock();
    if (!MON) {
        closing = true;
        g_pOverview.reset();
        return;
    }

    resetSubmapIfNeeded();

    const int   ID = closeOnID == -1 ? openedID : closeOnID;

    const int   SAFEID = std::clamp(ID, 0, SIDE_LENGTH * SIDE_LENGTH - 1);
    const auto& TILE   = images[SAFEID];

    Vector2D    tileSize = (MON->m_size / SIDE_LENGTH);

    *size = MON->m_size * MON->m_size / tileSize;
    *pos  = (-((MON->m_size / (double)SIDE_LENGTH) * Vector2D{SAFEID % SIDE_LENGTH, SAFEID / SIDE_LENGTH}) * MON->m_scale) * (MON->m_size / tileSize);

    closing = true;

    redrawAll();

    if (switchToSelection && TILE.workspaceID != MON->activeWorkspaceID()) {
        MON->setSpecialWorkspace(0);

        // If this tile's workspace was WORKSPACE_INVALID, move to the next
        // empty workspace. This should only happen if skip_empty is on, in
        // which case some tiles will be left with this ID intentionally.
        const int  NEWID = TILE.workspaceID == WORKSPACE_INVALID ? getWorkspaceIDNameFromString("emptynm").id : TILE.workspaceID;

        PHLWORKSPACE NEWIDWS;
        for (const auto& w : State::workspaceState()->workspacesCopy()) {
            if (w->m_id == NEWID) {
                NEWIDWS = w;
                break;
            }
        }

        const auto OLDWS            = MON->m_activeWorkspace;
        const auto OTHERMON         = NEWIDWS ? NEWIDWS->m_monitor.lock() : nullptr;
        const bool crossMonitorSwap = NEWIDWS && OTHERMON && OTHERMON != MON;

        if (crossMonitorSwap) {
            // Config::Actions::changeWorkspaceOnCurrentMonitor resolves "current
            // monitor" via global focus state (Desktop::focusState()->monitor()),
            // NOT the monitor passed in - if focus wasn't actually on MON at this
            // instant, the real swap lands on a different monitor pair than
            // expected, leaving windows laid out against the wrong monitor's
            // geometry (rendered blank, or offset by the size difference between
            // monitors). Call the same underlying primitive that dispatcher uses,
            // but with both monitors explicit and unambiguous.
            if (OTHERMON->activeWorkspaceID() == NEWIDWS->m_id) {
                // Already the active workspace on OTHERMON: swapActiveWorkspaces
                // directly sets both monitors' m_activeWorkspace, so this alone
                // finishes the switch.
                State::workspacePlacementController()->swapActiveWorkspaces(OTHERMON, MON);
            } else {
                // Not currently visible on OTHERMON: moveWorkspaceToMonitor only
                // relocates ownership (workspace->m_monitor) here - it only makes
                // the workspace active on the destination monitor if it was
                // already active on its old one, which by construction it isn't
                // in this branch. Without an explicit follow-up switch, the
                // workspace silently becomes ours without ever being displayed,
                // requiring a second attempt (now same-monitor) to actually see
                // it.
                State::workspacePlacementController()->moveWorkspaceToMonitor(NEWIDWS, MON, true);
                const auto CHANGE = Config::Actions::changeWorkspace(NEWIDWS->getConfigName());
                if (!CHANGE)
                    Log::logger->log(Log::ERR, "[hyprexpo] failed to change workspace: {}", CHANGE.error().message);
            }

            (void)Config::Actions::focusMonitor(MON);
        } else {
            const auto CHANGE = !NEWIDWS ? Config::Actions::changeWorkspace(std::to_string(NEWID)) : Config::Actions::changeWorkspace(NEWIDWS->getConfigName());
            if (!CHANGE)
                Log::logger->log(Log::ERR, "[hyprexpo] failed to change workspace: {}", CHANGE.error().message);
        }

        Animation::Workspace::startAnimation(MON->m_activeWorkspace, Animation::Workspace::ANIMATION_TYPE_IN, true, true);

        // OLDWS animates OUT only when it is genuinely being replaced. After a
        // cross-monitor swap it is not going away at all - it has become the
        // active, visible workspace on the other monitor, so sliding it out
        // (which offsets it by ~a monitor width via m_renderOffset) leaves that
        // monitor blank or visibly shifted until something resets the offset.
        // It is arriving there, so it animates IN, on its own monitor.
        if (crossMonitorSwap)
            Animation::Workspace::startAnimation(OLDWS, Animation::Workspace::ANIMATION_TYPE_IN, true, true);
        else
            Animation::Workspace::startAnimation(OLDWS, Animation::Workspace::ANIMATION_TYPE_OUT, false, true);

        startedOn = MON->m_activeWorkspace;
    }

    size->setCallbackOnEnd(removeOverview);
}

void COverview::onPreRender() {
    if (damageDirty) {
        damageDirty = false;
        redrawID(closing ? (closeOnID == -1 ? openedID : closeOnID) : openedID);
    }
}

void COverview::onWorkspaceChange() {
    const auto MON = pMonitor.lock();
    if (!MON)
        return;

    if (valid(startedOn))
        Animation::Workspace::startAnimation(startedOn, Animation::Workspace::ANIMATION_TYPE_OUT, false, true);
    else
        startedOn = MON->m_activeWorkspace;

    // Every monitor's active workspace is guaranteed present in images[] by
    // construction (see the ctor), so this should always find a match. If it
    // doesn't (e.g. an out-of-band workspace switch to an ID that didn't
    // exist when the overview opened), openedID keeps its previous value.
    for (size_t i = 0; i < (size_t)(SIDE_LENGTH * SIDE_LENGTH); ++i) {
        if (images[i].workspaceID != MON->activeWorkspaceID())
            continue;

        openedID = i;
        break;
    }

    closeOnID = openedID;
    close();
}

void COverview::render() {
    g_pHyprRenderer->m_renderPass.add(makeUnique<COverviewPassElement>());
}

bool COverview::shouldRenderOverviewForMonitor(const PHLMONITOR& monitor) const {
    if (pMonitor != monitor)
        return false;

    const auto MON = pMonitor.lock();
    if (!MON)
        return false;

    if (closing && (externalWorkspaceMoveDuringClose || MON->m_activeWorkspace != startedOn))
        return false;

    return true;
}

void COverview::fullRender() {
    const auto MON = pMonitor.lock();
    if (!MON)
        return;

    const auto GAPSIZE = (closing ? (1.0 - size->getPercent()) : size->getPercent()) * GAP_WIDTH;

    if (MON->m_activeWorkspace != startedOn && !closing) {
        // likely user changed.
        onWorkspaceChange();
    }

    Vector2D SIZE = size->value();

    static auto* const* PGAPSO = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:gaps_out")->getDataStaticPtr();
    const float OUTER = std::max<Hyprlang::INT>(0, **PGAPSO) * (closing ? (1.0 - size->getPercent()) : size->getPercent());

    Vector2D tileSize       = (SIZE / SIDE_LENGTH);
    Vector2D tileRenderSize = (SIZE - Vector2D{GAPSIZE, GAPSIZE} * (SIDE_LENGTH - 1) - Vector2D{OUTER * 2, OUTER * 2}) / SIDE_LENGTH;

    clearWithColor(BG_COLOR.stripA());

    static auto* const* PTILEROUND = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:tile_rounding")->getDataStaticPtr();
    static auto* const* PTOUNDPWR  = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:tile_rounding_power")->getDataStaticPtr();
    static auto* const* PTILEROUNDF = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:tile_rounding_focus")->getDataStaticPtr();
    static auto* const* PTILEROUNDC = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:tile_rounding_current")->getDataStaticPtr();
    static auto* const* PTILEROUNDH = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:tile_rounding_hover")->getDataStaticPtr();

    const int BASE_ROUND_SCALED   = std::max(0, (int)std::lround((double)**PTILEROUND * MON->m_scale));
    const int FOCUS_ROUND_SCALED  = **PTILEROUNDF >= 0 ? std::max(0, (int)std::lround((double)**PTILEROUNDF * MON->m_scale)) : BASE_ROUND_SCALED;
    const int CURRENT_ROUND_SCALED= **PTILEROUNDC >= 0 ? std::max(0, (int)std::lround((double)**PTILEROUNDC * MON->m_scale)) : BASE_ROUND_SCALED;
    const int HOVER_ROUND_SCALED  = **PTILEROUNDH >= 0 ? std::max(0, (int)std::lround((double)**PTILEROUNDH * MON->m_scale)) : BASE_ROUND_SCALED;
    const float ROUND_PWR         = **PTOUNDPWR;

    // (shadows moved to feature/shadows branch)

    std::vector<CBox> tileBoxes(images.size());

    for (size_t y = 0; y < (size_t)SIDE_LENGTH; ++y) {
        for (size_t x = 0; x < (size_t)SIDE_LENGTH; ++x) {
            const int id = x + y * SIDE_LENGTH;
            CBox      texbox{OUTER + x * tileRenderSize.x + x * GAPSIZE, OUTER + y * tileRenderSize.y + y * GAPSIZE, tileRenderSize.x, tileRenderSize.y};
            texbox.scale(MON->m_scale).translate(pos->value());
            texbox.round();
            tileBoxes[id] = texbox;
            // per-tile rounding override for focus/current/hover (priority: focus > current > hover)
            int tileRound = BASE_ROUND_SCALED;
            if ((int)id == kbFocusID)
                tileRound = FOCUS_ROUND_SCALED;
            else if ((int)id == openedID)
                tileRound = CURRENT_ROUND_SCALED;
            else if ((int)id == hoveredID)
                tileRound = HOVER_ROUND_SCALED;

            // clamp rounding to tile size
            const int maxCornerPx = std::max(0, (int)std::floor(std::min(texbox.w, texbox.h) / 2.0));
            tileRound = std::min(tileRound, maxCornerPx);

            // no shadow in this branch

            CRegion damage{0, 0, INT16_MAX, INT16_MAX};
            Render::GL::g_pHyprOpenGL->renderTextureInternal(images[id].fb->getTexture(), texbox, {.damage = &damage, .a = 1.0, .round = tileRound, .roundingPower = ROUND_PWR});
        }
    }

    // overlays: borders
    static auto* const* PBWIDTH      = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:border_width")->getDataStaticPtr();
    static auto  const* PBCOLCUR     = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:border_color_current")->getDataStaticPtr();
    static auto  const* PBCOLFOC     = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:border_color_focus")->getDataStaticPtr();
    static auto  const* PBCOLHOV     = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:border_color_hover")->getDataStaticPtr();
    // Deprecated configs for backwards compatibility
    static auto  const* PBGRCUR      = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:border_grad_current")->getDataStaticPtr();
    static auto  const* PBGREFOC     = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:border_grad_focus")->getDataStaticPtr();
    static auto  const* PBGREHOV     = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:border_grad_hover")->getDataStaticPtr();

    static auto* const* PDRAGPROXYCOL     = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:drag_drop_proxy_color")->getDataStaticPtr();
    static auto* const* PDRAGPROXYACTCOL  = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:drag_drop_proxy_active_color")->getDataStaticPtr();
    static auto  const* PDRAGPROXYBORDER  = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:drag_drop_proxy_border_color")->getDataStaticPtr();
    static auto* const* PDRAGPROXYBWIDTH  = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:drag_drop_proxy_border_width")->getDataStaticPtr();
    static auto* const* PDRAGPROXYROUND   = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:drag_drop_proxy_rounding")->getDataStaticPtr();
    static auto  const* PDRAGSOURCEBORDER = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:drag_drop_source_border_color")->getDataStaticPtr();
    static auto* const* PDRAGSOURCEBWIDTH = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprexpo:drag_drop_source_border_width")->getDataStaticPtr();

    // draw borders for hover, current and focus (priority order: focus > current > hover)

    // pass rounding based on state
    const int RND_CUR = CURRENT_ROUND_SCALED;
    const int RND_FOC = FOCUS_ROUND_SCALED;
    const int RND_HOV = HOVER_ROUND_SCALED;

    // Helper to parse border config (supports rgb/hex/gradient, with deprecated fallback)
    auto drawBorderForID = [&](int id, const std::string& borderSpec, const std::string& deprecatedGradSpec, int roundScaled, int borderWidthOverride = -1) {
        if (id < 0)
            return;
        if (borderWidthOverride == 0)
            return;
        const int ix = id % SIDE_LENGTH;
        const int iy = id / SIDE_LENGTH;
        CBox       box{OUTER + ix * tileRenderSize.x + ix * GAPSIZE, OUTER + iy * tileRenderSize.y + iy * GAPSIZE, tileRenderSize.x, tileRenderSize.y};
        box.scale(MON->m_scale).translate(pos->value());
        box.round();
        const int BWIDTH = std::max(1, borderWidthOverride > 0 ? borderWidthOverride : (int)**PBWIDTH);

        // Determine which spec to use (prefer new format, fallback to deprecated)
        std::string effectiveSpec = borderSpec.empty() ? deprecatedGradSpec : borderSpec;

        // Auto-detect format: gradient vs solid color
        if (isGradientBorderSpec(effectiveSpec)) {
            // Render as gradient border (hyprland style)
            const auto spec = parseGradientSpec(effectiveSpec);
            if (spec.valid) {
                Config::CGradientValueData grad;
                grad.m_colors.clear();
                grad.m_colors.push_back(spec.c1);
                grad.m_colors.push_back(spec.c2);
                grad.m_angle = spec.angleDeg * (float)M_PI / 180.f;
                grad.updateColorsOk();
                Render::GL::g_pHyprOpenGL->renderBorder(box, grad, {.round = roundScaled, .roundingPower = ROUND_PWR, .borderSize = BWIDTH});
            }
        } else if (!effectiveSpec.empty()) {
            Hyprexpo::SColorRGBA parsedColor;
            if (Hyprexpo::parseSolidColorSpec(effectiveSpec, parsedColor)) {
                CHyprColor color{parsedColor.r, parsedColor.g, parsedColor.b, parsedColor.a};
                Render::GL::g_pHyprOpenGL->renderBorder(box, color, {.round = roundScaled, .roundingPower = ROUND_PWR, .borderSize = BWIDTH});
            } else {
                Log::logger->log(Log::ERR, "[hyprexpo] invalid border color config: {}", effectiveSpec);
            }
        }
    };

    auto drawProxyBorder = [&](const CBox& proxy, int round, int borderWidth, const std::string& borderSpec, const std::string& fallbackSpec) {
        if (borderWidth <= 0)
            return;

        std::string effectiveSpec = borderSpec.empty() ? fallbackSpec : borderSpec;
        if (effectiveSpec.empty())
            return;

        if (isGradientBorderSpec(effectiveSpec)) {
            const auto spec = parseGradientSpec(effectiveSpec);
            if (!spec.valid)
                return;

            Config::CGradientValueData grad;
            grad.m_colors.clear();
            grad.m_colors.push_back(spec.c1);
            grad.m_colors.push_back(spec.c2);
            grad.m_angle = spec.angleDeg * (float)M_PI / 180.f;
            grad.updateColorsOk();
            Render::GL::g_pHyprOpenGL->renderBorder(proxy, grad, {.round = round, .roundingPower = ROUND_PWR, .borderSize = borderWidth});
            return;
        }

        Hyprexpo::SColorRGBA parsedColor;
        if (Hyprexpo::parseSolidColorSpec(effectiveSpec, parsedColor)) {
            Config::CGradientValueData grad{CHyprColor{parsedColor.r, parsedColor.g, parsedColor.b, parsedColor.a}};
            grad.updateColorsOk();
            Render::GL::g_pHyprOpenGL->renderBorder(proxy, grad, {.round = round, .roundingPower = ROUND_PWR, .borderSize = borderWidth});
        } else
            Log::logger->log(Log::ERR, "[hyprexpo] invalid drag_drop_proxy_border_color config: {}", effectiveSpec);
    };

    // Draw borders in order: hover (lowest), then current, then focus (highest priority)
    if (hoveredID != -1 && hoveredID != openedID && hoveredID != kbFocusID)
        drawBorderForID(hoveredID, std::string{*PBCOLHOV}, std::string{*PBGREHOV}, RND_HOV);
    drawBorderForID(openedID, std::string{*PBCOLCUR}, std::string{*PBGRCUR}, RND_CUR);
    if (kbFocusID != -1)
        drawBorderForID(kbFocusID, std::string{*PBCOLFOC}, std::string{*PBGREFOC}, RND_FOC);
    if (dragMoved && dragSourceID != -1) {
        const std::string sourceBorder = std::string{*PDRAGSOURCEBORDER}.empty() ? std::string{*PBCOLFOC} : std::string{*PDRAGSOURCEBORDER};
        const int         sourceWidth  = **PDRAGSOURCEBWIDTH >= 0 ? **PDRAGSOURCEBWIDTH : (int)**PBWIDTH;
        drawBorderForID(dragSourceID, sourceBorder, std::string{*PBGREFOC}, RND_FOC, sourceWidth);
    }

    dropIntent = {};
    dropIntentTargetID = -1;

    if (dragWindow && isTileValid(dragSourceID)) {
        const auto windowBox = dragWindow->getWindowMainSurfaceBox();
        if (windowBox.w > 0 && windowBox.h > 0) {
            const CBox&  sourceBox = tileBoxes[dragSourceID];
            const double scaleX    = sourceBox.w / MON->m_size.x;
            const double scaleY    = sourceBox.h / MON->m_size.y;
            const double minW      = std::min(sourceBox.w, 24.0 * MON->m_scale);
            const double minH      = std::min(sourceBox.h, 24.0 * MON->m_scale);

            CBox proxy{
                lastMousePosLocal.x * MON->m_scale - dragGrabOffset.x * scaleX,
                lastMousePosLocal.y * MON->m_scale - dragGrabOffset.y * scaleY,
                std::clamp(windowBox.w * scaleX, minW, sourceBox.w),
                std::clamp(windowBox.h * scaleY, minH, sourceBox.h),
            };
            proxy.round();

            const int maxProxyRound = std::max(0, (int)std::floor(std::min(proxy.w, proxy.h) / 2.0));
            const int autoRound     = std::min(RND_FOC, maxProxyRound);
            const int round         = **PDRAGPROXYROUND >= 0 ? std::min(std::max(0, (int)std::lround((double)**PDRAGPROXYROUND * MON->m_scale)), maxProxyRound) : autoRound;

            if (dragMoved && hoveredID != -1 && hoveredID != dragSourceID && isTileValid(hoveredID)) {
                const int tx = hoveredID % SIDE_LENGTH;
                const int ty = hoveredID / SIDE_LENGTH;
                const Hyprexpo::SRect targetTileLocal{
                    OUTER + tx * tileRenderSize.x + tx * GAPSIZE,
                    OUTER + ty * tileRenderSize.y + ty * GAPSIZE,
                    tileRenderSize.x,
                    tileRenderSize.y,
                };
                dropIntent = Hyprexpo::computeDropIntentGeometry({
                    .targetValid     = true,
                    .pointerLocal    = {lastMousePosLocal.x, lastMousePosLocal.y},
                    .targetTileLocal = targetTileLocal,
                    .workspaceSize   = {MON->m_size.x, MON->m_size.y},
                    .windowSize      = {windowBox.w, windowBox.h},
                    .grabOffset      = {dragGrabOffset.x, dragGrabOffset.y},
                });
                dropIntentTargetID = dropIntent.valid ? hoveredID : -1;
            }

            if (dropIntent.valid) {
                CBox targetProxy{
                    dropIntent.targetProxyLocal.x,
                    dropIntent.targetProxyLocal.y,
                    dropIntent.targetProxyLocal.w,
                    dropIntent.targetProxyLocal.h,
                };
                targetProxy.scale(MON->m_scale).translate(pos->value());
                targetProxy.round();

                const int targetMaxRound = std::max(0, (int)std::floor(std::min(targetProxy.w, targetProxy.h) / 2.0));
                const int targetRound    = **PDRAGPROXYROUND >= 0 ? std::min(std::max(0, (int)std::lround((double)**PDRAGPROXYROUND * MON->m_scale)), targetMaxRound) :
                                                                   std::min(RND_FOC, targetMaxRound);
                Render::GL::g_pHyprOpenGL->renderRect(targetProxy, CHyprColor{(uint64_t)**PDRAGPROXYACTCOL}, {.round = targetRound, .roundingPower = ROUND_PWR});

                const int   borderWidth   = **PDRAGPROXYBWIDTH >= 0 ? **PDRAGPROXYBWIDTH : std::max(2, (int)**PBWIDTH + 1);
                const std::string effectiveSpec = std::string{*PDRAGPROXYBORDER}.empty() ? std::string{*PBCOLFOC} : std::string{*PDRAGPROXYBORDER};
                drawProxyBorder(targetProxy, targetRound, borderWidth, effectiveSpec, std::string{*PBGREFOC});
            }

            Render::GL::g_pHyprOpenGL->renderRect(proxy, CHyprColor{(uint64_t)(dragMoved ? **PDRAGPROXYACTCOL : **PDRAGPROXYCOL)}, {.round = round, .roundingPower = ROUND_PWR});

            const int   borderWidth   = **PDRAGPROXYBWIDTH >= 0 ? **PDRAGPROXYBWIDTH : std::max(2, (int)**PBWIDTH + 1);
            std::string effectiveSpec = std::string{*PDRAGPROXYBORDER}.empty() ? std::string{*PBCOLFOC} : std::string{*PDRAGPROXYBORDER};
            if (effectiveSpec.empty())
                effectiveSpec = std::string{*PBGREFOC};
            drawProxyBorder(proxy, round, borderWidth, effectiveSpec, std::string{*PBGREFOC});
        }
    }
}
