/*
 * ImgWindow.h
 *
 * Integration for dear imgui into X-Plane.
 *
 * Copyright (C) 2018,2020 Christopher Collins
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef IMGWINDOW_H
#define IMGWINDOW_H

#include "SystemGL.h"

#include <climits>
#include <string>
#include <memory>

#include <XPLMDisplay.h>
#include <XPLMProcessing.h>
// Saar: mission-x
// imgui related
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

// end saar

#include <imgui.h>
#include <imgui_internal.h>

#include "../imgui/implot/implot.h" // v3.0.255.1

#include <queue>

#include "ImgFontAtlas.h"

#include "../../../src/ui/MxUICore.hpp" // v3.303.14 holds Font related data
#include "../../../src/core/xx_mission_constants.hpp" // v3.303.14 holds Font related data

#define NEW_FONT
//#define DISABLE_ORIG_CODE

/** ImgWindow is a Window for creating dear imgui widgets within.
 *
 * There's a few traps to be aware of when using dear imgui with X-Plane:
 *
 * 1) The Dear ImGUI coordinate scheme is inverted in the Y axis vs the X-Plane 
 *    (and OpenGL default) scheme. You must be careful if you're trying to 
 *    directly manipulate positioning of widgets rather than letting imgui 
 *    self-layout.  There are (private) functions in ImgWindow to do the 
 *    coordinate mapping.
 *
 * 2) The Dear ImGUI rendering space is only as big as the window - this means
 *    popup elements cannot be larger than the parent window.  This was
 *    unavoidable on XP11 because of how popup windows work and the possibility
 *    for negative coordinates (which imgui doesn't like).
 *
 * 3) There is no way to detect if the window is hidden without a per-frame
 *    processing loop or similar.
 *
 * @note It should be possible to map globally on XP9 & XP10 letting you run
 *     popups as large as you need, or to use the ImGUI native titlebars instead
 *     of the XP10 ones - source for this may be provided later, but could also
 *     be trivially adapted from this one by adjusting the way the space is
 *     translated and mapped in the DrawWindowCB and constructor.
 */
