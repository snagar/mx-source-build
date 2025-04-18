/*
 * ImgWindow.cpp
 *
 * Integration for dear imgui into X-Plane.
 *
 * Copyright (C) 2018,2020, Christopher Collins
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

#include "ImgWindow.h"
#include <XPLMDataAccess.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>

// mission-X, saar
#include "../../../src/core/xx_mission_constants.hpp"

// size of "frame" around a resizable window, by which its size can be changed
constexpr int WND_RESIZE_LEFT_WIDTH   = 15;
constexpr int WND_RESIZE_TOP_WIDTH    = 5;
constexpr int WND_RESIZE_RIGHT_WIDTH  = 15;
constexpr int WND_RESIZE_BOTTOM_WIDTH = 15;

static XPLMDataRef gVrEnabledRef        = nullptr;
static XPLMDataRef gModelviewMatrixRef  = nullptr;
static XPLMDataRef gViewportRef         = nullptr;
static XPLMDataRef gProjectionMatrixRef = nullptr;
static XPLMDataRef gFrameRatePeriodRef  = nullptr;

std::shared_ptr<ImgFontAtlas> ImgWindow::sFont1;
int                           ImgWindow::defaultFontPos{ 0 }; // v3.303.14
std::map<int, ImGuiKey>       ImgWindow::mapXPtoImgui_key;    // v3.303.14



ImgWindow::ImgWindow(int left, int top, int right, int bottom, XPLMWindowDecoration decoration, XPLMWindowLayer layer)
  : mFirstRender(true)
  , mFontAtlas(sFont1)
  , mPreferredLayer(layer)
  , bHandleWndResize(xplm_WindowDecorationSelfDecoratedResizable == decoration)
{
  ImFontAtlas* iFontAtlas = nullptr;
  if (mFontAtlas)
  {
    mFontAtlas->bindTexture();
    iFontAtlas = mFontAtlas->getAtlas();
  }
  mImGuiContext  = ImGui::CreateContext(iFontAtlas);
  mImPlotContext = ImPlot::CreateContext(); // v3.0.255.1
  ImGui::SetCurrentContext(mImGuiContext);
  auto& io = ImGui::GetIO();

  this->initRemapKeys2(); // saar missionx

  //io.SetKeyEventNativeData(); // should we implement this ?

  static bool first_init = false;
  if (!first_init)
  {
    gVrEnabledRef        = XPLMFindDataRef("sim/graphics/VR/enabled");
    gModelviewMatrixRef  = XPLMFindDataRef("sim/graphics/view/modelview_matrix");
    gViewportRef         = XPLMFindDataRef("sim/graphics/view/viewport");
    gProjectionMatrixRef = XPLMFindDataRef("sim/graphics/view/projection_matrix");
    gFrameRatePeriodRef  = XPLMFindDataRef("sim/operation/misc/frame_rate_period");
    first_init           = true;
  }

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
  // we render ourselves, we don't use the DrawListsFunc
  io.RenderDrawListsFn = nullptr;
#endif

#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO

  // set up the Keymap
  initOldKeymap(io);

  //io.KeyMap[ImGuiKey_Tab]         = XPLM_VK_TAB;
  //io.KeyMap[ImGuiKey_LeftArrow]   = XPLM_VK_LEFT;
  //io.KeyMap[ImGuiKey_RightArrow]  = XPLM_VK_RIGHT;
  //io.KeyMap[ImGuiKey_UpArrow]     = XPLM_VK_UP;
  //io.KeyMap[ImGuiKey_DownArrow]   = XPLM_VK_DOWN;
  //io.KeyMap[ImGuiKey_PageUp]      = XPLM_VK_PRIOR;
  //io.KeyMap[ImGuiKey_PageDown]    = XPLM_VK_NEXT;
  //io.KeyMap[ImGuiKey_Home]        = XPLM_VK_HOME;
  //io.KeyMap[ImGuiKey_End]         = XPLM_VK_END;
  //io.KeyMap[ImGuiKey_Insert]      = XPLM_VK_INSERT;
  //io.KeyMap[ImGuiKey_Delete]      = XPLM_VK_DELETE;
  //io.KeyMap[ImGuiKey_Backspace]   = XPLM_VK_BACK;
  //io.KeyMap[ImGuiKey_Space]       = XPLM_VK_SPACE;
  //io.KeyMap[ImGuiKey_Enter]       = XPLM_VK_RETURN;
  //io.KeyMap[ImGuiKey_Escape]      = XPLM_VK_ESCAPE;
  //io.KeyMap[ImGuiKey_KeypadEnter] = XPLM_VK_ENTER; // was: ImGuiKey_KeyPadEnter  in imgui v1.86  // v1.87 and up: ImGuiKey_KeypadEnter
  //io.KeyMap[ImGuiKey_A]           = XPLM_VK_A;
  //io.KeyMap[ImGuiKey_C]           = XPLM_VK_C;
  //io.KeyMap[ImGuiKey_V]           = XPLM_VK_V;
  //io.KeyMap[ImGuiKey_X]           = XPLM_VK_X;
  //io.KeyMap[ImGuiKey_Y]           = XPLM_VK_Y;
  //io.KeyMap[ImGuiKey_Z]           = XPLM_VK_Z;

#endif
  // disable window rounding since we're not rendering the frame anyway.
  auto& style          = ImGui::GetStyle();
  style.WindowRounding = 0;

  // bind the font
  if (mFontAtlas)
  {
    mFontTexture = static_cast<GLuint>(reinterpret_cast<intptr_t>(io.Fonts->TexID));
  }
  else
  {
    if (!iFontAtlas || iFontAtlas->TexID == nullptr)
    {
      // fallback binding if an atlas wasn't explicitly set.
      unsigned char* pixels;
      int            width, height;
      io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

      // slightly stupid dance around the texture number due to XPLM not using GLint here.
      int texNum = 0;
      XPLMGenerateTextureNumbers(&texNum, 1);
      mFontTexture = (GLuint)texNum;

      // upload texture.
      XPLMBindTexture2d((int)mFontTexture, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
      io.Fonts->SetTexID((void*)((intptr_t)(mFontTexture)));
    }
  }

  // disable OSX-like keyboard behaviors always - we don't have the keymapping for it.
  io.ConfigMacOSXBehaviors = false;

  // try to inhibit a few resize/move behaviors that won't play nice with our window control.
  io.ConfigWindowsResizeFromEdges      = false;
  io.ConfigWindowsMoveFromTitleBarOnly = true;

  XPLMCreateWindow_t windowParams = {
    sizeof(windowParams), left, top, right, bottom, 0, DrawWindowCB, HandleMouseClickCB, HandleKeyFuncCB, HandleCursorFuncCB, HandleMouseWheelFuncCB, reinterpret_cast<void*>(this), decoration, layer, HandleRightClickFuncCB,
  };
  mWindowID = XPLMCreateWindowEx(&windowParams);
}

ImgWindow::~ImgWindow()
{
  ImGui::SetCurrentContext(mImGuiContext);
  if (!mFontAtlas)
  {
    // if we didn't have an explicit font atlas, destroy the texture.
    glDeleteTextures(1, &mFontTexture);
  }
  ImPlot::DestroyContext(mImPlotContext); // v3.0.255.1
  ImGui::DestroyContext(mImGuiContext);
  XPLMDestroyWindow(mWindowID);
}

void
ImgWindow::GetCurrentWindowGeometry(int& left, int& top, int& right, int& bottom) const
{
  if (IsPoppedOut())
    GetWindowGeometryOS(left, top, right, bottom);
  else if (IsInVR())
  {
    left = bottom = 0;
    GetWindowGeometryVR(right, top);
  }
  else
  {
    GetWindowGeometry(left, top, right, bottom);
  }
}

void
ImgWindow::SetWindowResizingLimits(int minW, int minH, int maxW, int maxH)
{
  minWidth  = minW;
  minHeight = minH;
  maxWidth  = maxW;
  maxHeight = maxH;
  XPLMSetWindowResizingLimits(mWindowID, minW, minH, maxW, maxH);
}

void
ImgWindow::updateMatrices()
{
  // Get the current modelview matrix, viewport, and projection matrix from X-Plane
  XPLMGetDatavf(gModelviewMatrixRef, mModelView, 0, 16);
  XPLMGetDatavf(gProjectionMatrixRef, mProjection, 0, 16);
  XPLMGetDatavi(gViewportRef, mViewport, 0, 4);
}

static void
multMatrixVec4f(GLfloat dst[4], const GLfloat m[16], const GLfloat v[4])
{
  dst[0] = v[0] * m[0] + v[1] * m[4] + v[2] * m[8] + v[3] * m[12];
  dst[1] = v[0] * m[1] + v[1] * m[5] + v[2] * m[9] + v[3] * m[13];
  dst[2] = v[0] * m[2] + v[1] * m[6] + v[2] * m[10] + v[3] * m[14];
  dst[3] = v[0] * m[3] + v[1] * m[7] + v[2] * m[11] + v[3] * m[15];
}

void
ImgWindow::boxelsToNative(int x, int y, int& outX, int& outY)
{
  GLfloat boxelPos[4] = { (GLfloat)x, (GLfloat)y, 0, 1 };
  GLfloat eye[4], ndc[4];

  multMatrixVec4f(eye, mModelView, boxelPos);
  multMatrixVec4f(ndc, mProjection, eye);
  ndc[3] = 1.0f / ndc[3];
  ndc[0] *= ndc[3];
  ndc[1] *= ndc[3];

  outX = static_cast<int>((ndc[0] * 0.5f + 0.5f) * mViewport[2] + mViewport[0]);
  outY = static_cast<int>((ndc[1] * 0.5f + 0.5f) * mViewport[3] + mViewport[1]);
}

/*
 * NB:  This is a modified version of the imGui OpenGL2 renderer - however, because
 *     we need to play nice with the X-Plane GL state management, we cannot use
 *     the upstream one.
 */

