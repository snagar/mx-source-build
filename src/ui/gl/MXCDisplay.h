#ifndef _MXCDisplay_h_
#define _MXCDisplay_h_

#include "../MxUICore.hpp"
#include "XPLMDisplay.h"

//#include "stb_truetype.h"
//#include "../core/FontFile.h"


namespace missionx
{



class MXCKeySniffer
{
public:
  MXCKeySniffer(int inBeforeWindows);
  virtual ~MXCKeySniffer();

  virtual int HandleKeyStroke(char inCharKey, XPLMKeyFlags inFlags, char inVirtualKey) = 0;

private:
  int mBeforeWindows;

  static int KeySnifferCB(char inCharKey, XPLMKeyFlags inFlags, char inVirtualKey, void* inRefCon);
};


// missionx dummy
[[maybe_unused]]
static int
dummy_missionx_mouse_handler(XPLMWindowID in_window_id, int x, int y, int is_down, void* in_refcon)
{
  return 0;
}
[[maybe_unused]]
static XPLMCursorStatus
dummy_missionx_cursor_status_handler(XPLMWindowID in_window_id, int x, int y, void* in_refcon)
{
  return xplm_CursorDefault;
}
[[maybe_unused]]
static int
dummy_missionx_wheel_handler(XPLMWindowID in_window_id, int x, int y, int wheel, int clicks, void* in_refcon)
{
  return 0;
}
[[maybe_unused]]
static void
dummy_missionx_key_handler(XPLMWindowID in_window_id, char key, XPLMKeyFlags flags, char virtual_key, void* in_refcon, int losing_focus)
{
}


class MXCWindow : public missionx::MxUICore
{
public:
  // core
  XPLMWindowID mWindow;
  int          lastMouseX, lastMouseY;


  MXCWindow(); // missionx
  virtual ~MXCWindow();

  // MXCWindow( XPLMCreateWindow_t &params); // missionx
  void createWindow(XPLMCreateWindow_t& params);

  // virtual void print(const missionx::MxFont&, const int xpos, const int ypos, const std::string text) = 0;

  // virtual	void	DoDraw(XPLMWindowID in_window_id) = 0;
  virtual void DoDraw()                                                       = 0;
  virtual void HandleKey(char inKey, XPLMKeyFlags inFlags, char inVirtualKey) = 0;
  virtual void LoseFocus(void)                                                = 0;
  // virtual XPLMCursorStatus  HandleCursor(XPLMWindowID in_window_id, int x, int y, void * in_refcon) = 0; // // missionx XPLM200
  virtual int HandleClick(int x, int y, XPLMMouseStatus inMouse)                                                  = 0;
  virtual int HandleMouse(XPLMWindowID in_window_id, int x, int y, XPLMMouseStatus is_down, void* in_refcon)      = 0; // // missionx XPLM300
  virtual int HandleRightMouse(XPLMWindowID in_window_id, int x, int y, XPLMMouseStatus is_down, void* in_refcon) = 0; // // missionx XPLM300

  virtual int HandleMouseWheel(XPLMWindowID in_window_id, int x, int y, int wheel, int clicks, void* in_refcon) { return 0; } // v3.0.197

  void GetWindowGeometry(int* outLeft, int* outTop, int* outRight, int* outBottom);
  void SetWindowGeometry(int inLeft, int inTop, int inRight, int inBottom);
  int  GetWindowIsVisible(void);
  void SetWindowIsVisible(int inIsVisible);
  void TakeKeyboardFocus(void);
  void BringWindowToFront(void);
  int  IsWindowInFront(void);

  XPLMWindowID getWindowId(void);                                                   // missionx
  bool         is_pointer_in_ui_area(float x, float y, bool is_popped_out = false); // missionx


  virtual void toggleVisibleState();

  static void receive_main_monitor_bounds(int inMonitorIndex, int inLeftBx, int inTopBx, int inRightBx, int inBottomBx, void* refcon);


  static void DrawCB(XPLMWindowID inWindowID, void* inRefcon);
  static void HandleKeyCB(XPLMWindowID inWindowID, char inKey, XPLMKeyFlags inFlags, char inVirtualKey, void* inRefcon, int losingFocus);
  // static	XPLMCursorStatus 	HandleCursorCB(XPLMWindowID inWindowID, int x, int y, void * inRefcon);
  static int MouseClickCB(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void* inRefcon);
  static int MouseHandleCB(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void* inRefcon);      // missionx XPLM300
  static int RightMouseHandleCB(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void* inRefcon); // missionx XPLM300
  static int MouseWheelHandleCB(XPLMWindowID in_window_id, int x, int y, int wheel, int clicks, void* inRefcon); // missionx XPLM300 // v3.0.197

  void printString(int x, int y, std::string msg);
};

}
#endif