class
ImgWindow {
public:
    /** sFont1 is the global shared font-atlas.
     *
     * If you want to share fonts between windows, this needs to be set before
     * any dialogs are actually instantiated.  It will be automatically handed
     * over to the contexts as they're created.
     */
    //static std::shared_ptr<ImgFontAtlas> sFontAtlas;
    static std::shared_ptr<ImgFontAtlas> sFont1;
    static int                           defaultFontPos; // holds the loaded font that we will refer to as the default. Should always be equal to Zero as of v3.303.14

    virtual ~ImgWindow();
    
    /** Gets the current window geometry */
    void GetWindowGeometry (int& left, int& top, int& right, int& bottom) const
    { XPLMGetWindowGeometry(mWindowID, &left, &top, &right, &bottom); }
    
    /** Sets the current window geometry */
    void SetWindowGeometry (int left, int top, int right, int bottom)
    { XPLMSetWindowGeometry(mWindowID, left, top, right, bottom); }

    /** Gets the current window geometry of a popped out window */
    void GetWindowGeometryOS (int& left, int& top, int& right, int& bottom) const
    { XPLMGetWindowGeometryOS(mWindowID, &left, &top, &right, &bottom); }
    
    /** Sets the current window geometry of a popped out window */
    void SetWindowGeometryOS (int left, int top, int right, int bottom)
    { XPLMSetWindowGeometryOS(mWindowID, left, top, right, bottom); }

    /** Gets the current window size of window in VR */
    void GetWindowGeometryVR (int& width, int& height) const
    { XPLMGetWindowGeometryVR(mWindowID, &width, &height); }
    
    /** Sets the current window size of window in VR */
    void SetWindowGeometryVR (int width, int height)
    { XPLMSetWindowGeometryVR(mWindowID, width, height); }
    
    /** Gets the current valid geometry (free, OS, or VR
        If VR, then left=bottom=0 and right=width and top=height*/
    void GetCurrentWindowGeometry (int& left, int& top, int& right, int& bottom) const;
    
    /** Set resize limits. If set this way then the window object knows. */
    void SetWindowResizingLimits (int minW, int minH, int maxW, int maxH);

    /** SetVisible() makes the window visible after making the onShow() call.
     * It is also at this time that the window will be relocated onto the VR
     * display if the VR headset is in use.
     *
     * @param inIsVisible true to be displayed, false if the window is to be
     * hidden.
     */
    virtual void SetVisible(bool inIsVisible);

    /** GetVisible() returns the current window visibility.
     * @return true if the window is visible, false otherwise.
    */
    bool GetVisible() const;
    
    /** Is Window popped out */
    bool IsPoppedOut () const { return XPLMWindowIsPoppedOut(mWindowID) != 0; }

    /** Is Window in VR? */
    bool IsInVR () const { return XPLMWindowIsInVR(mWindowID) != 0; }
    
    /** Is Window inside the sim? */
    bool IsInsideSim () const { return !IsPoppedOut() && !IsInVR(); }
    
    /** Set the positioning mode
     * @see https://developer.x-plane.com/sdk/XPLMDisplay/#XPLMWindowPositioningMode */
    void SetWindowPositioningMode (XPLMWindowPositioningMode inPosMode,
                                   int                       inMonitorIdx = -1)
    { XPLMSetWindowPositioningMode (mWindowID, inPosMode, inMonitorIdx); }
    
    /** Bring window to front of Z-order */
    void BringWindowToFront () { XPLMBringWindowToFront(mWindowID); }
    
    /** Is Window in front of Z-order? */
    bool IsWindowInFront () const { return XPLMIsWindowInFront(mWindowID) != 0; }
    
    /** @brief Define Window drag area, ie. an area in which dragging with the mouse
     * moves the entire window.
     * @details Useful for windows without decoration.
     * For convenience (often you want a strip at the top of the window to be the drag area,
     * much like a little title bar), coordinates originate in the top-left
     * corner of the window and go right/down, ie. vertical axis is contrary
     * to what X-Plane usually uses, but in line with the ImGui system.
     * Right/Bottom can be set much large than window size just to extend the
     * drag area to the window's edges. So 0,0,INT_MAX,INT_MAX will surely
     * make the entire window the drag area.
     * @param left Left begin of drag area, relative to window's origin
     * @param top Ditto, top begin
     * @param right Ditto, right end
     * @param bottom Ditto, bottom end
     */
    void SetWindowDragArea (int left=0, int top=0, int right=INT_MAX, int bottom=INT_MAX);
    
    /** Clear the drag area, ie. stop the drag-the-window functionality */
    void ClearWindowDragArea ();
    
    /** Is a drag area defined? Return its sizes if wanted */
    bool HasWindowDragArea (int* pL = nullptr, int* pT = nullptr,
                            int* pR = nullptr, int* pB = nullptr) const;
    
    /** Is given position inside the defined drag area?
     * @param x Horizontal position in ImGui coordinates (0,0 in top/left corner)
     * @param y Vertical position in ImGui coordinates
     */
    bool IsInsideWindowDragArea (int x, int y) const;
    
protected:
    /** mFirstRender can be checked during buildInterface() to see if we're
     * being rendered for the first time or not.  This is particularly
     * important for windows that use Columns as SetColumnWidth() should only
     * be called once.
     *
     * There may be other times where it's advantageous to make specific ImGui
     * calls once and once only.
     */
    bool mFirstRender;

    /** Construct a window with the specified bounds
     *
     * @param left Left edge of the window's contents in global boxels.
     * @param top Top edge of the window's contents in global boxels.
     * @param right Right edge of the window's contents in global boxels.
     * @param bottom Bottom edge of the window's contents in global boxels.
     * @param decoration The decoration style to use (see notes)
     * @param layer the preferred layer to present this window in (see notes)
     *
     * @note The decoration should generally be one presented/rendered by XP -
     *     the ImGui window decorations are very intentionally supressed by
     *     ImgWindow to allow them to fit in with the rest of the simulator.
     *
     * @note The only layers that really make sense are Floating and Modal.  Do
     *     not set VR layer here however unless the window is ONLY to be
     *     rendered in VR.
     */
    ImgWindow(
        int left,
        int top,
        int right,
        int bottom, 
        XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle,
        XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows
        );
    
    ImgWindow(
        int left,
        int top,
        int right,
        int bottom, 
        std::shared_ptr<ImgFontAtlas>& inFontAtlas = ImgWindow::sFont1, // saar - dynamic font
        XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle,
        XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows
        );
    
    /** An ImgWindow object must not be copied!
     */
    ImgWindow (const ImgWindow&) = delete;
    ImgWindow& operator = (const ImgWindow&) = delete;

    ImGuiIO setFont(ImFontAtlas* iFontAtlas, std::shared_ptr<ImgFontAtlas>& inFont);

    /** SetWindowTitle sets the title of the window both in the ImGui layer and
     * at the XPLM layer.
     *
     * @param title the title to set.
     */
    void SetWindowTitle(const std::string &title);

    /** moveForVR() is an internal helper for moving the window to either it's
     * preferred layer or the VR layer depending on if the headset is in use.
     */
    void moveForVR();
    
    /** A hook called right before ImGui::Begin in case you want to set up something
     * before interface building begins
     * @return addition flags to be passed to the imgui::begin() call,
     *         like for example ImGuiWindowFlags_MenuBar */
    virtual ImGuiWindowFlags_ beforeBegin() { return ImGuiWindowFlags_None; }

    /** buildInterface() is the method where you can define your ImGui interface
     * and handle events.  It is called every frame the window is drawn.
     *
     * @note You must NOT delete the window object inside buildInterface() -
     *     use SafeDelete() for that.
     */
    virtual void buildInterface() = 0;

    /** A hook called after all rendering is done, right before the
     * X-Plane window draw call back returns
     * in case you want to do something that otherwise would conflict with rendering. */
    virtual void afterRendering() {}

    /** onShow() is called before making the Window visible.  It provides an
     * opportunity to prevent the window being shown.
     *
     * @note the implementation in the base-class is a null handler.  You can
     *     safely override this without chaining.
     *
     * @return true if the Window should be shown, false if the attempt to show
     *     should be suppressed.
     */
    virtual bool onShow();

    /** SafeDelete() can be used within buildInterface() to get the object to
     *     self-delete once it's finished rendering this frame.
     */
    void SafeDelete();
    
    /** Returns X-Plane's internal Window id */
    XPLMWindowID GetWindowId () const { return mWindowID; }

private:
    std::shared_ptr<ImgFontAtlas> mFontAtlas;

    static void DrawWindowCB(XPLMWindowID inWindowID, void *inRefcon);

    static int HandleMouseClickCB(
        XPLMWindowID inWindowID,
        int x, int y,
        XPLMMouseStatus inMouse,
        void *inRefcon);

    static void HandleKeyFuncCB(
        XPLMWindowID inWindowID,
        char inKey,
        XPLMKeyFlags inFlags,
        char inVirtualKey,
        void *inRefcon,
        int losingFocus);

    static XPLMCursorStatus HandleCursorFuncCB(
        XPLMWindowID inWindowID,
        int x, int y,
        void *inRefcon);

    static int HandleMouseWheelFuncCB(
        XPLMWindowID inWindowID,
        int x, int y,
        int wheel,
        int clicks,
        void *inRefcon);

    static int HandleRightClickFuncCB(
        XPLMWindowID inWindowID,
        int x, int y,
        XPLMMouseStatus inMouse,
        void *inRefcon);

    static float SelfDestructCallback(float inElapsedSinceLastCall,
                                      float inElapsedTimeSinceLastFlightLoop,
                                      int inCounter,
                                      void *inRefcon);
    static std::queue<ImgWindow *>  sPendingDestruction;
    static XPLMFlightLoopID         sSelfDestructHandler;

    int HandleMouseClickGeneric(
        int x, int y,
        XPLMMouseStatus inMouse,
        int button = 0);

    void RenderImGui(ImDrawData *draw_data);

    void updateImgui();

    void updateMatrices();

    void boxelsToNative(int x, int y, int &outX, int &outY);

    void translateImguiToBoxel(float inX, float inY, int &outX, int &outY);

    void translateToImguiSpace(int inX, int inY, float &outX, float &outY);

    float mModelView[16], mProjection[16];
    int mViewport[4];

    std::string mWindowTitle;

    XPLMWindowID mWindowID;
    ImGuiContext *mImGuiContext;
    ImPlotContext *mImPlotContext; // v3.0.255.1
    GLuint mFontTexture;

    int mTop;
    int mBottom;
    int mLeft;
    int mRight;

    XPLMWindowLayer mPreferredLayer;
    
    /** Shall reset the backspace key? (see HandleKeyFuncCB for details) */
    bool bResetBackspace = false; // v3.303.14 saar missionx - removed, we handle BACKspace individually we do the down + up at the same instance 
    /** Set if `xplm_WindowDecorationSelfDecoratedResizable`,
     *  ie. we need to handle resizing ourselves: X-Plane provides
     *  the "hand" mouse icon but as we catch mouse events X-Plane
     *  cannot handle resizing. (And passing on mouse events is
     *  discouraged.
     *  @see https://developer.x-plane.com/sdk/XPLMHandleMouseClick_f/ */
    const bool bHandleWndResize;
    
    /** Resize limits. There's no way to query XP, so we need to keep track ourself */
    int minWidth    = 100;
    int minHeight   = 100;
    int maxWidth    = INT_MAX;
    int maxHeight   = INT_MAX;

    /** Window drag area in ImGui coordinates (0,0 is top/left corner) */
    int dragLeft    = -1;
    int dragTop     = -1;
    int dragRight   = -1;       // right > left
    int dragBottom  = -1;       // bottom > top!
    
    /** Last (processed) mouse drag pos while moving/resizing */
    int lastMouseDragX  = -1;
    int lastMouseDragY  = -1;
    
    /** What are we dragging right now? */
    struct DragTy {
        bool wnd    : 1;
        bool left   : 1;
        bool top    : 1;
        bool right  : 1;
        bool bottom : 1;
        
        DragTy () { clear(); }
        void clear () { wnd = left = top = right = bottom = false; }
        operator bool() const { return wnd || left || top || right || bottom; }
    } dragWhat;

    public:
      // saar: for Mission-X
      // This will replace the flight call back integration. In Mission-X there is only one flight call back managed from plugin() and Mission::flc()
      virtual void flc() = 0;
      void toggleWindowState();

      /////// These functions where originaly in "starter window" and in their oen namespace. I don't see the reason not to move them into the base window class.
      /// @brief Helper for creating unique IDs
      /// @details Required when creating many widgets in a loop, e.g. in a table
      ///
      void PushID_formatted(const char* format, ...); // v3.303.14
//#ifdef APL
//      void PushID_formatted(const char* format, ...);
//#else
//      void PushID_formatted(const char* format, ...);
//  void PushID_formatted(const char* format, ...)    IM_FMTARGS(1);
//#endif
      /// @brief Button with on-hover popup helper text
      /// @param label Text on Button
      /// @param tip Tooltip text when hovering over the button (or NULL of none)
      /// @param colFg Foreground/text color (optional, otherwise no change)
      /// @param colBg Background color (optional, otherwise no change)
      /// @param size button size, 0 for either axis means: auto size

      bool ButtonTooltip(const char* label,
        const char* tip = nullptr,
        ImU32 colFg = IM_COL32(1, 1, 1, 0),
        ImU32 colBg = IM_COL32(1, 1, 1, 0),
        const ImVec2& size = ImVec2(0, 0));

      static ImVec4 getColorAsImVec4(const std::string& inColor_s);

      static void HelpMarker(const char* desc, ImVec4 inTextColor = ImVec4( 1.000f, 1.000f, 1.000f, 1.0f )); // from IMGUI demo // v3.303.14 added default color
      static void mxUiHelpMarker(ImVec4 inTextColor,const char* desc); // saar, missionx. Prefer color first

      //// @brief Add tooltip to cournet item
      // @param inColor - (ImVec4) decimal color vector (R,G,B,A)
      // @param inTip - (std::string) tip text.
      void mx_add_tooltip(ImVec4 inColor/* = missionx::color::color_vec4_white*/, const std::string& inTip) const;

      // Slider helper
      template <typename T>
      bool add_slider_float_helper(T& outRefParam, float min_f, float max_f, float inWidth_f = 400.0f)
      {
        float fSliderVal = (float)outRefParam;
        ImGui::PushItemWidth(inWidth_f);
        if (ImGui::DragFloat("SliderHelper", &fSliderVal, 1.0f, min_f, max_f, "%.0f px")) // debug window
        {
          outRefParam = fSliderVal;
        }
        ImGui::PopItemWidth();        

        return true;
      }

      ImVec2 vec2_plus(ImVec2 v1, ImVec2 v2) { return ImVec2(v1.x + v2.x, v1.y + v1.y); };
      ImVec2 vec2_minus(ImVec2 v1, ImVec2 v2) { return ImVec2(v1.x - v2.x, v1.y - v1.y); };
      ImVec2 vec2_multi(ImVec2 v1, ImVec2 v2) { return ImVec2(v1.x * v2.x, v1.y * v1.y); };
      ImVec2 vec2_multi_num(ImVec2 v1, float fVal) { return ImVec2(v1.x * fVal, v1.y * fVal); };


      ImVec4 mxConvertMxVec4ToImVec4(const missionx::mxVec4 & inMxVec4 ); // the function replaces a deprecated ImGui::GetWindowContentRegionWidth(). The new replacer functions does not seem to do the trick in some cases, so I have copied the logic of the original function into my own one in the hope it will continue and server me.
      float mxUiGetContentWidth(); // the function replaces a deprecated ImGui::GetWindowContentRegionWidth(). The new replacer functions does not seem to do the trick in some cases, so I have copied the logic of the original function into my own one in the hope it will continue and server me.
      float mxUiGetContentHeight(); // 
      ImVec2 mxUiGetWindowContentWxH(); // 

      //std::list<int> lstFontQueue{}; // holds the "imgui::pushFont" order. We could just keep it as a counter too.
      int            iFontQueue{ 0 };
      //std::string prevFontTypeName{ "" };

      // int prevFontPosition = 0;      
      //int prevFontIndex = 0;

      void mxUiSetDefaultFont();
      void mxUiResetAllFontsToDefault();
      
      void mxUiSetFont(const std::string& inTextType); // set the font using the font type name liked TEXT_TYPE_TITLE_REG, TEXT_TYPE_TITLE_BIG, TEXT_TYPE_TEXT_REG
      //void mxUiSetFontByPx(const int inFontPosition, const float inSizePx); // set the font using the size in px as key. example: mxUiSetFontByIndx(0, 16)
      void mxUiReleaseLastFont(const int inHowManyCycles = 1); // pop out Fonts we pushed, default is only the last one but you can release more than one. Updates iFontQueue.

      // In case of thread dependency, store the return value in a parameter and then send it when calling "mxEndUiDisableState()" function.
      // In case of non thread state dependence, it is safe to use the same boolean condition for "inBoolState"
      bool mxStartUiDisableState(const bool in_true_exp_to_disable); // v24.02.6 true means not disable
      void mxEndUiDisableState(const bool in_true_exp_to_disable);   // v24.02.6 true means not disable

      
      // void mxUiStartEvalDisableItems(const bool inFlag);
      // void mxUiEndEvalDisableItems(const bool inFlag);



      bool mxUiButtonTooltip(const char* label, const char* tip = nullptr, ImVec4 colFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4 colBg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), const ImVec2& size = ImVec2(0, 0));



      static std::map<int, ImGuiKey> mapXPtoImgui_key;
      static ImGuiKey                getKey(int inVirtualKey);
    private:
      //void                            initRemapKeys();

      #ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
      void initOldKeymap(ImGuiIO& io);
      #endif // !IMGUI_DISABLE_OBSOLETE_KEYIO

      void initRemapKeys2();

}; 