void
ImgWindow::RenderImGui(ImDrawData* draw_data)
{
  // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
  ImGuiIO& io = ImGui::GetIO();
  if ((io.DisplayFramebufferScale.x != 1.0) || (io.DisplayFramebufferScale.y != 1.0))
  {
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);
  }

  updateMatrices();

  // We are using the OpenGL fixed pipeline because messing with the
  // shader-state in X-Plane is not very well documented, but using the fixed
  // function pipeline is.

  // 1TU + Alpha settings, no depth, no fog.
  XPLMSetGraphicsState(0, 1, 0, 1, 1, 0, 0);
  glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
  glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
  glDisable(GL_CULL_FACE);
  glEnable(GL_SCISSOR_TEST);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnable(GL_TEXTURE_2D);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glScalef(1.0f, -1.0f, 1.0f);
  glTranslatef(static_cast<GLfloat>(mLeft), static_cast<GLfloat>(-mTop), 0.0f);

  // Render command lists
  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list   = draw_data->CmdLists[n];
    const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
    const ImDrawIdx*  idx_buffer = cmd_list->IdxBuffer.Data;
    glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + offsetof(ImDrawVert, pos)));
    glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + offsetof(ImDrawVert, uv)));
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + offsetof(ImDrawVert, col)));

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
    {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if (pcmd->UserCallback)
      {
        pcmd->UserCallback(cmd_list, pcmd);
      }
      else
      {
        XPLMBindTexture2d((int)(intptr_t)pcmd->TextureId, 0);

        // Scissors work in viewport space - must translate the coordinates from ImGui -> Boxels, then Boxels -> Native.
        // FIXME: it must be possible to apply the scale+transform manually to the projection matrix so we don't need to doublestep.
        int bTop, bLeft, bRight, bBottom;
        translateImguiToBoxel(pcmd->ClipRect.x, pcmd->ClipRect.y, bLeft, bTop);
        translateImguiToBoxel(pcmd->ClipRect.z, pcmd->ClipRect.w, bRight, bBottom);
        int nTop, nLeft, nRight, nBottom;
        boxelsToNative(bLeft, bTop, nLeft, nTop);
        boxelsToNative(bRight, bBottom, nRight, nBottom);
        glScissor(nLeft, nBottom, nRight - nLeft, nTop - nBottom);
        glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
      }
      idx_buffer += pcmd->ElemCount;
    }
  }

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  // Restore modified state
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glPopAttrib();
  glPopClientAttrib();
}

void
ImgWindow::translateToImguiSpace(int inX, int inY, float& outX, float& outY)
{
  outX = static_cast<float>(inX - mLeft);
  if (outX < 0.0f || outX > (float)(mRight - mLeft))
  {
    outX = -FLT_MAX;
    outY = -FLT_MAX;
    return;
  }
  outY = static_cast<float>(mTop - inY);
  if (outY < 0.0f || outY > (float)(mTop - mBottom))
  {
    outX = -FLT_MAX;
    outY = -FLT_MAX;
    return;
  }
}


void
ImgWindow::translateImguiToBoxel(float inX, float inY, int& outX, int& outY)
{
  outX = (int)(mLeft + inX);
  outY = (int)(mTop - inY);
}


