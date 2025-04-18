#ifndef MXPADMSGLINE_H_
#define MXPADMSGLINE_H_
#pragma once

#include "../ui/gl/mxButton.hpp"
#include <string>

namespace missionx
{

class mxpadMsgLine
{
public:
  std::string label;
  std::string text;

  missionx::mx_btn_colors lblColor;
  missionx::mx_btn_colors txtColor;

  mxpadMsgLine()
  {
    label.clear();
    text.clear();
    lblColor = missionx::mx_btn_colors::white;
    txtColor = missionx::mx_btn_colors::white;
  }
  void clone(mxpadMsgLine& inMsgLine)
  {
    label    = inMsgLine.label;
    text     = inMsgLine.text;
    lblColor = inMsgLine.lblColor;
    txtColor = inMsgLine.txtColor;
  }

  void operator=(mxpadMsgLine& inMsgLine) { mxpadMsgLine::clone(inMsgLine); }
};


class mxPadLine
{
public:
  mxPadLine()
  {
    xp_btnHide.setDisplayHighlightHover(true); // v3.0.211.3

    // scrollDescription.x = 0;
    // scrollDescription.y = 0;

    // ptrFont01 = nullptr;
    scrollBarXOffset = 0;

    xp_btnSelction.setDisplayHighlightHover(true);
    xp_btnSelction.setTextAlignment(missionx::mx_alignment::align_left, missionx::mx_alignment::align_center);
  }

  ~mxPadLine() {}

  // MxFontNk  * ptrFont01; // v3.0.160

  missionx::MxButton xp_btnHide; // v3.0.211.3 hide button
  missionx::MxButton xp_label;   // v3.0.211.3 colored label in mxpad message
  missionx::MxButton xp_textMsg; // v3.0.211.3 represent raw of message to draw

//  missionx::MxButtonTexture xp_btnDummy; // v3.0.213.2 will be used for any icon based button except hide


  std::deque<missionx::mxpadMsgLine> textLines;
  int                                userScrollSetToDiplay;    // store the user next/prev set he/she clicked to display the relative lines.
  int                                countSetsInMessageBuffer; // holds how many sets of line are there. it should be function of

  // missionx::MxButton xp_btnSel[4];
  missionx::MxButton xp_btnSelction;

  // missionx::Timer timerMessage;


  // nk_scroll scrollDescription;
  int scrollBarXOffset; // = WINDOWS_WIDTH - 20;
};

} // namespace
#endif