//namespace missionx
//{
//  namespace color
//  {
//    // Based on https://web.archive.org/web/20180301041827/https://prideout.net/archive/colors.php
//   inline const ImVec4 color_vec4_aliceblue            = { 0.941f, 0.973f, 1.000f, 1.0f };
//   inline const ImVec4 color_vec4_antiquewhite         = { 0.980f, 0.922f, 0.843f, 1.0f };
//   inline const ImVec4 color_vec4_aqua                 = { 0.000f, 1.000f, 1.000f, 1.0f };
//   inline const ImVec4 color_vec4_aquamarine           = { 0.498f, 1.000f, 0.831f, 1.0f };
//   inline const ImVec4 color_vec4_azure                = { 0.941f, 1.000f, 1.000f, 1.0f };
//   inline const ImVec4 color_vec4_beige                = { 0.961f, 0.961f, 0.863f, 1.0f };
//   inline const ImVec4 color_vec4_bisque               = { 1.000f, 0.894f, 0.769f, 1.0f };
//   inline const ImVec4 color_vec4_black                = { 0.000f, 0.000f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_blanchedalmond       = { 1.000f, 0.922f, 0.804f, 1.0f };
//   inline const ImVec4 color_vec4_blue                 = { 0.000f, 0.000f, 1.000f, 1.0f };
//   inline const ImVec4 color_vec4_blueviolet           = { 0.541f, 0.169f, 0.886f, 1.0f };
//   inline const ImVec4 color_vec4_brown                = { 0.647f, 0.165f, 0.165f, 1.0f };
//   inline const ImVec4 color_vec4_burlywood            = { 0.871f, 0.722f, 0.529f, 1.0f };
//   inline const ImVec4 color_vec4_cadetblue            = { 0.373f, 0.620f, 0.627f, 1.0f };
//   inline const ImVec4 color_vec4_chartreuse           = { 0.498f, 1.000f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_chocolate            = { 0.824f, 0.412f, 0.118f, 1.0f };
//   inline const ImVec4 color_vec4_coral                = { 1.000f, 0.498f, 0.314f, 1.0f };
//   inline const ImVec4 color_vec4_cornflowerblue       = { 0.392f, 0.584f, 0.929f, 1.0f };
//   inline const ImVec4 color_vec4_cornsilk             = { 1.000f, 0.973f, 0.863f, 1.0f };
//   inline const ImVec4 color_vec4_crimson              = { 0.863f, 0.078f, 0.235f, 1.0f };
//   inline const ImVec4 color_vec4_cyan                 = { 0.000f, 1.000f, 1.000f, 1.0f };
//   inline const ImVec4 color_vec4_darkblue             = { 0.000f, 0.000f, 0.545f, 1.0f };
//   inline const ImVec4 color_vec4_darkcyan             = { 0.000f, 0.545f, 0.545f, 1.0f };
//   inline const ImVec4 color_vec4_darkgoldenrod        = { 0.722f, 0.525f, 0.043f, 1.0f };
//   inline const ImVec4 color_vec4_darkgray             = { 0.663f, 0.663f, 0.663f, 1.0f };
//   inline const ImVec4 color_vec4_darkgreen            = { 0.000f, 0.392f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_darkgrey             = { 0.663f, 0.663f, 0.663f, 1.0f };
//   inline const ImVec4 color_vec4_darkkhaki            = { 0.741f, 0.718f, 0.420f, 1.0f };
//   inline const ImVec4 color_vec4_darkmagenta          = { 0.545f, 0.000f, 0.545f, 1.0f };
//   inline const ImVec4 color_vec4_darkolivegreen       = { 0.333f, 0.420f, 0.184f, 1.0f };
//   inline const ImVec4 color_vec4_darkorange           = { 1.000f, 0.549f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_darkorchid           = { 0.600f, 0.196f, 0.800f, 1.0f };
//   inline const ImVec4 color_vec4_darkred              = { 0.545f, 0.000f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_darksalmon           = { 0.914f, 0.588f, 0.478f, 1.0f };
//   inline const ImVec4 color_vec4_darkseagreen         = { 0.561f, 0.737f, 0.561f, 1.0f };
//   inline const ImVec4 color_vec4_darkslateblue        = { 0.282f, 0.239f, 0.545f, 1.0f };
//   inline const ImVec4 color_vec4_darkslategray        = { 0.184f, 0.310f, 0.310f, 1.0f };
//   inline const ImVec4 color_vec4_darkslategrey        = { 0.184f, 0.310f, 0.310f, 1.0f };
//   inline const ImVec4 color_vec4_darkturquoise        = { 0.000f, 0.808f, 0.820f, 1.0f };
//   inline const ImVec4 color_vec4_darkviolet           = { 0.580f, 0.000f, 0.827f, 1.0f };
//   inline const ImVec4 color_vec4_deeppink             = { 1.000f, 0.078f, 0.576f, 1.0f };
//   inline const ImVec4 color_vec4_deepskyblue          = { 0.000f, 0.749f, 1.000f, 1.0f };
//   inline const ImVec4 color_vec4_dimgray              = { 0.412f, 0.412f, 0.412f, 1.0f };
//   inline const ImVec4 color_vec4_dimgrey              = { 0.412f, 0.412f, 0.412f, 1.0f };
//   inline const ImVec4 color_vec4_dodgerblue           = { 0.118f, 0.565f, 1.000f, 1.0f };
//   inline const ImVec4 color_vec4_firebrick            = { 0.698f, 0.133f, 0.133f, 1.0f };
//   inline const ImVec4 color_vec4_floralwhite          = { 1.000f, 0.980f, 0.941f, 1.0f };
//   inline const ImVec4 color_vec4_forestgreen          = { 0.133f, 0.545f, 0.133f, 1.0f };
//   inline const ImVec4 color_vec4_fuchsia              = { 1.000f, 0.000f, 1.000f, 1.0f };
//   inline const ImVec4 color_vec4_gainsboro            = { 0.863f, 0.863f, 0.863f, 1.0f };
//   inline const ImVec4 color_vec4_ghostwhite           = { 0.973f, 0.973f, 1.000f, 1.0f };
//   inline const ImVec4 color_vec4_gold                 = { 1.000f, 0.843f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_goldenrod            = { 0.855f, 0.647f, 0.125f, 1.0f };
//   inline const ImVec4 color_vec4_gray                 = { 0.502f, 0.502f, 0.502f, 1.0f };
//   inline const ImVec4 color_vec4_green                = { 0.000f, 0.502f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_greenyellow          = { 0.678f, 1.000f, 0.184f, 1.0f };
//   inline const ImVec4 color_vec4_grey                 = { 0.502f, 0.502f, 0.502f, 1.0f };
//   inline const ImVec4 color_vec4_honeydew             = { 0.941f, 1.000f, 0.941f, 1.0f };
//   inline const ImVec4 color_vec4_hotpink              = { 1.000f, 0.412f, 0.706f, 1.0f };
//   inline const ImVec4 color_vec4_indianred            = { 0.804f, 0.361f, 0.361f, 1.0f };
//   inline const ImVec4 color_vec4_indigo               = { 0.294f, 0.000f, 0.510f, 1.0f };
//   inline const ImVec4 color_vec4_ivory                = { 1.000f, 1.000f, 0.941f, 1.0f };
//   inline const ImVec4 color_vec4_khaki                = { 0.941f, 0.902f, 0.549f, 1.0f };
//   inline const ImVec4 color_vec4_lavender             = { 0.902f, 0.902f, 0.980f, 1.0f };
//   inline const ImVec4 color_vec4_lavenderblush        = { 1.000f, 0.941f, 0.961f, 1.0f };
//   inline const ImVec4 color_vec4_lawngreen            = { 0.486f, 0.988f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_lemonchiffon         = { 1.000f, 0.980f, 0.804f, 1.0f };
//   inline const ImVec4 color_vec4_lightblue            = { 0.678f, 0.847f, 0.902f, 1.0f };
//   inline const ImVec4 color_vec4_lightcoral           = { 0.941f, 0.502f, 0.502f, 1.0f };
//   inline const ImVec4 color_vec4_lightcyan            = { 0.878f, 1.000f, 1.000f, 1.0f };
//   inline const ImVec4 color_vec4_lightgoldenrodyellow = { 0.980f, 0.980f, 0.824f, 1.0f };
//   inline const ImVec4 color_vec4_lightgray            = { 0.827f, 0.827f, 0.827f, 1.0f };
//   inline const ImVec4 color_vec4_lightgreen           = { 0.565f, 0.933f, 0.565f, 1.0f };
//   inline const ImVec4 color_vec4_lightgrey            = { 0.827f, 0.827f, 0.827f, 1.0f };
//   inline const ImVec4 color_vec4_lightpink            = { 1.000f, 0.714f, 0.757f, 1.0f };
//   inline const ImVec4 color_vec4_lightsalmon          = { 1.000f, 0.627f, 0.478f, 1.0f };
//   inline const ImVec4 color_vec4_lightseagreen        = { 0.125f, 0.698f, 0.667f, 1.0f };
//   inline const ImVec4 color_vec4_lightskyblue         = { 0.529f, 0.808f, 0.980f, 1.0f };
//   inline const ImVec4 color_vec4_lightslategray       = { 0.467f, 0.533f, 0.600f, 1.0f };
//   inline const ImVec4 color_vec4_lightslategrey       = { 0.467f, 0.533f, 0.600f, 1.0f };
//   inline const ImVec4 color_vec4_lightsteelblue       = { 0.690f, 0.769f, 0.871f, 1.0f };
//   inline const ImVec4 color_vec4_lightyellow          = { 1.000f, 1.000f, 0.878f, 1.0f };
//   inline const ImVec4 color_vec4_lime                 = { 0.000f, 1.000f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_limegreen            = { 0.196f, 0.804f, 0.196f, 1.0f };
//   inline const ImVec4 color_vec4_linen                = { 0.980f, 0.941f, 0.902f, 1.0f };
//   inline const ImVec4 color_vec4_magenta              = { 1.000f, 0.000f, 1.000f, 1.0f };
//   inline const ImVec4 color_vec4_maroon               = { 0.502f, 0.000f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_mediumaquamarine     = { 0.400f, 0.804f, 0.667f, 1.0f };
//   inline const ImVec4 color_vec4_mediumblue           = { 0.000f, 0.000f, 0.804f, 1.0f };
//   inline const ImVec4 color_vec4_mediumorchid         = { 0.729f, 0.333f, 0.827f, 1.0f };
//   inline const ImVec4 color_vec4_mediumpurple         = { 0.576f, 0.439f, 0.859f, 1.0f };
//   inline const ImVec4 color_vec4_mediumseagreen       = { 0.235f, 0.702f, 0.443f, 1.0f };
//   inline const ImVec4 color_vec4_mediumslateblue      = { 0.482f, 0.408f, 0.933f, 1.0f };
//   inline const ImVec4 color_vec4_mediumspringgreen    = { 0.000f, 0.980f, 0.604f, 1.0f };
//   inline const ImVec4 color_vec4_mediumturquoise      = { 0.282f, 0.820f, 0.800f, 1.0f };
//   inline const ImVec4 color_vec4_mediumvioletred      = { 0.780f, 0.082f, 0.522f, 1.0f };
//   inline const ImVec4 color_vec4_midnightblue         = { 0.098f, 0.098f, 0.439f, 1.0f };
//   inline const ImVec4 color_vec4_mintcream            = { 0.961f, 1.000f, 0.980f, 1.0f };
//   inline const ImVec4 color_vec4_mistyrose            = { 1.000f, 0.894f, 0.882f, 1.0f };
//   inline const ImVec4 color_vec4_moccasin             = { 1.000f, 0.894f, 0.710f, 1.0f };
//   inline const ImVec4 color_vec4_navajowhite          = { 1.000f, 0.871f, 0.678f, 1.0f };
//   inline const ImVec4 color_vec4_navy                 = { 0.000f, 0.000f, 0.502f, 1.0f };
//   inline const ImVec4 color_vec4_oldlace              = { 0.992f, 0.961f, 0.902f, 1.0f };
//   inline const ImVec4 color_vec4_olive                = { 0.502f, 0.502f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_olivedrab            = { 0.420f, 0.557f, 0.137f, 1.0f };
//   inline const ImVec4 color_vec4_orange               = { 1.000f, 0.647f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_orangered            = { 1.000f, 0.271f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_orchid               = { 0.855f, 0.439f, 0.839f, 1.0f };
//   inline const ImVec4 color_vec4_palegoldenrod        = { 0.933f, 0.910f, 0.667f, 1.0f };
//   inline const ImVec4 color_vec4_palegreen            = { 0.596f, 0.984f, 0.596f, 1.0f };
//   inline const ImVec4 color_vec4_paleturquoise        = { 0.686f, 0.933f, 0.933f, 1.0f };
//   inline const ImVec4 color_vec4_palevioletred        = { 0.859f, 0.439f, 0.576f, 1.0f };
//   inline const ImVec4 color_vec4_papayawhip           = { 1.000f, 0.937f, 0.835f, 1.0f };
//   inline const ImVec4 color_vec4_peachpuff            = { 1.000f, 0.855f, 0.725f, 1.0f };
//   inline const ImVec4 color_vec4_peru                 = { 0.804f, 0.522f, 0.247f, 1.0f };
//   inline const ImVec4 color_vec4_pink                 = { 1.000f, 0.753f, 0.796f, 1.0f };
//   inline const ImVec4 color_vec4_plum                 = { 0.867f, 0.627f, 0.867f, 1.0f };
//   inline const ImVec4 color_vec4_powderblue           = { 0.690f, 0.878f, 0.902f, 1.0f };
//   inline const ImVec4 color_vec4_purple               = { 0.502f, 0.000f, 0.502f, 1.0f };
//   inline const ImVec4 color_vec4_red                  = { 1.000f, 0.000f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_rosybrown            = { 0.737f, 0.561f, 0.561f, 1.0f };
//   inline const ImVec4 color_vec4_royalblue            = { 0.255f, 0.412f, 0.882f, 1.0f };
//   inline const ImVec4 color_vec4_saddlebrown          = { 0.545f, 0.271f, 0.075f, 1.0f };
//   inline const ImVec4 color_vec4_salmon               = { 0.980f, 0.502f, 0.447f, 1.0f };
//   inline const ImVec4 color_vec4_sandybrown           = { 0.957f, 0.643f, 0.376f, 1.0f };
//   inline const ImVec4 color_vec4_seagreen             = { 0.180f, 0.545f, 0.341f, 1.0f };
//   inline const ImVec4 color_vec4_seashell             = { 1.000f, 0.961f, 0.933f, 1.0f };
//   inline const ImVec4 color_vec4_sienna               = { 0.627f, 0.322f, 0.176f, 1.0f };
//   inline const ImVec4 color_vec4_silver               = { 0.753f, 0.753f, 0.753f, 1.0f };
//   inline const ImVec4 color_vec4_skyblue              = { 0.529f, 0.808f, 0.922f, 1.0f };
//   inline const ImVec4 color_vec4_slateblue            = { 0.416f, 0.353f, 0.804f, 1.0f };
//   inline const ImVec4 color_vec4_slategray            = { 0.439f, 0.502f, 0.565f, 1.0f };
//   inline const ImVec4 color_vec4_slategrey            = { 0.439f, 0.502f, 0.565f, 1.0f };
//   inline const ImVec4 color_vec4_snow                 = { 1.000f, 0.980f, 0.980f, 1.0f };
//   inline const ImVec4 color_vec4_springgreen          = { 0.000f, 1.000f, 0.498f, 1.0f };
//   inline const ImVec4 color_vec4_steelblue            = { 0.275f, 0.510f, 0.706f, 1.0f };
//   inline const ImVec4 color_vec4_tan                  = { 0.824f, 0.706f, 0.549f, 1.0f };
//   inline const ImVec4 color_vec4_teal                 = { 0.000f, 0.502f, 0.502f, 1.0f };
//   inline const ImVec4 color_vec4_thistle              = { 0.847f, 0.749f, 0.847f, 1.0f };
//   inline const ImVec4 color_vec4_tomato               = { 1.000f, 0.388f, 0.278f, 1.0f };
//   inline const ImVec4 color_vec4_turquoise            = { 0.251f, 0.878f, 0.816f, 1.0f };
//   inline const ImVec4 color_vec4_violet               = { 0.933f, 0.510f, 0.933f, 1.0f };
//   inline const ImVec4 color_vec4_wheat                = { 0.961f, 0.871f, 0.702f, 1.0f };
//   inline const ImVec4 color_vec4_white                = { 1.000f, 1.000f, 1.000f, 1.0f };
//   inline const ImVec4 color_vec4_whitesmoke           = { 0.961f, 0.961f, 0.961f, 1.0f };
//   inline const ImVec4 color_vec4_yellow               = { 1.000f, 1.000f, 0.000f, 1.0f };
//   inline const ImVec4 color_vec4_yellowgreen          = { 0.604f, 0.804f, 0.196f, 1.0f };
//
//    }
//}






#endif // #ifndef IMGWINDOW_H