void
ImgWindow::updateImgui()
{
  ImGui::SetCurrentContext(mImGuiContext);
  auto& io = ImGui::GetIO();

  // transfer the window geometry to ImGui
  XPLMGetWindowGeometry(mWindowID, &mLeft, &mTop, &mRight, &mBottom);

  float win_width  = static_cast<float>(mRight - mLeft);
  float win_height = static_cast<float>(mTop - mBottom);

  // Needed to add this to prevent io.DeltaTime causing a CTD because when X-Plane starts FrameRatePeriod is equal to 0.0f
  float FrameRatePeriod = XPLMGetDataf(gFrameRatePeriodRef);
  if (FrameRatePeriod > 0.0f)
  {
    io.DeltaTime = XPLMGetDataf(gFrameRatePeriodRef);
  }
  io.DisplaySize = ImVec2(win_width, win_height);
  // in boxels, we're always scale 1, 1.
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
  ImGui::NewFrame();

  ImGui::SetNextWindowPos(ImVec2((float)0.0, (float)0.0), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(win_width, win_height), ImGuiCond_Always);

  // and construct the window
  ImGui::Begin(mWindowTitle.c_str(), nullptr, beforeBegin() | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
  buildInterface();
  ImGui::End();


  ImGui::EndFrame(); // missionx saar based on:  IM_ASSERT(g.IO.KeyMods == expected_key_mod_flags && "Mismatching io.KeyCtrl/io.KeyShift/io.KeyAlt/io.KeySuper vs io.KeyMods"); imgui.cpp

  // finally, handle window focus.
  int hasKeyboardFocus = XPLMHasKeyboardFocus(mWindowID);
  if (io.WantTextInput && !hasKeyboardFocus)
  {
    XPLMTakeKeyboardFocus(mWindowID);
  }
  else if (!io.WantTextInput && hasKeyboardFocus)
  {
    XPLMTakeKeyboardFocus(nullptr);
    // reset keysdown otherwise we'll think any keys used to defocus the keyboard are still down!
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    // reset keysdown otherwise we'll think any keys used to defocus the keyboard are still down!
    for (auto& key : io.KeysDown)
    {
      key = false;
    }
#else
    // for (auto& [xpKey, imgKey] : ImgWindow::mapXPtoImgui_key)
    for (auto& [xpKey, imgKey] : ImgWindow::mapXPtoImgui_key)
      io.AddKeyEvent(imgKey, false);
#endif
  }
  mFirstRender = false;
}

void
ImgWindow::DrawWindowCB(XPLMWindowID /* inWindowID */, void* inRefcon)
{
  ImGuiIO& io         = ImGui::GetIO();

  auto* thisWindow = reinterpret_cast<ImgWindow*>(inRefcon);

  thisWindow->updateImgui();

  ImGui::SetCurrentContext(thisWindow->mImGuiContext);
  ImGui::Render();

  thisWindow->RenderImGui(ImGui::GetDrawData());

  // Give subclasses a chance to do something after all rendering
  thisWindow->afterRendering();

  // Hack: Reset the Backspace key if in VR (see HandleKeyFuncCB for details)  
  if (thisWindow->bResetBackspace)
  {    
    #ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
      io.KeysDown[XPLM_VK_BACK] = false;
    #else
      io.AddKeyEvent(ImGuiKey_Backspace, false);
    #endif
    thisWindow->bResetBackspace = false;
  }

  #ifdef IMGUI_DISABLE_OBSOLETE_KEYIO
    io.KeyMods = 0; // reset keyMods state, part of workaround to enable copy+paste using the latest ImGui library build // https://github.com/ocornut/imgui/issues/4921
  #endif // IMGUI_DISABLE_OBSOLETE_KEYIO

}

int
ImgWindow::HandleMouseClickCB(XPLMWindowID /* inWindowID */, int x, int y, XPLMMouseStatus inMouse, void* inRefcon)
{
  auto* thisWindow = reinterpret_cast<ImgWindow*>(inRefcon);
  return thisWindow->HandleMouseClickGeneric(x, y, inMouse, 0);
}

int
ImgWindow::HandleMouseClickGeneric(int x, int y, XPLMMouseStatus inMouse, int button)
{
  ImGui::SetCurrentContext(mImGuiContext);
  ImGuiIO& io = ImGui::GetIO();

  // Tell ImGui the mous position relative to the window
  translateToImguiSpace(x, y, io.MousePos.x, io.MousePos.y);
  const int loc_x = int(io.MousePos.x); // local x, relative to top/left corner
  const int loc_y = int(io.MousePos.y);
  const int dx    = x - lastMouseDragX; // dragged how far since last down/drag event?
  const int dy    = y - lastMouseDragY;

  switch (inMouse)
  {

    case xplm_MouseDrag:
      io.MouseDown[button] = true;

      // Any kind of self-dragging/resizing only happens with a floating window in the sim
      if (button == 0 &&   // left button
          IsInsideSim() && // floating window in sim
          dragWhat &&      // and if there actually _is_ dragging
          (dx != 0 || dy != 0))
      {
        // shall we drag the entire window?
        if (dragWhat.wnd)
        {
          mLeft += dx; // move the wdinow
          mRight += dx;
          mTop += dy;
          mBottom += dy;
        }
        else
        {
          // do we need to handle window resize?
          if (dragWhat.left)
            mLeft += dx;
          if (dragWhat.top)
            mTop += dy;
          if (dragWhat.right)
            mRight += dx;
          if (dragWhat.bottom)
            mBottom += dy;

          // Make sure resizing limits are honored
          if (mRight - mLeft < minWidth)
          {
            if (dragWhat.left)
              mLeft = mRight - minWidth;
            else
              mRight = mLeft + minWidth;
          }
          if (mRight - mLeft > maxWidth)
          {
            if (dragWhat.left)
              mLeft = mRight - maxWidth;
            else
              mRight = mLeft + maxWidth;
          }
          if (mTop - mBottom < minHeight)
          {
            if (dragWhat.top)
              mTop = mBottom + minHeight;
            else
              mBottom = mTop - minHeight;
          }
          if (mTop - mBottom > maxHeight)
          {
            if (dragWhat.top)
              mTop = mBottom + maxHeight;
            else
              mBottom = mTop - maxHeight;
          }
          // FIXME: If we had to apply resizing restricitons, then mouse and window frame will now be out of synch
        }

        // Change window geometry
        SetWindowGeometry(mLeft, mTop, mRight, mBottom);
        // now that the window has moved under the mouse we need to update relative mouse pos
        translateToImguiSpace(x, y, io.MousePos.x, io.MousePos.y);
        // Update the last handled position
        lastMouseDragX = x;
        lastMouseDragY = y;
      }
      break;

    case xplm_MouseDown:
      io.MouseDown[button] = true;

      // Which part of the window would we drag, if any?
      dragWhat.clear();
      if (button == 0 &&            // left button
          IsInsideSim() &&          // floating window in simulator
          loc_x >= 0 && loc_y >= 0) // valid local position
      {
        // shall we drag the entire window?
        if (IsInsideWindowDragArea(loc_x, loc_y))
        {
          dragWhat.wnd = true;
        }
        // do we need to handle window resize?
        else if (bHandleWndResize)
        {
          dragWhat.left   = loc_x <= WND_RESIZE_LEFT_WIDTH;
          dragWhat.top    = loc_y <= WND_RESIZE_TOP_WIDTH;
          dragWhat.right  = loc_x >= (mRight - mLeft) - WND_RESIZE_RIGHT_WIDTH;
          dragWhat.bottom = loc_y >= (mTop - mBottom) - WND_RESIZE_BOTTOM_WIDTH;
        }
        // Anything to drag?
        if (dragWhat)
        {
          // Remember pos in case of dragging
          lastMouseDragX = x;
          lastMouseDragY = y;
        }
      }
      break;

    case xplm_MouseUp:
      io.MouseDown[button] = false;
      lastMouseDragX = lastMouseDragY = -1;
      dragWhat.clear();
      break;
    default:
      // dunno!
      break;
  }

  return 1;
}


void
ImgWindow::HandleKeyFuncCB(XPLMWindowID /*inWindowID*/, char inKey, XPLMKeyFlags inFlags, char inVirtualKey, void* inRefcon, int blosingFocus)
{
  auto* thisWindow = reinterpret_cast<ImgWindow*>(inRefcon);
  ImGui::SetCurrentContext(thisWindow->mImGuiContext);
  ImGuiIO& io = ImGui::GetIO();
  //io.KeyMods  = 0;  // moved to DrawWindowCB()
  if (io.WantCaptureKeyboard)
  {

    // Loosing focus? That's not exactly something ImGui allows us to do...
    // we try convincing ImGui to let it go by sending an [Esc] key
    if (blosingFocus)
    {
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
      io.KeysDown[int(XPLM_VK_ESCAPE)] = true;
#else
      io.AddKeyEvent(ImGuiKey_Escape, true);
#endif
    }
    else
    {
      // Hack for the Backspace key in VR:
      // Apparently, the virtual VR keyboard sends both the Up and the Down
      // event within the same drawing cycle, which would overwrite
      // io.KeyDown[XPLM_VK_BACK] with false again before we could pass on true.
      // Also see https://forums.x-plane.org/index.php?/forums/topic/147139-dear-imgui-x-plane/&do=findComment&comment=2032062
      // though I am following a different solution:
      // So we ignore the "up" event (release key) here, and do the actual
      // release only after the next drawing cycle (flag bResetBackspace).
      // (And this little delay doesn't hurt in non-VR either, so we don't even test for VR.)

      // If Backspace is _released_ ...
      ImGuiKey translatedKey; // new saar missionx



      const auto lmbda_debug_print = [&]() {
        #ifdef DEBUG_KEYBOARD
          char buff[255] = { '\0' };
          sprintf(buff, "inKey: %c, inFlags: %i, inVirtualKey(char): %c, inVirtualKey(int): %i, translatedKey: %i\n", inKey, inFlags, inVirtualKey, inVirtualKey, (int)translatedKey);
          XPLMDebugString(buff);
        #endif // !DEBUG_KEYBOARD
      };


      if (inVirtualKey == XPLM_VK_BACK && !(inFlags & xplm_DownFlag))
      {
        #ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
          thisWindow->bResetBackspace = true; // have it reset only later in DrawWindowCB
        #else
          translatedKey = getKey((int)inVirtualKey);

          #ifndef RELEASE
            lmbda_debug_print(); // debug
          #endif

          io.AddKeyEvent(translatedKey, true);
          io.AddKeyEvent(translatedKey, false); // release immediately
          inKey = '\0';
        #endif // IMGUI_DISABLE_OBSOLETE_KEYIO
      }
      else
      {
        // in all normal cases: save the up/down flag as it comes from XP
       #ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
        translatedKey                  = getKey((int)inVirtualKey);

        #ifndef RELEASE
          lmbda_debug_print(); // debug
        #endif

        if(inVirtualKey > 0)
          io.KeysDown[int(inVirtualKey)] = (inFlags & xplm_DownFlag) == xplm_DownFlag;


       #else  
        translatedKey = getKey((int)inVirtualKey);

        #ifndef RELEASE
          lmbda_debug_print(); // debug
        #endif


         #ifndef IBM
          // ASCII Non printable characters are lower than 32. https://www.ascii-code.com/
          // Workaround to fix specific non printable keys from adding a char while the inKey should have been null "\0".
          if (int(char(translatedKey)) < 32 && (translatedKey == ImGuiKey_Home || translatedKey == ImGuiKey_End || translatedKey == ImGuiKey_PageUp || translatedKey == ImGuiKey_PageDown))
          {
            inKey = '\0'; // reset inKey values for Linux since cases like [home][end] produce the "P/W" chars too.
          }
          #ifndef RELEASE
            lmbda_debug_print(); // debug
          #endif

         #endif

          io.AddKeyEvent(translatedKey, ((inFlags & xplm_DownFlag) == xplm_DownFlag));
       #endif

      }

      io.KeyShift = (inFlags & xplm_ShiftFlag) == xplm_ShiftFlag;
      io.KeyAlt   = (inFlags & xplm_OptionAltFlag) == xplm_OptionAltFlag;
      io.KeyCtrl  = (inFlags & xplm_ControlFlag) == xplm_ControlFlag;

      // inKey will only includes printable characters,
      // but also those created with key combinations like @ or {}
      // [CTRL+C] ==> inKey: , inFlags: 12, inVirtualKey(char): C, inVirtualKey(int): 67, translatedKey: 548

      if ((inFlags & xplm_DownFlag) == xplm_DownFlag && inKey > '\0')
      {
        char smallStr[2] = { inKey, 0 };
        io.AddInputCharactersUTF8(smallStr);
        
        #ifdef IMGUI_DISABLE_OBSOLETE_KEYIO

          // This is a hack to MANUALLY set the io.KeyMods so later on IM will not use GetMergedModsFromKeys(), which is another hack I added in the imgui.cpp file
          if (io.KeyCtrl)
              io.KeyMods |= ImGuiMod_Ctrl;
          if (io.KeyShift)
              io.KeyMods |= ImGuiMod_Shift;
          if (io.KeyAlt)
              io.KeyMods |= ImGuiMod_Alt;

        #endif

        ////////////////////// DEBUG //////////////////////
#ifndef RELEASE
        if (io.KeyCtrl || io.KeyShift || io.KeyAlt)
        {
          // GetClipboardText  / if (io.SetClipboardTextFn)
          const std::string mod_s     = (io.KeyCtrl) ? "CONTROL" : (io.KeyShift) ? "SHIFT" : "ALT";
          const std::string upDown_s  = ((inFlags & xplm_DownFlag) == xplm_DownFlag) ? "Down" : "Release";
          char              buff[255] = { '\0' };
          
#ifdef IBM
          sprintf(buff, "Added %s %s key modifier with key: %c, translated value: %i\n", mod_s.c_str(), upDown_s.c_str(), inKey, (int)translatedKey);
#else
          snprintf(buff, sizeof(buff)-1, "Added %s %s key modifier with key: %c, translated value: %i\n", mod_s.c_str(), upDown_s.c_str(), inKey, (int)translatedKey);
#endif
          XPLMDebugString(buff);
        }
#endif // !RELEASE
      }
    }
  }
}


XPLMCursorStatus
ImgWindow::HandleCursorFuncCB(XPLMWindowID /*inWindowID*/, int x, int y, void* inRefcon)
{
  auto* thisWindow = reinterpret_cast<ImgWindow*>(inRefcon);
  ImGui::SetCurrentContext(thisWindow->mImGuiContext);
  ImGuiIO& io = ImGui::GetIO();
  float    outX, outY;
  thisWindow->translateToImguiSpace(x, y, outX, outY);
  io.MousePos = ImVec2(outX, outY);
  // FIXME: Maybe we can support imgui's cursors a bit better?
  return xplm_CursorDefault;
}


int
ImgWindow::HandleMouseWheelFuncCB(XPLMWindowID /*inWindowID*/, int x, int y, int wheel, int clicks, void* inRefcon)
{
  auto* thisWindow = reinterpret_cast<ImgWindow*>(inRefcon);
  ImGui::SetCurrentContext(thisWindow->mImGuiContext);
  ImGuiIO& io = ImGui::GetIO();

  float outX, outY;
  thisWindow->translateToImguiSpace(x, y, outX, outY);
  io.MousePos = ImVec2(outX, outY);
  switch (wheel)
  {
    case 0:
      io.MouseWheel += static_cast<float>(clicks);
      break;
    case 1:
      io.MouseWheelH += static_cast<float>(clicks);
      break;
    default:
      // unknown wheel
      break;
  }
  return 1;
}

int
ImgWindow::HandleRightClickFuncCB(XPLMWindowID /* inWindowID */, int x, int y, XPLMMouseStatus inMouse, void* inRefcon)
{
  auto* thisWindow = reinterpret_cast<ImgWindow*>(inRefcon);
  return thisWindow->HandleMouseClickGeneric(x, y, inMouse, 1);
}


void
ImgWindow::SetWindowTitle(const std::string& title)
{
  mWindowTitle = title;
  XPLMSetWindowTitle(mWindowID, mWindowTitle.c_str());
}

void
ImgWindow::SetVisible(bool inIsVisible)
{
  if (inIsVisible)
    moveForVR();
  if (GetVisible() == inIsVisible)
  {
    // if the state is already correct, no-op.
    return;
  }
  if (inIsVisible)
  {
    if (!onShow())
    {
      // chance to early abort.
      return;
    }
  }
  XPLMSetWindowIsVisible(mWindowID, inIsVisible);
}

void
ImgWindow::moveForVR()
{
  // if we're trying to display the window, check the state of the VR flag
  // - if we're VR enabled, explicitly move the window to the VR world.
  if (XPLMGetDatai(gVrEnabledRef))
  {
    XPLMSetWindowPositioningMode(mWindowID, xplm_WindowVR, 0);
  }
  else
  {
    if (IsInVR())
    {
      XPLMSetWindowPositioningMode(mWindowID, mPreferredLayer, -1);
    }
  }
}

bool
ImgWindow::GetVisible() const
{
  return XPLMGetWindowIsVisible(mWindowID) != 0;
}


bool
ImgWindow::onShow()
{
  return true;
}

void
ImgWindow::SetWindowDragArea(int left, int top, int right, int bottom)
{
  dragLeft   = left;
  dragTop    = top;
  dragRight  = right;
  dragBottom = bottom;
}

void
ImgWindow::ClearWindowDragArea()
{
  dragLeft = dragTop = dragRight = dragBottom = -1;
}

bool
ImgWindow::HasWindowDragArea(int* pL, int* pT, int* pR, int* pB) const
{
  // return definition if requested
  if (pL)
    *pL = dragLeft;
  if (pT)
    *pT = dragTop;
  if (pR)
    *pR = dragRight;
  if (pB)
    *pB = dragBottom;

  // is a valid drag area defined?
  return dragLeft >= 0 && dragTop >= 0 && dragRight > dragLeft && dragBottom >= dragTop;
}

bool
ImgWindow::IsInsideWindowDragArea(int x, int y) const
{
  // values outside the window aren't valid
  if (x == -FLT_MAX || y == -FLT_MAX)
    return false;

  // is a drag area defined in the first place?
  if (!HasWindowDragArea())
    return false;

  // inside the defined drag area?
  return dragLeft <= x && x <= dragRight && dragTop <= y && y <= dragBottom;
}

void
ImgWindow::SafeDelete()
{
  sPendingDestruction.push(this);
  if (sSelfDestructHandler == nullptr)
  {
    XPLMCreateFlightLoop_t flParams{
      sizeof(flParams),
      xplm_FlightLoop_Phase_BeforeFlightModel,
      &ImgWindow::SelfDestructCallback,
      nullptr,
    };
    sSelfDestructHandler = XPLMCreateFlightLoop(&flParams);
  }
  XPLMScheduleFlightLoop(sSelfDestructHandler, -1, 1);
}

std::queue<ImgWindow*> ImgWindow::sPendingDestruction;
XPLMFlightLoopID       ImgWindow::sSelfDestructHandler = nullptr;

float
ImgWindow::SelfDestructCallback(float /*inElapsedSinceLastCall*/, float /*inElapsedTimeSinceLastFlightLoop*/, int /*inCounter*/, void* /*inRefcon*/)
{
  while (!sPendingDestruction.empty())
  {
    auto* thisObj = sPendingDestruction.front();
    sPendingDestruction.pop();
    delete thisObj;
  }
  return 0;
}

///// Saar: Mission-X v3.0.251.1
void
ImgWindow::toggleWindowState()
{
  if (this->GetVisible())
    this->SetVisible(false); // hide
  else
    this->SetVisible(true); // show
}


void
ImgWindow::PushID_formatted(const char* format, ...)
{
  // format the variable string
  va_list args;
  char    sz[500];
  va_start(args, format);
  vsnprintf(sz, sizeof(sz), format, args);
  va_end(args);
  // Call the actual push function
  ImGui::PushID(sz);
}


bool
ImgWindow::ButtonTooltip(const char* label, const char* tip, ImU32 colFg, ImU32 colBg, const ImVec2& size)
{
  // Setup colors
  if (colFg != IM_COL32(1, 1, 1, 0))
    ImGui::PushStyleColor(ImGuiCol_Text, colFg);
  if (colBg != IM_COL32(1, 1, 1, 0))
    ImGui::PushStyleColor(ImGuiCol_Button, colBg);

  // do the button
  bool b = ImGui::Button(label, size);

  // restore previous colors
  if (colBg != IM_COL32(1, 1, 1, 0))
    ImGui::PopStyleColor();
  if (colFg != IM_COL32(1, 1, 1, 0))
    ImGui::PopStyleColor();

  // do the tooltip
  if (tip && ImGui::IsItemHovered())
    ImGui::SetTooltip("%s", tip);

  // return if button pressed
  return b;
}


ImVec4
ImgWindow::getColorAsImVec4(const std::string& inColor_s)
{
  if ( missionx::mxconst::get_YELLOW() == inColor_s)
    return (ImVec4(1.0f, 1.0f, 0.0f, 1.0));
  else if (missionx::mxconst::get_WHITE() == inColor_s)
    return (ImVec4(1.0f, 1.0f, 1.0f, 1.0));
  else if (missionx::mxconst::get_GREEN() == inColor_s)
    return (ImVec4(0.0f, 1.0f, 0.0f, 1.0));
  else if (missionx::mxconst::get_RED() == inColor_s)
    return (ImVec4(1.0f, 0.0f, 0.0f, 1.0));
  else if (missionx::mxconst::get_BLUE() == inColor_s)
    return (ImVec4(0.0f, 0.0f, 1.0f, 1.0));
  else if (missionx::mxconst::get_ORANGE() == inColor_s)
    return (ImVec4(1.0f, 0.5f, 0.0f, 1.0));
  else if (missionx::mxconst::get_PURPLE() == inColor_s)
    return (ImVec4(1.0f, 0.0f, 1.0f, 1.0));
  else if (missionx::mxconst::get_BLACK() == inColor_s)
    return (ImVec4(0.0f, 0.0f, 0.0f, 1.0));

  return ImVec4(1.0f, 1.0f, 1.0f, 1.0); // white
}


void
ImgWindow::HelpMarker(const char* desc, ImVec4 inTextColor)
{

  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered())
  {
    ImGui::PushStyleColor(ImGuiCol_Text, inTextColor);

    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();

    ImGui::PopStyleColor();
  }
}


