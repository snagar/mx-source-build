#include "MXCDisplay.h"

missionx::MXCKeySniffer::MXCKeySniffer(int inBeforeWindows)
  : mBeforeWindows(inBeforeWindows)
{
  XPLMRegisterKeySniffer(KeySnifferCB, mBeforeWindows, reinterpret_cast<void*>(this));
}

missionx::MXCKeySniffer::~MXCKeySniffer()
{
  XPLMUnregisterKeySniffer(KeySnifferCB, mBeforeWindows, reinterpret_cast<void*>(this));
}


int
missionx::MXCKeySniffer::KeySnifferCB(char inCharKey, XPLMKeyFlags inFlags, char inVirtualKey, void* inRefCon)
{
  MXCKeySniffer* me = reinterpret_cast<MXCKeySniffer*>(inRefCon);
  return me->HandleKeyStroke(inCharKey, inFlags, inVirtualKey);
}



missionx::MXCWindow::MXCWindow()
{
  mWindow    = NULL;
  lastMouseX = lastMouseY = 0;
}

void
missionx::MXCWindow::createWindow(XPLMCreateWindow_t& params)
{
  mWindow = XPLMCreateWindowEx(&params);
}

missionx::MXCWindow::~MXCWindow()
{
  XPLMDestroyWindow(mWindow);
}


void
missionx::MXCWindow::GetWindowGeometry(int* outLeft, int* outTop, int* outRight, int* outBottom)
{
  XPLMGetWindowGeometry(mWindow, outLeft, outTop, outRight, outBottom);
}

void
missionx::MXCWindow::SetWindowGeometry(int inLeft, int inTop, int inRight, int inBottom)
{
  XPLMSetWindowGeometry(mWindow, inLeft, inTop, inRight, inBottom);
}

int
missionx::MXCWindow::GetWindowIsVisible(void)
{
  return XPLMGetWindowIsVisible(mWindow);
}

void
missionx::MXCWindow::SetWindowIsVisible(int inIsVisible)
{
  XPLMSetWindowIsVisible(mWindow, inIsVisible);
}

void
missionx::MXCWindow::TakeKeyboardFocus(void)
{
  XPLMTakeKeyboardFocus(mWindow);
}

void
missionx::MXCWindow::BringWindowToFront(void)
{
  XPLMBringWindowToFront(mWindow);
}

int
missionx::MXCWindow::IsWindowInFront(void)
{
  return XPLMIsWindowInFront(mWindow);
}

void
missionx::MXCWindow::DrawCB(XPLMWindowID inWindowID, void* inRefcon)
{
  MXCWindow* me = reinterpret_cast<MXCWindow*>(inRefcon);
  me->DoDraw();
}

void
missionx::MXCWindow::HandleKeyCB(XPLMWindowID inWindowID, char inKey, XPLMKeyFlags inFlags, char inVirtualKey, void* inRefcon, int losingFocus)
{
  MXCWindow* me = reinterpret_cast<MXCWindow*>(inRefcon);
  if (losingFocus)
    me->LoseFocus();
  else
    me->HandleKey(inKey, inFlags, inVirtualKey);
}

// XPLMCursorStatus missionx::MXCWindow::HandleCursorCB(XPLMWindowID inWindowID, int x, int y, void * inRefcon)
//{
//  MXCWindow * me = reinterpret_cast<MXCWindow *>(inRefcon);
//  return me->HandleCursor(inWindowID, x, y, me);
//}

int
missionx::MXCWindow::MouseClickCB(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void* inRefcon)
{
  MXCWindow* me = reinterpret_cast<MXCWindow*>(inRefcon);
  return me->HandleClick(x, y, inMouse);
}

int
missionx::MXCWindow::MouseHandleCB(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void* inRefcon) // missionx XPLM300
{
  MXCWindow* me = reinterpret_cast<MXCWindow*>(inRefcon);
  return me->HandleMouse(me->mWindow, x, y, inMouse, me);
}

int
missionx::MXCWindow::RightMouseHandleCB(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void* inRefcon)
{
  MXCWindow* me = reinterpret_cast<MXCWindow*>(inRefcon);
  return me->HandleRightMouse(me->mWindow, x, y, inMouse, me);
}

int
missionx::MXCWindow::MouseWheelHandleCB(XPLMWindowID in_window_id, int x, int y, int wheel, int clicks, void* inRefcon)
{
  MXCWindow* me = reinterpret_cast<MXCWindow*>(inRefcon);
  return me->HandleMouseWheel(me->mWindow, x, y, wheel, clicks, me);
}

void
missionx::MXCWindow::printString(int x, int y, std::string msg)
{
}

// missionx
XPLMWindowID
missionx::MXCWindow::getWindowId(void)
{
  return mWindow;
}

bool
missionx::MXCWindow::is_pointer_in_ui_area(float x, float y, bool is_popped_out)
{
  int outLeft   = 0;
  int outTop    = 0;
  int outRight  = 0;
  int outBottom = 0;
  // int bounds[4] = { 0 }; /* left, bottom, right, top */
  int isPoppedOut = XPLMWindowIsPoppedOut(this->mWindow); // ignore "is_popped_out", using internal code

  if (isPoppedOut)
    XPLMGetWindowGeometryOS(mWindow, &outLeft, &outTop, &outRight, &outBottom);
  else
    XPLMGetWindowGeometry(mWindow, &outLeft, &outTop, &outRight, &outBottom);

  return ((x > outLeft && x < outRight) && (y < outTop && y > outBottom));
}


void
missionx::MXCWindow::receive_main_monitor_bounds(int inMonitorIndex, int inLeftBx, int inTopBx, int inRightBx, int inBottomBx, void* refcon)
{
  int* main_monitor_bounds = (int*)refcon; // should be our bounds[] array
  if (inMonitorIndex == 0)                 /* the main monitor */
  {
    main_monitor_bounds[0] = inLeftBx;
    main_monitor_bounds[1] = inBottomBx;
    main_monitor_bounds[2] = inRightBx;
    main_monitor_bounds[3] = inTopBx;
  }
}


void
missionx::MXCWindow::toggleVisibleState()
{
  if (mWindow == NULL)
    return;

  if (uiState == missionx::mx_ui_state::ui_hidden || !XPLMGetWindowIsVisible(mWindow))
  {
    uiState = mx_ui_state::ui_enabled_and_visible; // for windows this means visible
    XPLMSetWindowIsVisible(mWindow, true);
    if (!XPLMIsWindowInFront(mWindow))
    {
      XPLMBringWindowToFront(mWindow);
    }
  }
  else
  {
    uiState = mx_ui_state::ui_hidden; // meaning do not draw
    XPLMSetWindowIsVisible(mWindow, false);
  }
}
