#pragma once

#define WLR_USE_UNSTABLE

#include "globals.hpp"
#include "HyprexpoLogic.hpp"
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/render/Framebuffer.hpp>
#include <hyprland/src/render/Texture.hpp>
#include <hyprland/src/helpers/AnimatedVariable.hpp>
#include <hyprland/src/helpers/signal/Signal.hpp>
#include <hyprland/src/managers/eventLoop/EventLoopTimer.hpp>
#include <string>
#include <vector>

// saves on resources, but is a bit broken rn with blur.
// hyprland's fault, but cba to fix.
constexpr bool ENABLE_LOWRES = false;

namespace Render {
    class CRenderContext;
}

// Tiles identify workspaces by selector ("3", "name:foo"): since Hyprland 0.57
// named workspaces have no numeric ID. Empty means a padding tile.
std::string  workspaceSelector(const PHLWORKSPACE& workspace);
PHLWORKSPACE findWorkspaceBySelector(const std::string& selector);
// Selector of the next empty numbered workspace (the "emptynm" rule), or empty on failure.
std::string emptyWorkspaceSelector();

class COverview {
  public:
    COverview(PHLWORKSPACE startedOn_, bool swipe = false);
    ~COverview();

    void render(Render::CRenderContext& ctx);
    void damage();
    void onDamageReported();
    void onPreRender();

    void setClosing(bool closing);
    // True once close() has armed the teardown animation. Further gestures must
    // be ignored until the overview is destroyed, otherwise a second swipe
    // rewinds the in-flight close animation (the close "replays" from ~80%).
    bool closeCommitted() const {
        return m_closeCommitted;
    }
    bool shouldRenderOverviewForMonitor(const PHLMONITOR& monitor) const;
    void onWindowMoveToWorkspace(const PHLWINDOW& window, const PHLWORKSPACE& workspace);

    void resetSwipe();
    void onSwipeUpdate(double delta);
    void onSwipeEnd();

    // close without a selection
    void          close(bool switchToSelection = true);
    void          selectHoveredWorkspace();

    // keyboard navigation interface
    void          onKbMoveFocus(const std::string& dir);
    void          onKbConfirm();
    void          onKbSelectNumber(int num);
    void          onKbSelectToken(int visibleIdx);
    bool          selectVisibleToken(const std::string& token);
    std::string   selectedWorkspace() const;
    bool          selectWorkspace(const std::string& selector);
    bool          selectVisibleIndex(size_t index);
    bool          moveWindowBetweenVisibleIndices(size_t sourceIndex, size_t targetIndex, const PHLWINDOW& window = nullptr);

    bool          blockOverviewRendering = false;
    bool          blockDamageReporting   = false;

    PHLMONITORREF pMonitor;
    bool          m_isSwiping = false;

    struct SWorkspaceImage {
        SP<Render::IFramebuffer> fb;
        std::string              workspace;
        PHLWORKSPACE             pWorkspace;
        CBox                     box;
    };

  private:
    void       redrawID(int id, bool forcelowres = false);
    void       redrawAll(bool forcelowres = false);
    void       onWorkspaceChange();
    void       fullRender(Render::CRenderContext& ctx);
    void       updateHoveredFromMouse();
    void       ensureKbFocusInitialized();
    bool       isTileValid(int id) const;
    void       moveFocus(int dx, int dy);
    void       moveFocusLinear(int step);
    int        tileForWorkspace(const std::string& selector) const;
    int        tileForVisibleIndex(int vIdx) const;
    void       beginWindowDrag();
    bool       finishWindowDrag();
    void       updateWindowDrag();
    void       redrawDraggedWindowTiles(int source, int target);
    void       queueRedrawID(int id);
    void       flushQueuedRedraws();
    PHLWINDOW  windowAtTilePoint(int id, const Vector2D& localPoint) const;
    Vector2D   tilePointToWorkspacePoint(int id, const Vector2D& localPoint) const;
    PHLWORKSPACE ensureWorkspaceForTile(int id);
    bool         isDropTargetTile(int id) const;
    void         assignWorkspaceToEmptyTile(int id);
    void       enterSubmapIfEnabled();
    void       resetSubmapIfNeeded();

    int        SIDE_LENGTH = 3;
    int        GAP_WIDTH   = 5;
    CHyprColor BG_COLOR    = CHyprColor{0.1, 0.1, 0.1, 1.0};

    bool       damageDirty = false;

    Vector2D                     lastMousePosLocal = Vector2D{};

    int                          openedID  = -1;
    int                          closeOnID = -1;
    int                          kbFocusID = -1;
    int                          hoveredID = -1;
    bool                         submapActive = false;

    Vector2D                     dragStartLocal = Vector2D{};
    int                          dragSourceID   = -1;
    bool                         dragMoved      = false;
    Vector2D                     dragGrabOffset = Vector2D{};
    PHLWINDOW                    dragWindow;
    int                          dropIntentTargetID = -1;
    Hyprexpo::SDropIntentGeometry dropIntent;

    std::vector<int>             queuedRedrawIDs;
    std::vector<int>             settlingRedrawIDs;
    int                          redrawSettleTicks = 0;
    SP<CEventLoopTimer>          redrawSettleTimer;

    std::vector<SWorkspaceImage> images;

    PHLWORKSPACE                 startedOn;

    PHLANIMVAR<Vector2D>         size;
    PHLANIMVAR<Vector2D>         pos;

    bool                         closing = false;
    bool                         m_closeCommitted = false;
    bool                         externalWorkspaceMoveDuringClose = false;

    CHyprSignalListener          mouseMoveHook;
    CHyprSignalListener          mouseButtonHook;
    CHyprSignalListener          touchMoveHook;
    CHyprSignalListener          touchDownHook;
    CHyprSignalListener          workspaceMoveHook;

    bool                         swipe             = false;
    bool                         swipeWasCommenced = false;

    friend class COverviewPassElement;
};

inline std::unique_ptr<COverview> g_pOverview;