void
ImgWindow::mx_add_tooltip(ImVec4 inColor, const std::string& inTip) const
{

  if (!IsInVR() && ImGui::IsItemHovered())
  {
    ImGui::PushStyleColor(ImGuiCol_Text, inColor); // dark gray

    ImGui::BeginTooltip();
    // ImGui::PushItemWidth(200.0f);
    ImGui::Text("%s", inTip.c_str());
    // ImGui::PopItemWidth();
    ImGui::EndTooltip();

    ImGui::PopStyleColor(1);
  }
}

ImVec4
ImgWindow::mxConvertMxVec4ToImVec4(const missionx::mxVec4& inMxVec4)
{
  return ImVec4(inMxVec4.x, inMxVec4.y, inMxVec4.z, inMxVec4.w);
}


float
ImgWindow::mxUiGetContentWidth()
{
  ImGuiWindow* window = GImGui->CurrentWindow;
  return window->ContentRegionRect.GetWidth();
}

float
ImgWindow::mxUiGetContentHeight()
{
  ImGuiWindow* window = GImGui->CurrentWindow;
  return window->ContentRegionRect.GetHeight();
}

ImVec2
ImgWindow::mxUiGetWindowContentWxH()
{
  return ImVec2(mxUiGetContentWidth(), mxUiGetContentHeight());
}

void
ImgWindow::mxUiSetDefaultFont()
{
  this->mxUiResetAllFontsToDefault();
}

void
ImgWindow::mxUiResetAllFontsToDefault()
{
  while (this->iFontQueue--)
    ImGui::PopFont();

  this->iFontQueue = 0;  

  ImGui::SetWindowFontScale (missionx::mxconst::DEFAULT_BASE_FONT_SCALE);
}



void
ImgWindow::mxUiSetFont(const std::string& inTextType)
{
  assert(!missionx::MxUICore::mapFontsMeta.empty() && "No fonts were loaded.");

  int fontID = 0;

  // Eval if font type exists or fallback to default_font
  // if (missionx::MxUICore::mapFontTypeToFontID.find(inTextType) != missionx::MxUICore::mapFontTypeToFontID.end())
  if (missionx::MxUICore::mapFontTypeToFontID.contains(inTextType) )
  {
    fontID = missionx::MxUICore::mapFontTypeToFontID[inTextType];

    missionx::MxUICore::mapFontTypesBeingUsedInProgram[inTextType] = fontID;

  }
  ImGui::PushFont(ImgWindow::sFont1->getAtlas()->Fonts[fontID]);

  ++this->iFontQueue;
}



void
ImgWindow::mxUiReleaseLastFont(const int inHowManyCycles)
{
  for (int loop1 = 0; loop1 < inHowManyCycles; ++loop1)
  {
    if (this->iFontQueue > 0)
    {
      ImGui::PopFont();
      this->iFontQueue--;
    }
  }
}


bool
ImgWindow::mxStartUiDisableState(const bool in_true_exp_to_disable)
{
  if (!in_true_exp_to_disable) // if expresion is false, skip.
    return in_true_exp_to_disable;

  ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

  return in_true_exp_to_disable;
}

void
ImgWindow::mxEndUiDisableState(const bool in_true_exp_to_disable)
{
  if (! in_true_exp_to_disable)
    return;

  ImGui::PopItemFlag();
  ImGui::PopStyleVar();
}

// void
// ImgWindow::mxUiStartEvalDisableItems(const bool inFlag)
// {
//   if (inFlag) // disable line
//   {
//     ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
//     ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
//   }
// }
//
// void
// ImgWindow::mxUiEndEvalDisableItems(const bool inFlag)
// {
//   if (inFlag) // disable line
//   {
//     ImGui::PopItemFlag();
//     ImGui::PopStyleVar();
//   }
// }

bool
ImgWindow::mxUiButtonTooltip(const char* label, const char* tip, ImVec4 colFg, ImVec4 colBg, const ImVec2& size)
{
  // Setup colors
    ImGui::PushStyleColor(ImGuiCol_Text, colFg);
    ImGui::PushStyleColor(ImGuiCol_Button, colBg);

  // do the button
  bool b = ImGui::Button(label, size);

  // restore previous colors
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

  // do the tooltip
  if (tip && ImGui::IsItemHovered())
    ImGui::SetTooltip("%s", tip);

  // return if button pressed
  return b;
}

void
ImgWindow::mxUiHelpMarker(ImVec4 inTextColor, const char* desc)
{
  ImgWindow::HelpMarker(desc, inTextColor);
}



ImGuiKey
ImgWindow::getKey(int inVirtualKey)
{
  if (ImgWindow::mapXPtoImgui_key.find(inVirtualKey) != ImgWindow::mapXPtoImgui_key.end())
    return ImgWindow::mapXPtoImgui_key[inVirtualKey];

  return ImGuiKey_None;
}


 #ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
void
ImgWindow::initOldKeymap(ImGuiIO& io)
{
  // set up the Keymap
  io.KeyMap[ImGuiKey_Tab]            = XPLM_KEY_TAB;
  io.KeyMap[ImGuiKey_LeftArrow]      = XPLM_KEY_LEFT;
  io.KeyMap[ImGuiKey_RightArrow]     = XPLM_KEY_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow]        = XPLM_KEY_UP;
  io.KeyMap[ImGuiKey_DownArrow]      = XPLM_KEY_DOWN;
  io.KeyMap[ImGuiKey_PageUp]         = XPLM_VK_PRIOR;
  io.KeyMap[ImGuiKey_PageDown]       = XPLM_VK_NEXT;
  io.KeyMap[ImGuiKey_Home]           = XPLM_VK_HOME;
  io.KeyMap[ImGuiKey_End]            = XPLM_VK_END;
  //io.KeyMap[ImGuiKey_Insert]         = XPLM_VK_;
  io.KeyMap[ImGuiKey_Delete]         = XPLM_KEY_DELETE;
  io.KeyMap[ImGuiKey_Backspace]      = XPLM_VK_BACK;
  io.KeyMap[ImGuiKey_Space]          = XPLM_VK_SPACE;
  io.KeyMap[ImGuiKey_Enter]          = XPLM_VK_RETURN;
  io.KeyMap[ImGuiKey_Escape]         = XPLM_KEY_ESCAPE;
  //io.KeyMap[ImGuiKey_LeftCtrl]       = XPLM_VK_;
  //io.KeyMap[ImGuiKey_LeftShift]      = XPLM_VK_;
  //io.KeyMap[ImGuiKey_LeftAlt]        = XPLM_VK_;
  //io.KeyMap[ImGuiKey_LeftSuper]      = XPLM_VK_;
  //io.KeyMap[ImGuiKey_RightCtrl]      = XPLM_VK_;
  //io.KeyMap[ImGuiKey_RightShift]     = XPLM_VK_;
  //io.KeyMap[ImGuiKey_RightAlt]       = XPLM_VK_;
  //io.KeyMap[ImGuiKey_RightSuper]     = XPLM_VK_;
  //io.KeyMap[ImGuiKey_Menu]           = XPLM_VK_;
  io.KeyMap[ImGuiKey_0]              = XPLM_KEY_0;
  io.KeyMap[ImGuiKey_1]              = XPLM_KEY_1;
  io.KeyMap[ImGuiKey_2]              = XPLM_KEY_2;
  io.KeyMap[ImGuiKey_3]              = XPLM_KEY_3;
  io.KeyMap[ImGuiKey_4]              = XPLM_KEY_4;
  io.KeyMap[ImGuiKey_5]              = XPLM_KEY_5;
  io.KeyMap[ImGuiKey_6]              = XPLM_KEY_6;
  io.KeyMap[ImGuiKey_7]              = XPLM_KEY_7;
  io.KeyMap[ImGuiKey_8]              = XPLM_KEY_8;
  io.KeyMap[ImGuiKey_9]              = XPLM_KEY_9;
  io.KeyMap[ImGuiKey_A]              = XPLM_VK_A;
  io.KeyMap[ImGuiKey_B]              = XPLM_VK_B;
  io.KeyMap[ImGuiKey_C]              = XPLM_VK_C;
  io.KeyMap[ImGuiKey_D]              = XPLM_VK_D;
  io.KeyMap[ImGuiKey_E]              = XPLM_VK_E;
  io.KeyMap[ImGuiKey_F]              = XPLM_VK_F;
  io.KeyMap[ImGuiKey_G]              = XPLM_VK_G;
  io.KeyMap[ImGuiKey_H]              = XPLM_VK_H;
  io.KeyMap[ImGuiKey_I]              = XPLM_VK_I;
  io.KeyMap[ImGuiKey_J]              = XPLM_VK_J;
  io.KeyMap[ImGuiKey_K]              = XPLM_VK_K;
  io.KeyMap[ImGuiKey_L]              = XPLM_VK_L;
  io.KeyMap[ImGuiKey_M]              = XPLM_VK_M;
  io.KeyMap[ImGuiKey_N]              = XPLM_VK_N;
  io.KeyMap[ImGuiKey_O]              = XPLM_VK_O;
  io.KeyMap[ImGuiKey_P]              = XPLM_VK_P;
  io.KeyMap[ImGuiKey_Q]              = XPLM_VK_Q;
  io.KeyMap[ImGuiKey_R]              = XPLM_VK_R;
  io.KeyMap[ImGuiKey_S]              = XPLM_VK_S;
  io.KeyMap[ImGuiKey_T]              = XPLM_VK_T;
  io.KeyMap[ImGuiKey_U]              = XPLM_VK_U;
  io.KeyMap[ImGuiKey_V]              = XPLM_VK_V;
  io.KeyMap[ImGuiKey_W]              = XPLM_VK_W;
  io.KeyMap[ImGuiKey_X]              = XPLM_VK_X;
  io.KeyMap[ImGuiKey_Y]              = XPLM_VK_Y;
  io.KeyMap[ImGuiKey_Z]              = XPLM_VK_Z;
  //io.KeyMap[ImGuiKey_F1]             = XPLM_VK_;
  //io.KeyMap[ImGuiKey_F2]             = XPLM_VK_;
  //io.KeyMap[ImGuiKey_F3]             = XPLM_VK_;
  //io.KeyMap[ImGuiKey_F4]             = XPLM_VK_;
  //io.KeyMap[ImGuiKey_F5]             = XPLM_VK_;
  //io.KeyMap[ImGuiKey_F6]             = XPLM_VK_;
  //io.KeyMap[ImGuiKey_F7]             = XPLM_VK_;
  //io.KeyMap[ImGuiKey_F8]             = XPLM_VK_;
  //io.KeyMap[ImGuiKey_F9]             = XPLM_VK_;
  //io.KeyMap[ImGuiKey_F10]            = XPLM_VK_;
  //io.KeyMap[ImGuiKey_F11]            = XPLM_VK_;
  //io.KeyMap[ImGuiKey_F12]            = XPLM_VK_;
  io.KeyMap[ImGuiKey_Apostrophe]     = XPLM_VK_QUOTE;
  io.KeyMap[ImGuiKey_Comma]          = XPLM_VK_COMMA;
  io.KeyMap[ImGuiKey_Minus]          = XPLM_VK_MINUS;
  io.KeyMap[ImGuiKey_Period]         = XPLM_VK_PERIOD;
  io.KeyMap[ImGuiKey_Slash]          = XPLM_VK_SLASH;
  io.KeyMap[ImGuiKey_Semicolon]      = XPLM_VK_SEMICOLON;
  io.KeyMap[ImGuiKey_Equal]          = XPLM_VK_EQUAL;
  io.KeyMap[ImGuiKey_LeftBracket]    = XPLM_VK_LBRACE;
  io.KeyMap[ImGuiKey_Backslash]      = XPLM_VK_BACKSLASH;
  io.KeyMap[ImGuiKey_RightBracket]   = XPLM_VK_RBRACE;
  //io.KeyMap[ImGuiKey_GraveAccent]    = XPLM_VK_;
  //io.KeyMap[ImGuiKey_CapsLock]       = XPLM_VK_;
  //io.KeyMap[ImGuiKey_ScrollLock]     = XPLM_VK_;
  //io.KeyMap[ImGuiKey_NumLock]        = XPLM_VK_;
  io.KeyMap[ImGuiKey_PrintScreen]    = XPLM_VK_PRINT;
  //io.KeyMap[ImGuiKey_Pause]          = XPLM_VK_;
  io.KeyMap[ImGuiKey_Keypad0]        = XPLM_VK_NUMPAD0;
  io.KeyMap[ImGuiKey_Keypad1]        = XPLM_VK_NUMPAD1;
  io.KeyMap[ImGuiKey_Keypad2]        = XPLM_VK_NUMPAD2;
  io.KeyMap[ImGuiKey_Keypad3]        = XPLM_VK_NUMPAD3;
  io.KeyMap[ImGuiKey_Keypad4]        = XPLM_VK_NUMPAD4;
  io.KeyMap[ImGuiKey_Keypad5]        = XPLM_VK_NUMPAD5;
  io.KeyMap[ImGuiKey_Keypad6]        = XPLM_VK_NUMPAD6;
  io.KeyMap[ImGuiKey_Keypad7]        = XPLM_VK_NUMPAD7;
  io.KeyMap[ImGuiKey_Keypad8]        = XPLM_VK_NUMPAD8;
  io.KeyMap[ImGuiKey_Keypad9]        = XPLM_VK_NUMPAD9;
  io.KeyMap[ImGuiKey_KeypadDecimal]  = XPLM_VK_DECIMAL;
  io.KeyMap[ImGuiKey_KeypadDivide]   = XPLM_VK_DIVIDE;
  io.KeyMap[ImGuiKey_KeypadMultiply] = XPLM_VK_MULTIPLY;
  io.KeyMap[ImGuiKey_KeypadSubtract] = XPLM_VK_SUBTRACT;
  io.KeyMap[ImGuiKey_KeypadAdd]      = XPLM_VK_ADD;
  io.KeyMap[ImGuiKey_KeypadEnter]    = XPLM_KEY_RETURN;
  io.KeyMap[ImGuiKey_KeypadEqual]    = XPLM_VK_EQUAL;
}
#endif



void
ImgWindow::initRemapKeys2()
{
  mapXPtoImgui_key[(int)XPLM_KEY_RETURN]  = ImGuiKey_Enter;
  mapXPtoImgui_key[(int)XPLM_KEY_ESCAPE]  = ImGuiKey_Escape;
  mapXPtoImgui_key[(int)XPLM_KEY_TAB]     = ImGuiKey_Tab;
  mapXPtoImgui_key[(int)XPLM_KEY_DELETE]  = ImGuiKey_Delete;
  mapXPtoImgui_key[(int)XPLM_KEY_LEFT]    = ImGuiKey_LeftArrow;
  mapXPtoImgui_key[(int)XPLM_KEY_RIGHT]   = ImGuiKey_RightArrow;
  mapXPtoImgui_key[(int)XPLM_KEY_UP]      = ImGuiKey_UpArrow;
  mapXPtoImgui_key[(int)XPLM_KEY_DOWN]    = ImGuiKey_DownArrow;
  mapXPtoImgui_key[(int)XPLM_KEY_0]       = ImGuiKey_0;
  mapXPtoImgui_key[(int)XPLM_KEY_1]       = ImGuiKey_1;
  mapXPtoImgui_key[(int)XPLM_KEY_2]       = ImGuiKey_2;
  mapXPtoImgui_key[(int)XPLM_KEY_3]       = ImGuiKey_3;
  mapXPtoImgui_key[(int)XPLM_KEY_4]       = ImGuiKey_4;
  mapXPtoImgui_key[(int)XPLM_KEY_5]       = ImGuiKey_5;
  mapXPtoImgui_key[(int)XPLM_KEY_6]       = ImGuiKey_6;
  mapXPtoImgui_key[(int)XPLM_KEY_7]       = ImGuiKey_7;
  mapXPtoImgui_key[(int)XPLM_KEY_8]       = ImGuiKey_8;
  mapXPtoImgui_key[(int)XPLM_KEY_9]       = ImGuiKey_9;
  mapXPtoImgui_key[(int)XPLM_KEY_DECIMAL] = ImGuiKey_Period;
  mapXPtoImgui_key[(int)XPLM_VK_BACK]     = ImGuiKey_Backspace;
  mapXPtoImgui_key[(int)XPLM_VK_TAB]      = ImGuiKey_Tab;
  // mapXPtoImgui_key[(int)XPLM_VK_CLEAR]      = ImGuiKey_;
  //mapXPtoImgui_key[(int)XPLM_VK_RETURN] = ImGuiKey_Enter;
  //mapXPtoImgui_key[(int)XPLM_VK_ESCAPE] = ImGuiKey_Escape;
  mapXPtoImgui_key[(int)XPLM_VK_SPACE]  = ImGuiKey_Space;
  mapXPtoImgui_key[(int)XPLM_VK_PRIOR]  = ImGuiKey_PageUp;
  mapXPtoImgui_key[(int)XPLM_VK_NEXT]   = ImGuiKey_PageDown;
  mapXPtoImgui_key[(int)XPLM_VK_END]    = ImGuiKey_End;
  mapXPtoImgui_key[(int)XPLM_VK_HOME]   = ImGuiKey_Home;
  mapXPtoImgui_key[(int)XPLM_VK_LEFT]   = ImGuiKey_LeftArrow;
  mapXPtoImgui_key[(int)XPLM_VK_UP]     = ImGuiKey_UpArrow;
  mapXPtoImgui_key[(int)XPLM_VK_RIGHT]  = ImGuiKey_RightArrow;
  mapXPtoImgui_key[(int)XPLM_VK_DOWN]   = ImGuiKey_DownArrow;
  // mapXPtoImgui_key[(int)XPLM_VK_SELECT]     = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_PRINT]      = ImGuiKey_PrintScreen;
  // mapXPtoImgui_key[(int)XPLM_VK_EXECUTE]    = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_SNAPSHOT]   = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_INSERT]     = ImGuiKey_;
  mapXPtoImgui_key[(int)XPLM_VK_DELETE] = ImGuiKey_Delete;
  // mapXPtoImgui_key[(int)XPLM_VK_HELP]       = ImGuiKey_;
  mapXPtoImgui_key[(int)XPLM_VK_0]        = ImGuiKey_0;
  mapXPtoImgui_key[(int)XPLM_VK_1]        = ImGuiKey_1;
  mapXPtoImgui_key[(int)XPLM_VK_2]        = ImGuiKey_2;
  mapXPtoImgui_key[(int)XPLM_VK_3]        = ImGuiKey_3;
  mapXPtoImgui_key[(int)XPLM_VK_4]        = ImGuiKey_4;
  mapXPtoImgui_key[(int)XPLM_VK_5]        = ImGuiKey_5;
  mapXPtoImgui_key[(int)XPLM_VK_6]        = ImGuiKey_6;
  mapXPtoImgui_key[(int)XPLM_VK_7]        = ImGuiKey_7;
  mapXPtoImgui_key[(int)XPLM_VK_8]        = ImGuiKey_8;
  mapXPtoImgui_key[(int)XPLM_VK_9]        = ImGuiKey_9;
  mapXPtoImgui_key[(int)XPLM_VK_A]        = ImGuiKey_A;
  mapXPtoImgui_key[(int)XPLM_VK_B]        = ImGuiKey_B;
  mapXPtoImgui_key[(int)XPLM_VK_C]        = ImGuiKey_C;
  mapXPtoImgui_key[(int)XPLM_VK_D]        = ImGuiKey_D;
  mapXPtoImgui_key[(int)XPLM_VK_E]        = ImGuiKey_E;
  mapXPtoImgui_key[(int)XPLM_VK_F]        = ImGuiKey_F;
  mapXPtoImgui_key[(int)XPLM_VK_G]        = ImGuiKey_G;
  mapXPtoImgui_key[(int)XPLM_VK_H]        = ImGuiKey_H;
  mapXPtoImgui_key[(int)XPLM_VK_I]        = ImGuiKey_I;
  mapXPtoImgui_key[(int)XPLM_VK_J]        = ImGuiKey_J;
  mapXPtoImgui_key[(int)XPLM_VK_K]        = ImGuiKey_K;
  mapXPtoImgui_key[(int)XPLM_VK_L]        = ImGuiKey_L;
  mapXPtoImgui_key[(int)XPLM_VK_M]        = ImGuiKey_M;
  mapXPtoImgui_key[(int)XPLM_VK_N]        = ImGuiKey_N;
  mapXPtoImgui_key[(int)XPLM_VK_O]        = ImGuiKey_O;
  mapXPtoImgui_key[(int)XPLM_VK_P]        = ImGuiKey_P;
  mapXPtoImgui_key[(int)XPLM_VK_Q]        = ImGuiKey_Q;
  mapXPtoImgui_key[(int)XPLM_VK_R]        = ImGuiKey_R;
  mapXPtoImgui_key[(int)XPLM_VK_S]        = ImGuiKey_S;
  mapXPtoImgui_key[(int)XPLM_VK_T]        = ImGuiKey_T;
  mapXPtoImgui_key[(int)XPLM_VK_U]        = ImGuiKey_U;
  mapXPtoImgui_key[(int)XPLM_VK_V]        = ImGuiKey_V;
  mapXPtoImgui_key[(int)XPLM_VK_W]        = ImGuiKey_W;
  mapXPtoImgui_key[(int)XPLM_VK_X]        = ImGuiKey_X;
  mapXPtoImgui_key[(int)XPLM_VK_Y]        = ImGuiKey_Y;
  mapXPtoImgui_key[(int)XPLM_VK_Z]        = ImGuiKey_Z;
  mapXPtoImgui_key[(int)XPLM_VK_NUMPAD0]  = ImGuiKey_Keypad0;
  mapXPtoImgui_key[(int)XPLM_VK_NUMPAD1]  = ImGuiKey_Keypad1;
  mapXPtoImgui_key[(int)XPLM_VK_NUMPAD2]  = ImGuiKey_Keypad2;
  mapXPtoImgui_key[(int)XPLM_VK_NUMPAD3]  = ImGuiKey_Keypad3;
  mapXPtoImgui_key[(int)XPLM_VK_NUMPAD4]  = ImGuiKey_Keypad4;
  mapXPtoImgui_key[(int)XPLM_VK_NUMPAD5]  = ImGuiKey_Keypad5;
  mapXPtoImgui_key[(int)XPLM_VK_NUMPAD6]  = ImGuiKey_Keypad6;
  mapXPtoImgui_key[(int)XPLM_VK_NUMPAD7]  = ImGuiKey_Keypad7;
  mapXPtoImgui_key[(int)XPLM_VK_NUMPAD8]  = ImGuiKey_Keypad8;
  mapXPtoImgui_key[(int)XPLM_VK_NUMPAD9]  = ImGuiKey_Keypad9;
  mapXPtoImgui_key[(int)XPLM_VK_MULTIPLY] = ImGuiKey_KeypadMultiply;
  mapXPtoImgui_key[(int)XPLM_VK_ADD]      = ImGuiKey_KeypadAdd;
  mapXPtoImgui_key[(int)XPLM_VK_SUBTRACT] = ImGuiKey_Minus;
  mapXPtoImgui_key[(int)XPLM_VK_DECIMAL]  = ImGuiKey_Period;
  mapXPtoImgui_key[(int)XPLM_VK_DIVIDE]   = ImGuiKey_KeypadDivide;

  mapXPtoImgui_key[(int)XPLM_VK_EQUAL] = ImGuiKey_Equal;
  mapXPtoImgui_key[(int)XPLM_VK_MINUS] = ImGuiKey_Minus;
  mapXPtoImgui_key[(int)XPLM_VK_QUOTE]     = ImGuiKey_Apostrophe;
  mapXPtoImgui_key[(int)XPLM_VK_SEMICOLON] = ImGuiKey_Semicolon;
  mapXPtoImgui_key[(int)XPLM_VK_BACKSLASH] = ImGuiKey_Backslash;
  mapXPtoImgui_key[(int)XPLM_VK_COMMA]     = ImGuiKey_Comma;
  mapXPtoImgui_key[(int)XPLM_VK_SLASH]     = ImGuiKey_Slash;
  mapXPtoImgui_key[(int)XPLM_VK_PERIOD]    = ImGuiKey_Period;

  mapXPtoImgui_key[(int)XPLM_VK_ENTER]      = ImGuiKey_Enter;
  mapXPtoImgui_key[(int)XPLM_VK_NUMPAD_ENT] = ImGuiKey_Enter;
  mapXPtoImgui_key[(int)XPLM_VK_NUMPAD_EQ]  = ImGuiKey_Equal;

  // mapXPtoImgui_key[(int)XPLM_VK_F1]         = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F2]         = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F3]         = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F4]         = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F5]         = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F6]         = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F7]         = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F8]         = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F9]         = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F10]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F11]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F12]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F13]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F14]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F15]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F16]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F17]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F18]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F19]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F20]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F21]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F22]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F23]        = ImGuiKey_;
  // mapXPtoImgui_key[(int)XPLM_VK_F24]        = ImGuiKey_;

  // mapXPtoImgui_key[(int)XPLM_VK_SEPARATOR]  = ImGuiKey_;

  // mapXPtoImgui_key[(int)XPLM_VK_RBRACE]     = ImGuiKey_RightBracket;
  // mapXPtoImgui_key[(int)XPLM_VK_LBRACE]     = ImGuiKey_LeftBracket;


  // mapXPtoImgui_key[(int)XPLM_VK_BACKQUOTE]  = ImGuiKey_;
}
