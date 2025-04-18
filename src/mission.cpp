#define NOMINMAX // used for std::numeric_limits<float>::max()
#include "mission.h"
#include <limits>
#include <filesystem>
#include <utility>
namespace fs = std::filesystem;

// The raw TTF data of OpenFontIcons has been generated into the following file
#include "fa-solid-900.v15.4.inc"
#include <IconsFontAwesome5.h> // inside libs/imgui4xp

// v3.303.14 Added explicitly since we removed mxconst::QMM from "read_mission_file" class
#include "core/QueueMessageManager.h"
#include "io/ListDir.h"

namespace missionx
{
bool Mission::isMissionValid;

std::string Mission::currentObjectiveName;

std::vector<std::string> Mission::vecGoalsExecOrder;

ImgOptionsSPtrTy   Mission::uiImGuiOptions; // v3.0.251.1
ImgMxpadSPtrTy     Mission::uiImGuiMxpad;   // v3.0.251.1
ImguiBrieferSPtrTy Mission::uiImGuiBriefer; // v3.0.251.1

bool Mission::usingCustomMetarFile; // v3.0.201

int missionx::Mission::displayCueInfo; // v3.0.203.5

void
add_icons_to_fonts(const std::shared_ptr<ImgFontAtlas>& inOutFontAtlas, const float inFontSize)
{
  // Now we merge some icons from the OpenFontsIcons font into the above font
  // (see `imgui/docs/FONTS.txt`)
  ImFontConfig config;
  config.MergeMode = true;

  // We only read very selectively the individual glyphs we are actually using
  // to save on texture space
  static ImVector<ImWchar> icon_ranges;

  ImFontGlyphRangesBuilder builder;
  // Add all icons that are actually used (they concatenate into one string)
  builder.AddText(ICON_FA_ARROW_DOWN ICON_FA_ARROW_UP ICON_FA_PLAY ICON_FA_REPLY ICON_FA_SAVE ICON_FA_SIGN_OUT_ALT ICON_FA_FAST_FORWARD ICON_FA_STEP_FORWARD ICON_FA_PAUSE ICON_FA_TRASH ICON_FA_TRASH_ALT ICON_FA_SEARCH ICON_FA_EXTERNAL_LINK_SQUARE_ALT ICON_FA_WINDOW_MAXIMIZE ICON_FA_WINDOW_MINIMIZE ICON_FA_WINDOW_RESTORE ICON_FA_WINDOW_CLOSE
                    ICON_FA_SYNC
                    ICON_FA_REPLY ICON_FA_USER_LOCK);
  builder.BuildRanges(&icon_ranges);

  // Merge the icon font with the text font
  inOutFontAtlas->AddFontFromMemoryCompressedTTF(fa_solid_900_compressed_data, fa_solid_900_compressed_size, inFontSize, &config, icon_ranges.Data);
}

void
configureImgWindow()
{
  //ImgWindow::sFont1 = std::make_shared<ImgFontAtlas>();

  ImgWindow::sFont1 = std::make_shared<ImgFontAtlas>();
  // Create default Font + icons
  // When calling: "ImFileOpen" we must have a context ready. This was not needed prior to ImGui v1.90
  ImGui::CreateContext();  // v3.305.3


  // use actual parameters to configure the font, or use one of the other methods.

  // this is a post from kuroneko on x-plane.org explaining this use.

  // Basic setup looks something like:
  // To avoid bleeding VRAM like it's going out of fashion, there is only one font atlas shared over all ImgWindows
  // and we keep the managed pointer to it in the ImgWindow class statics.

  // I use the C++11 managed/smart pointers to enforce RAII behaviors rather than encouraging use of new/delete.
  //  This means the font atlas will only get destroyed when you break all references to it.
  // (ie: via ImgWindow::sFont1.reset())  You should never really need to do that though,
  // unless you're being disabled (because you'll lose your texture handles anyway and it's probably a good idea
  // to forcibly tear down the font atlas then).

  // It's probably a bug that the instance of ImgWindow doesn't actually take a copy of the shared_ptr to ensure
  // the font atlas remains valid until it's destroyed.  I was working on a lot of things when I threw that update
  // together and I was heading down that path, but I think I forgot to finish it.

  // you can use any of these fonts that are provided with X-Plane or find you own.
  // Currently you can only load one font and not sure if this might change in the future.
  // ImgWindow::sFont1->AddFontFromFileTTF("./Resources/fonts/DejaVuSans.ttf", FONT_SIZE);

  std::string defaultMetaData = R"(## Created by Mission-X Plugin
##
## Mission-X font ini file.
##
## Please do not modify it unless you know what you are doing.
## Remember: always make a copy before you alter anything
##############################################################

## Font File
font=./Resources/fonts/DejaVuSans.ttf
font=./Resources/fonts/Inconsolata.ttf
font=./Resources/fonts/tahomabd.ttf
font=./Resources/fonts/Roboto-Bold.ttf
#font=./Resources/plugins/missionx/libs/fonts/EBGaramond-Bold.ttf

default_font=DejaVuSans.ttf,13

## Font Type and Size
title_reg=tahomabd.ttf,15
title_small=tahomabd.ttf,14
title_smallest=tahomabd.ttf,13
title_med=tahomabd.ttf,18
title_big=tahomabd.ttf,24

title_toolbar=DejaVuSans.ttf,24

# text_reg=EBGaramond-Bold.ttf,18
# text_small=EBGaramond-Bold.ttf,16

text_reg=Inconsolata.ttf,14
text_small=Inconsolata.ttf,13
text_med=Inconsolata.ttf,26

msg_bottom=Roboto-Bold.ttf,13
msg_popup=Roboto-Bold.ttf,15
)"; 

  const std::string cn_default_font_name = "DejaVuSans.ttf";
  const std::string cn_default_font_expr = "DejaVuSans.ttf,13";

  int         seq {0}; // seq 0 is stored with the first default font
  float       fontSizePx{ 13.0f };
  std::string fontName;
  // Lambda functions
  const auto lmbda_eval_font_data = [func = __func__](const std::vector<std::string>& inVec, std::string& outFontName_s, float& outSizePx_f, std::string inDefaultFontName, float inDefaultSize)
  {
    
    assert( (inVec.empty() == false) && fmt::format ("[{}], Metadata Font must not be empty", func).c_str());

    const std::string&              value_s  = inVec.at(0);
    if (std::vector<std::string> vecSplit = mxUtils::split_v2(value_s, ",");
        vecSplit.size() < static_cast<size_t>(2))
    {
      outFontName_s = std::move(inDefaultFontName);
      outSizePx_f   = inDefaultSize;
    }
    else
    {
      outFontName_s = vecSplit.at(0);
      outSizePx_f   = mxUtils::stringToNumber<float>(vecSplit.at(1), 2);
    }
  };

  const auto lmbda_eval_font_name_and_return_existing_one = [](std::string inFontName, std::string inDefaultName)
  {
    if (mxUtils::isElementExists(MxUICore::mapFontsMeta, inFontName))
      return inFontName;

    return inDefaultName;
  };

  ////// End Lambda functions //////


  // Read fonts from fonts.ini file
  auto vecFonts = missionx::ListDir::readFontMetadata("font=", 0, cn_default_font_expr, defaultMetaData);

  std::string sTitleRegFontName;

  if (!vecFonts.empty())
  {
    // initialize the font file meta data
    std::vector<std::string> vecFontName;
    vecFontName.clear();
    std::ranges::for_each(std::as_const(vecFonts),
                  [&](const fs::path& p)
                  {
                    vecFontName.emplace_back(p.filename().string());
                    Utils::addElementToMap(MxUICore::mapFontsMeta, p.filename().string(), MxUICore::mxFontData(p.filename().string(), p.string())); // store font name and font location
                  });


    // read default font and force defaults for this specific font case
    std::vector<std::string> vecTemp = missionx::ListDir::readFontMetadata(mxconst::get_TEXT_TYPE_DEFAULT() + "=", 1, cn_default_font_expr, defaultMetaData);

    assert(vecTemp.empty() == false && std::string(__func__).append(", vecTemp Font must not be empty").c_str());


    if (vecTemp.empty())
      MxUICore::mxDefaultFontName = cn_default_font_name;
    else
    {
      auto vecDefaultFont = mxUtils::split_v2(vecTemp.at(0), ",");
      assert(vecDefaultFont.empty() == false && std::string(__func__).append(", vecDefaultFont must not be empty").c_str());
      MxUICore::mxDefaultFontName = vecDefaultFont.at(0);
    }

    lmbda_eval_font_data(vecTemp, fontName, fontSizePx, cn_default_font_name, mxconst::FONT_PIXEL_13);
    fontName   = MxUICore::mxDefaultFontName;                                                         // force default font name
    fontSizePx = mxconst::FONT_PIXEL_13;                                                              // force size 13px as default size - special case
    MxUICore::mapFontsMeta[fontName].initFontMeta(fontSizePx, mxconst::get_TEXT_TYPE_DEFAULT());            // init DEFAULT FONT
    MxUICore::mapFontsMeta[fontName].initFontMeta(fontSizePx + 1, mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1()); // init DEFAULT FONT + 1



    // read title font
    vecTemp = missionx::ListDir::readFontMetadata(mxconst::get_TEXT_TYPE_TITLE_REG() + "=", 1, cn_default_font_expr, defaultMetaData);
    lmbda_eval_font_data(vecTemp, fontName, fontSizePx, MxUICore::mxDefaultFontName, mxconst::FONT_PIXEL_15);
    fontName = lmbda_eval_font_name_and_return_existing_one(fontName, MxUICore::mxDefaultFontName);
    MxUICore::mapFontsMeta[fontName].initFontMeta(fontSizePx, mxconst::get_TEXT_TYPE_TITLE_REG());  
    sTitleRegFontName = fontName; // v3.305.3

    vecTemp = missionx::ListDir::readFontMetadata(mxconst::get_TEXT_TYPE_TITLE_MED() + "=", 1, cn_default_font_expr, defaultMetaData);
    lmbda_eval_font_data(vecTemp, fontName, fontSizePx, MxUICore::mxDefaultFontName, mxconst::FONT_PIXEL_18);
    fontName = lmbda_eval_font_name_and_return_existing_one(fontName, MxUICore::mxDefaultFontName);
    MxUICore::mapFontsMeta[fontName].initFontMeta(fontSizePx, mxconst::get_TEXT_TYPE_TITLE_MED());

    vecTemp = missionx::ListDir::readFontMetadata(mxconst::get_TEXT_TYPE_TITLE_BIG() + "=", 1, cn_default_font_expr, defaultMetaData);
    lmbda_eval_font_data(vecTemp, fontName, fontSizePx, MxUICore::mxDefaultFontName, mxconst::FONT_PIXEL_24);
    fontName = lmbda_eval_font_name_and_return_existing_one(fontName, MxUICore::mxDefaultFontName);
    MxUICore::mapFontsMeta[fontName].initFontMeta(fontSizePx, mxconst::get_TEXT_TYPE_TITLE_BIG());

    // Read text fonts
    vecTemp = missionx::ListDir::readFontMetadata(mxconst::get_TEXT_TYPE_TEXT_REG() + "=", 1, cn_default_font_expr, defaultMetaData);
    lmbda_eval_font_data(vecTemp, fontName, fontSizePx, MxUICore::mxDefaultFontName, mxconst::FONT_PIXEL_14);
    fontName = lmbda_eval_font_name_and_return_existing_one(fontName, MxUICore::mxDefaultFontName);
    MxUICore::mapFontsMeta[fontName].initFontMeta(fontSizePx, mxconst::get_TEXT_TYPE_TEXT_REG());

    vecTemp = missionx::ListDir::readFontMetadata(mxconst::get_TEXT_TYPE_TEXT_MED() + "=", 1, cn_default_font_expr, defaultMetaData);
    lmbda_eval_font_data(vecTemp, fontName, fontSizePx, MxUICore::mxDefaultFontName, mxconst::FONT_PIXEL_18);
    fontName = lmbda_eval_font_name_and_return_existing_one(fontName, MxUICore::mxDefaultFontName);
    MxUICore::mapFontsMeta[fontName].initFontMeta(fontSizePx, mxconst::get_TEXT_TYPE_TEXT_MED());

    vecTemp = missionx::ListDir::readFontMetadata(mxconst::get_TEXT_TYPE_TEXT_SMALL() + "=", 1, cn_default_font_expr, defaultMetaData);
    lmbda_eval_font_data(vecTemp, fontName, fontSizePx, MxUICore::mxDefaultFontName, mxconst::FONT_PIXEL_14);
    fontName = lmbda_eval_font_name_and_return_existing_one(fontName, MxUICore::mxDefaultFontName);
    MxUICore::mapFontsMeta[fontName].initFontMeta(fontSizePx, mxconst::get_TEXT_TYPE_TEXT_SMALL());

    vecTemp = missionx::ListDir::readFontMetadata(mxconst::get_TEXT_TYPE_MSG_BOTTOM() + "=", 1, cn_default_font_expr, defaultMetaData);
    lmbda_eval_font_data(vecTemp, fontName, fontSizePx, MxUICore::mxDefaultFontName, mxconst::FONT_PIXEL_13);
    fontName = lmbda_eval_font_name_and_return_existing_one(fontName, MxUICore::mxDefaultFontName);
    MxUICore::mapFontsMeta[fontName].initFontMeta(fontSizePx, mxconst::get_TEXT_TYPE_MSG_BOTTOM());

    vecTemp = missionx::ListDir::readFontMetadata(mxconst::get_TEXT_TYPE_MSG_POPUP() + "=", 1, cn_default_font_expr, defaultMetaData);
    lmbda_eval_font_data(vecTemp, fontName, fontSizePx, MxUICore::mxDefaultFontName, mxconst::FONT_PIXEL_13);
    fontName = lmbda_eval_font_name_and_return_existing_one(fontName, MxUICore::mxDefaultFontName);
    MxUICore::mapFontsMeta[fontName].initFontMeta(fontSizePx, mxconst::get_TEXT_TYPE_MSG_POPUP());

    vecTemp = missionx::ListDir::readFontMetadata(mxconst::get_TEXT_TYPE_TITLE_TOOLBAR() + "=", 1, cn_default_font_expr, defaultMetaData);
    lmbda_eval_font_data(vecTemp, fontName, fontSizePx, MxUICore::mxDefaultFontName, mxconst::FONT_PIXEL_18);
    fontName = lmbda_eval_font_name_and_return_existing_one(fontName, MxUICore::mxDefaultFontName);
    MxUICore::mapFontsMeta[fontName].initFontMeta(fontSizePx, mxconst::get_TEXT_TYPE_TITLE_TOOLBAR());

    vecTemp = missionx::ListDir::readFontMetadata(mxconst::get_TEXT_TYPE_TITLE_SMALL() + "=", 1, cn_default_font_expr, defaultMetaData);
    lmbda_eval_font_data(vecTemp, fontName, fontSizePx, MxUICore::mxDefaultFontName, mxconst::FONT_PIXEL_14);
    fontName = lmbda_eval_font_name_and_return_existing_one(fontName, MxUICore::mxDefaultFontName);
    MxUICore::mapFontsMeta[fontName].initFontMeta(fontSizePx, mxconst::get_TEXT_TYPE_TITLE_SMALL());

    vecTemp = missionx::ListDir::readFontMetadata(mxconst::get_TEXT_TYPE_TITLE_SMALLEST() + "=", 1, cn_default_font_expr, defaultMetaData);
    lmbda_eval_font_data(vecTemp, fontName, fontSizePx, MxUICore::mxDefaultFontName, mxconst::FONT_PIXEL_13);
    fontName = lmbda_eval_font_name_and_return_existing_one(fontName, MxUICore::mxDefaultFontName);
    MxUICore::mapFontsMeta[fontName].initFontMeta(fontSizePx, mxconst::get_TEXT_TYPE_TITLE_SMALLEST());


    if (auto fnt_default = ImgWindow::sFont1->AddFontFromFileTTF(MxUICore::mapFontsMeta[MxUICore::mxDefaultFontName].fontLocation_s.c_str(), mxconst::FONT_PIXEL_13))
    {
      MxUICore::mapFontsMeta[MxUICore::mxDefaultFontName].setFontID(seq, mxconst::get_TEXT_TYPE_DEFAULT());
      seq++;

      ImgWindow::sFont1->AddFontFromFileTTF(MxUICore::mapFontsMeta[MxUICore::mxDefaultFontName].fontLocation_s.c_str(), mxconst::FONT_PIXEL_13 + 1);
      MxUICore::mapFontsMeta[MxUICore::mxDefaultFontName].setFontID(seq, mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1());
      seq++;
    }

    // create the Atlas from all the fonts and their sizes
    for (auto& meta : MxUICore::mapFontsMeta | std::views::values)
    {
      // v3.305.3 loop over map_metaData
      for (auto& [fontType, fontMeta] : meta.mapMetaData)
      {
        if ((fontType == mxconst::get_TEXT_TYPE_DEFAULT()) || (fontType == mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1()))
          continue;

        if (auto fnt = ImgWindow::sFont1->AddFontFromFileTTF(meta.fontLocation_s.c_str(), fontMeta.fSizePx))
        {
          if (fontType == mxconst::get_TEXT_TYPE_TITLE_REG())
            add_icons_to_fonts(ImgWindow::sFont1, fontMeta.fSizePx);

          meta.setFontID(seq, fontType);
          //fontMeta.id = seq;
          seq++;
        }
      }

    } // end loop over all font settings and creating the Atlas
  }
  else
  { // initialize internal font

    constexpr auto imgui_internal_font        = "ProggyClean.ttf";
    MxUICore::mxDefaultFontName               = imgui_internal_font;
    sTitleRegFontName                         = MxUICore::mxDefaultFontName;

    seq = 0;

    // Prepare Meta Font data with all supported Text Types and sizes
    fontName   = MxUICore::mxDefaultFontName;                                                         // force default font name
    fontSizePx = mxconst::FONT_PIXEL_13;
    MxUICore::mapFontsMeta[fontName].initFontMeta(mxconst::FONT_PIXEL_13, mxconst::get_TEXT_TYPE_DEFAULT());            // init DEFAULT FONT
    MxUICore::mapFontsMeta[fontName].initFontMeta(mxconst::FONT_PIXEL_14, mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1());
    MxUICore::mapFontsMeta[fontName].initFontMeta(mxconst::FONT_PIXEL_15, mxconst::get_TEXT_TYPE_TITLE_REG());
    MxUICore::mapFontsMeta[fontName].initFontMeta(mxconst::FONT_PIXEL_18, mxconst::get_TEXT_TYPE_TEXT_MED());
    MxUICore::mapFontsMeta[fontName].initFontMeta(mxconst::FONT_PIXEL_14, mxconst::get_TEXT_TYPE_TEXT_REG());
    MxUICore::mapFontsMeta[fontName].initFontMeta(mxconst::FONT_PIXEL_14, mxconst::get_TEXT_TYPE_TITLE_SMALL());
    MxUICore::mapFontsMeta[fontName].initFontMeta(mxconst::FONT_PIXEL_14, mxconst::get_TEXT_TYPE_TITLE_SMALLEST());
    MxUICore::mapFontsMeta[fontName].initFontMeta(mxconst::FONT_PIXEL_18, mxconst::get_TEXT_TYPE_TITLE_MED());
    MxUICore::mapFontsMeta[fontName].initFontMeta(mxconst::FONT_PIXEL_24, mxconst::get_TEXT_TYPE_TITLE_BIG());
    MxUICore::mapFontsMeta[fontName].initFontMeta(mxconst::FONT_PIXEL_24, mxconst::get_TEXT_TYPE_TITLE_TOOLBAR());
    MxUICore::mapFontsMeta[fontName].initFontMeta(mxconst::FONT_PIXEL_14, mxconst::get_TEXT_TYPE_MSG_BOTTOM());
    MxUICore::mapFontsMeta[fontName].initFontMeta(mxconst::FONT_PIXEL_14, mxconst::get_TEXT_TYPE_MSG_POPUP());



    ImFontConfig font_cfg = ImFontConfig();
    font_cfg.SizePixels   = mxconst::FONT_PIXEL_13;

    // Explicitly create the default font
    ImgWindow::sFont1->AddFontDefault(&font_cfg);
    MxUICore::mapFontsMeta[fontName].setFontID(seq, mxconst::get_TEXT_TYPE_DEFAULT());
    //add_icons_to_fonts(ImgWindow::sFont1, font_cfg.SizePixels); // v3.305.3 deprecated we add this to mxconst::get_TEXT_TYPE_TITLE_REG() and pixel 13
    seq++;


    // create the rest of the Atlas from all the fonts and their sizes
    for (auto& meta : MxUICore::mapFontsMeta | std::views::values)
    {
      // v3.305.3 loop over map_metaData
      for (auto& [fontType, fontMeta] : meta.mapMetaData)
      {
        if ((fontType == mxconst::get_TEXT_TYPE_DEFAULT()) || (fontType == mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1()))
          continue;

        if (auto fnt = ImgWindow::sFont1->AddFontFromFileTTF(meta.fontLocation_s.c_str(), fontMeta.fSizePx))
        {
          if (fontType == mxconst::get_TEXT_TYPE_TITLE_REG())
            add_icons_to_fonts(ImgWindow::sFont1, fontMeta.fSizePx);

          meta.setFontID(seq, fontType);
          //fontMeta.id = seq;
          seq++;
        }
      }

    } // end loop over all font settings and creating the Atlas

  } // end initializing internal font



} // configureImgWindow

// -------------------------------------

int
setCameraPosition(XPLMCameraPosition_t* outCameraPosition, int inIsLosingControl, void* inRefcon)
{

  /* Fill out the camera position info. */
  if (outCameraPosition && !inIsLosingControl)
  {
    outCameraPosition->x       = missionx::data_manager::mxCameraPosition.x;
    outCameraPosition->y       = missionx::data_manager::mxCameraPosition.y;
    outCameraPosition->z       = missionx::data_manager::mxCameraPosition.z;
    outCameraPosition->pitch   = missionx::data_manager::mxCameraPosition.pitch;
    outCameraPosition->heading = missionx::data_manager::mxCameraPosition.heading;
    outCameraPosition->roll    = missionx::data_manager::mxCameraPosition.roll;

    inIsLosingControl = missionx::data_manager::isLosingControl_i;
  }
  if (missionx::data_manager::isLosingControl_i == 0)  // return the opposite state of losControl value
    return 1;
  
  return 0;
} // setCameraPosition

}

// -------------------------------------

void
missionx::Mission::add_GPS_data (const int optionalPointIndex)
{
  // Reveal GPS next location if options is set correctly
  // Check in local leg, if there is none then
  // check in global GPS and pick the one that correlates to: Mission::flight_leg_progress_counter_i attribute
  //  bool bAddedGPS_info = false; // v3.0.254.10 not used
  const int  entries   = XPLMCountFMSEntries ();
  const int        index     = entries;
  IXMLNode   cNode_ptr = IXMLNode::emptyIXMLNode;
  NavAidInfo navInfo;

  // Get pointer to XML Node
  if (!data_manager::mapFlightLegs[data_manager::currentLegName].xGPS.isEmpty() &&
       data_manager::mapFlightLegs[data_manager::currentLegName].xGPS.nChildNode(mxconst::get_ELEMENT_POINT().c_str()) > 0) // if to reveal 1 by one then first check if there is GPS element in LEG if not then check global GPS
    cNode_ptr = data_manager::mapFlightLegs[data_manager::currentLegName].xGPS.getChildNode(mxconst::get_ELEMENT_POINT().c_str());
  else if (!data_manager::xmlGPS.isEmpty() && data_manager::xmlGPS.nChildNode(mxconst::get_ELEMENT_POINT().c_str()) >= this->flight_leg_progress_counter_i)
  {
    if (optionalPointIndex > -1)
      cNode_ptr = data_manager::xmlGPS.getChildNode(mxconst::get_ELEMENT_POINT().c_str(), optionalPointIndex);
    else
      cNode_ptr = data_manager::xmlGPS.getChildNode(mxconst::get_ELEMENT_POINT().c_str(), this->flight_leg_progress_counter_i);
  }


  if (!cNode_ptr.isEmpty())
  {
    auto              lat_f              = Utils::readNodeNumericAttrib<float> (cNode_ptr, mxconst::get_ATTRIB_LAT(), 0.0f);
    auto              lon_f              = Utils::readNodeNumericAttrib<float> (cNode_ptr, mxconst::get_ATTRIB_LONG(), 0.0f);
    const auto        elev_ft_f          = Utils::readNodeNumericAttrib<float> (cNode_ptr, mxconst::get_ATTRIB_ELEV_FT(), 0.0f); // v3.0.241.3
    const std::string icao               = Utils::readAttrib (cNode_ptr, mxconst::get_ELEMENT_ICAO(), "");
    const int         navType_fromNode_i = Utils::readNodeNumericAttrib<int> (cNode_ptr, mxconst::get_ATTRIB_NAV_TYPE(), xplm_Nav_Unknown); // v3.0.255.4

    // add ICAO if available
    bool flag_icao_is_valid = false; // will hold the icao string if it is valid one.
    if (!icao.empty())
    {
      // search XPLMNavRef
      XPLMNavRef nav_ref = XPLM_NAV_NOT_FOUND;

      if (navType_fromNode_i > xplm_Nav_Unknown)                                                // v3.0.255.4
        nav_ref = XPLMFindNavAid (nullptr, icao.c_str(), &lat_f, &lon_f, nullptr, navType_fromNode_i); // find ref based on point nav_type. We can add assert on navRef_fromNode_i
      else
        nav_ref = XPLMFindNavAid (nullptr, icao.c_str(), &lat_f, &lon_f, nullptr, xplm_Nav_Airport);

      if (nav_ref != XPLM_NAV_NOT_FOUND)
      {
        flag_icao_is_valid = true;
        XPLMGetNavAidInfo(nav_ref, &navInfo.navType, &navInfo.lat, &navInfo.lon, &navInfo.height_mt, &navInfo.freq, &navInfo.heading, navInfo.ID, navInfo.name, navInfo.inRegion);


        XPLMSetFMSEntryInfo(index, nav_ref, (int)navInfo.height_mt);
      }
    }

    const auto lmbda_get_elevation = [&]() {
      if (flag_icao_is_valid)
        return (int)navInfo.height_mt;

      return (int)(elev_ft_f * missionx::feet2meter);
    };

    const auto elev_mt_i = lmbda_get_elevation();

    if (!flag_icao_is_valid)
      XPLMSetFMSEntryLatLon(index, lat_f, lon_f, elev_mt_i); // v3.0.241.3

    XPLMSetDestinationFMSEntry(index); // set GPS/FMS active entry

    missionx::data_manager::write_fpln_to_external_folder(); // v3.0.253.8 we need to update the FPLN to reflect the new location, we will have to read the FPLN from GNS/GTN for example

  } // end cNode_ptr is valid

} // add_GPS_data

// -------------------------------------

missionx::Mission::Mission()
{
  const std::time_t end_time  = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  const std::string dAte      = mxUtils::rtrim(std::ctime(&end_time));
  const std::string header_s  = ">>>>>>>>>>>> Loading Mission-X (" + dAte + ") <<<<<<<<<<<<\n";
  const std::string version_s = "Mission-X Version: v" + std::string(PLUGIN_VER_MAJOR) + "." + std::string(PLUGIN_VER_MINOR) + "." + std::string(PLUGIN_REVISION) + "!!!!!!\n";
  data_manager::missionState = missionx::mx_mission_state_enum::mission_undefined;

  // v25.03.1 read preference file.
  missionx::data_manager::xMissionxPropertiesNode = missionx::system_actions::load_plugin_options(); // v3.309.1 switched to return the XML node
  system_actions::pluginSetupOptions.node         = missionx::data_manager::xMissionxPropertiesNode.getChildNode(mxconst::get_ELEMENT_SETUP().c_str()).deepCopy();

  // initialize Mission-X Log file
  const bool bCycleLogFiles = Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_OPT_CYCLE_LOG_FILES(), true); // v25.03.1
  Log::set_logFile( Utils::getPluginDirectoryWithSep(missionx::PLUGIN_DIR_NAME) + "missionx.log", bCycleLogFiles ); // v24.12.2

  // v3.0.0
  Log::logXPLMDebugString(header_s);  // v3.305.2 will write to both Log and missionx log files.
  Log::logXPLMDebugString(version_s); // v3.305.2 will write to both Log and missionx log files.

  // initialize external script manager
  if (missionx::data_manager::sm.init())
  {
    #ifdef APL
    data_manager::ext_bas_open        = missionx::data_manager::sm.ext_bas_open;
    data_manager::ext_sm_init_success = data_manager::sm.ext_sm_init_success;
    data_manager::bas                 = data_manager::sm.bas;
    #endif
    missionx::ext_script::register_my_functions(missionx::data_manager::sm.bas); // register external functions
  }
  
  missionMenuEntry = nullptr;
}


missionx::Mission::~Mission() = default;

// -------------------------------------


void
missionx::Mission::syncOptionsWithMenu() const
{
  // set toggle designer mode menu
  int val_i = Utils::readNodeNumericAttrib<int>( missionx::system_actions::pluginSetupOptions.node,mxconst::get_OPT_ENABLE_DESIGNER_MODE(), false); // this property is not saved in pref file
  XPLMCheckMenuItem(this->missionMenuOptionsEntry, this->mx_menu.optionToggleDesignerMode, ((val_i == 1) ? xplm_Menu_Checked : xplm_Menu_Unchecked));

  // set toggle CueInfo menu
  val_i = Utils::readNodeNumericAttrib<int>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_DISPLAY_VISUAL_CUES(), false); // this property is not saved in pref file
  XPLMCheckMenuItem(this->missionMenuOptionsEntry, this->mx_menu.optionToggleCueInfo, ((val_i == 1) ? xplm_Menu_Checked : xplm_Menu_Unchecked));

  // set toggle AutoHide MxPad menu
  bool val_b = Utils::getNodeText_type_1_5<bool> (missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_HIDE_SHOW_MXPAD(), true);
  XPLMCheckMenuItem(this->missionMenuOptionsEntry, this->mx_menu.optionAutoHideMxPad, ((val_b) ? xplm_Menu_Checked : xplm_Menu_Unchecked));

  // Toggle disable cold and dark at start of mission // v3.0.221.10
  val_b = Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_DISABLE_PLUGIN_COLD_AND_DARK_WORKAROUND(), mxconst::DEFAULT_DISABLE_PLUGIN_COLD_AND_DARK);
  XPLMCheckMenuItem(this->missionMenuOptionsEntry, this->mx_menu.optionDisablePluginColdAndDark, ((val_b == false) ? xplm_Menu_Unchecked : xplm_Menu_Checked));

}


// -------------------------------------

void
missionx::Mission::init()
{
  bool                                  flag_called_rebuild_apt_dat{ false };
  std::string                           errMsg;
  missionx::data_manager                dm;

  // v25.03.1 moved to the Mission class constructor
  // missionx::data_manager::xMissionxPropertiesNode = missionx::system_actions::load_plugin_options(); // v3.309.1 switched to return the XML node
  // system_actions::pluginSetupOptions.node         = missionx::data_manager::xMissionxPropertiesNode.getChildNode(mxconst::get_ELEMENT_SETUP().c_str()).deepCopy();

  // read setup options
  const auto propPluginVersion_s = Utils::readAttrib (missionx::data_manager::xMissionxPropertiesNode, mxconst::get_ATTRIB_PLUGIN_VERSION(), ""); // v24025
  const int  mxVer_i             = Utils::readNodeNumericAttrib<int> (missionx::data_manager::xMissionxPropertiesNode, mxconst::get_ATTRIB_MXFEATURE(), 0);

  if ( missionx::PLUGIN_VERSION_S != propPluginVersion_s) // v24025
  {
    missionx::data_manager::xMissionxPropertiesNode.updateAttribute(missionx::PLUGIN_VERSION_S.c_str (), mxconst::get_ATTRIB_PLUGIN_VERSION().c_str(), mxconst::get_ATTRIB_PLUGIN_VERSION().c_str());
    missionx::system_actions::store_plugin_options();
    missionx::data_manager::set_flag_rebuild_apt_dat(true);
    flag_called_rebuild_apt_dat = true;
  }

  if (mxVer_i < missionx::MX_FEATURES_VERSION)
  {
    Log::logMsg(">>> Your feature version: " + mxUtils::formatNumber<int>(mxVer_i) + " is lower than expected - version: " + mxUtils::formatNumber<int>(missionx::MX_FEATURES_VERSION));

    if (!missionx::system_actions::pluginSetupOptions.node.isEmpty() && !missionx::data_manager::xMissionxPropertiesNode.isEmpty())
    {
      const std::string ver_s = mxUtils::formatNumber<int>(missionx::MX_FEATURES_VERSION);
      missionx::data_manager::xMissionxPropertiesNode.updateAttribute(ver_s.c_str(), mxconst::get_ATTRIB_MXFEATURE().c_str(), mxconst::get_ATTRIB_MXFEATURE().c_str()); // v3.0.255.4.2

      missionx::system_actions::pluginSetupOptions.node.updateAttribute(ver_s.c_str(), mxconst::get_ATTRIB_MXFEATURE().c_str(), mxconst::get_ATTRIB_MXFEATURE().c_str());

      // v3.303.14 remove FONT_LOCATION since we are using fonts.ini
      if (!system_actions::pluginSetupOptions.node.getChildNode(mxconst::SETUP_FONT_LOCATION).isEmpty()) // if we have <setup_font_location> element
        system_actions::pluginSetupOptions.node.getChildNode(mxconst::SETUP_FONT_LOCATION).deleteNodeContent();
      if (!system_actions::pluginSetupOptions.node.getChildNode(mxconst::SETUP_FONT_PIXEL_SIZE).isEmpty()) // if we have <setup_font_pixel_size> element
        system_actions::pluginSetupOptions.node.getChildNode(mxconst::SETUP_FONT_PIXEL_SIZE).deleteNodeContent();
      if (!system_actions::pluginSetupOptions.node.getChildNode(mxconst::SETUP_WRITE_CACHE_TO_DB).isEmpty()) // if we have <setup_font_pixel_size> element
        system_actions::pluginSetupOptions.node.getChildNode(mxconst::SETUP_WRITE_CACHE_TO_DB).deleteNodeContent();
      


      missionx::system_actions::store_plugin_options();
    }

    missionx::data_manager::set_flag_rebuild_apt_dat(true);
    flag_called_rebuild_apt_dat = true;
  }
  else
  {
    Log::logMsg(">>> Current plugins feature version is: " + mxUtils::formatNumber<int>(missionx::MX_FEATURES_VERSION));
  }

  // v3.303.12 store last XP version to decide if to re-read the apt-dat
  const int iProperty_xp_version = static_cast<int>(Utils::readNumericAttrib(missionx::data_manager::xMissionxPropertiesNode, mxconst::get_ATTRIB_XP_VERSION(), 0.0));
  if (data_manager::xplane_ver_i != iProperty_xp_version)
  {
    const std::string ver_s = mxUtils::formatNumber<int>(missionx::data_manager::xplane_ver_i);

    missionx::data_manager::xMissionxPropertiesNode.updateAttribute(ver_s.c_str(), mxconst::get_ATTRIB_XP_VERSION().c_str(), mxconst::get_ATTRIB_XP_VERSION().c_str());
    missionx::system_actions::pluginSetupOptions.node.updateAttribute(ver_s.c_str(), mxconst::get_ATTRIB_XP_VERSION().c_str(), mxconst::get_ATTRIB_XP_VERSION().c_str());
    missionx::system_actions::store_plugin_options();

    if (!flag_called_rebuild_apt_dat)
    {
      missionx::data_manager::set_flag_rebuild_apt_dat(true);
      Log::logMsg("[" + std::string(__func__) + "] x-plane version is different than in the property file. Will call rebuild_apt_dat() function."); // __FILE__
    }

  } // // v3.303.12


  // v3.0.255.4.2 lock overpass url flag
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_LOCK_OVERPASS_URL_TO_USER_PICK()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_SETUP_LOCK_OVERPASS_URL_TO_USER_PICK(), false);
    missionx::system_actions::store_plugin_options();
  }


  // v3.0.255.4.1 store overpass url in data manager
  if (!missionx::data_manager::xMissionxPropertiesNode.isEmpty() && !missionx::data_manager::xMissionxPropertiesNode.getChildNode(mxconst::get_ELEMENT_OVERPASS().c_str()).isEmpty())
  {
    missionx::data_manager::vecOverpassUrls.clear();
    for (int i1 = 0; i1 < missionx::data_manager::xMissionxPropertiesNode.getChildNode(mxconst::get_ELEMENT_OVERPASS().c_str()).nChildNode(mxconst::get_ELEMENT_URL().c_str()); ++i1)
    {
      IXMLNode          xUrl = missionx::data_manager::xMissionxPropertiesNode.getChildNode(mxconst::get_ELEMENT_OVERPASS().c_str()).getChildNode(mxconst::get_ELEMENT_URL().c_str(), i1);
      const std::string text = xUrl.getText();
      if (!text.empty())
        data_manager::vecOverpassUrls.emplace_back(text);
    }
  }
  if (data_manager::vecOverpassUrls.empty())
  {
    missionx::data_manager::vecOverpassUrls = mxUtils::split_v2(
      "https://lz4.overpass-api.de/api/interpreter,https://z.overpass-api.de/api/interpreter,https://overpass.openstreetmap.ru/api/interpreter,https://overpass.openstreetmap.fr/api/interpreter,https://overpass.kumi.systems/api/interpreter",
      ",");
  }



  // v3.0.215.1
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_HIDE_SHOW_MXPAD()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_AUTO_HIDE_SHOW_MXPAD(), true);
    missionx::system_actions::store_plugin_options();
  }


  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_DISPLAY_VISUAL_CUES()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<int>(mxconst::get_OPT_DISPLAY_VISUAL_CUES(), (int)0);
    missionx::system_actions::store_plugin_options();
  }


  // v3.0.221.6
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_PAUSE_IN_VR()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_AUTO_PAUSE_IN_VR(), mxconst::DEFAULT_AUTO_PAUSE_IN_VR);
    missionx::system_actions::store_plugin_options();
  }


  // v3.0.221.7
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_DISPLAY_MISSIONX_IN_VR()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_DISPLAY_MISSIONX_IN_VR(), mxconst::DEFAULT_DISPLAY_MISSIONX_IN_VR);
    missionx::system_actions::store_plugin_options();
  }


  // v3.0.221.10
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_DISABLE_PLUGIN_COLD_AND_DARK_WORKAROUND()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_DISABLE_PLUGIN_COLD_AND_DARK_WORKAROUND(), mxconst::DEFAULT_DISABLE_PLUGIN_COLD_AND_DARK);
    missionx::system_actions::store_plugin_options();
  }


  // v3.0.241.2
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_WRITE_CONVERTED_FPLN_TO_XPLANE_FOLDER()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_WRITE_CONVERTED_FPLN_TO_XPLANE_FOLDER(), mxconst::DEFAULT_WRITE_CONVERTED_FMS_TO_XPLANE_FOLDER);
    missionx::system_actions::store_plugin_options();
  }


  // v3.0.241.7
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_SETUP_DISPLAY_TARGET_MARKERS(), true);
    missionx::system_actions::store_plugin_options();
  }


  // v3.0.251.1
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_SLIDER_FONT_SCALE_SIZE()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<double>(mxconst::get_SETUP_SLIDER_FONT_SCALE_SIZE(), (double)mxconst::DEFAULT_BASE_FONT_SCALE);
    missionx::system_actions::store_plugin_options();
  }


  // v3.0.253.4 overpass (osm) filter
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_FILTER()).isEmpty())
  {
    Utils::xml_search_and_set_node_text(system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_FILTER(), mxconst::get_DEFAULT_OVERPASS_WAYS_FILTER(), mxUtils::formatNumber<int>((int) missionx::mx_property_type::MX_STRING), true); // "6" = string type
    missionx::system_actions::store_plugin_options();
  }


  // v3.0.253.6 overpass (osm) url
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_URL()).isEmpty())
  {
    Utils::xml_search_and_set_node_text(system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_URL(), "", mxUtils::formatNumber<int>((int)missionx::mx_property_type::MX_STRING), true); // "6" = string type
    missionx::system_actions::store_plugin_options();
  }


  // v3.0.253.7 OPT_GPS_IMMEDIATE_EXPOSURE
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE(), true);
    missionx::system_actions::store_plugin_options();
  }


  // v3.0.253.7 missing SETUP_DISPLAY_TARGET_MARKERS_AWAY_FROM_TARGET
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS_AWAY_FROM_TARGET()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_SETUP_DISPLAY_TARGET_MARKERS_AWAY_FROM_TARGET(), false);
    missionx::system_actions::store_plugin_options();
  }


  // v3.0.253.9.1
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_PAUSE_IN_2D()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_AUTO_PAUSE_IN_2D(), mxconst::DEFAULT_AUTO_PAUSE_IN_2D);
    missionx::system_actions::store_plugin_options();
  }


  // v3.0.303.6
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_NORMALIZED_VOLUME()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<int>(mxconst::get_SETUP_NORMALIZED_VOLUME(), mxconst::DEFAULT_SETUP_MISSION_VOLUME_I);
    missionx::system_actions::store_plugin_options();
  }

  // v3.0.303.6
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_NORMALIZE_VOLUME_B()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_SETUP_NORMALIZE_VOLUME_B(), false);
    missionx::system_actions::store_plugin_options();
  }


  // v3.303.8.3 Authorization Key   
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_AUTHORIZATIOJN_KEY()).isEmpty())
  {
    Utils::xml_search_and_set_node_text(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_AUTHORIZATIOJN_KEY(), "", mxUtils::formatNumber<int>((int)missionx::mx_property_type::MX_STRING), true);
    missionx::system_actions::store_plugin_options();
  }
  

  // v3.303.9.1 make sure <scoring> is part of properties
  if (missionx::data_manager::xMissionxPropertiesNode.getChildNode(mxconst::get_ELEMENT_SCORING().c_str()).isEmpty()) // rebuild scoring too
  {
    if (auto node_ptr = missionx::data_manager::xMissionxPropertiesNode.getChildNode(mxconst::get_ELEMENT_SCORING().c_str()); node_ptr.isEmpty())
    {
      const auto newNode = Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_ELEMENT_SCORING());
      assert( (! newNode.isEmpty() ) && "There is no <scoring> element in Utils::XSD");

      node_ptr = missionx::data_manager::xMissionxPropertiesNode.addChild(newNode.deepCopy());
      missionx::system_actions::store_plugin_options();
    }
  }

  // v24.12.2 Inventory layout preference. XP11 or XP12 (XP12 = with stations support)
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::SETUP_USE_XP11_INV_LAYOUT).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::SETUP_USE_XP11_INV_LAYOUT, (missionx::data_manager::xplane_ver_i < missionx::XP12_VERSION_NO));
    missionx::system_actions::store_plugin_options();
  }

  // v25.03.1 Keep only one "missionx.log" file and don't cycle.
  if (Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_CYCLE_LOG_FILES()).isEmpty())
  {
    missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_OPT_CYCLE_LOG_FILES(), true );
    missionx::system_actions::store_plugin_options();
  }


  // initialize core attributes
  data_manager::missionState = mx_mission_state_enum::mission_undefined;
  Mission::isMissionValid    = false;
  Mission::currentObjectiveName.clear();
  Mission::vecGoalsExecOrder.clear();

  Log::logMsg("XP Version: " + mxUtils::formatNumber<int>(data_manager::xplane_ver_i));

  // metar
  usingCustomMetarFile = false;

#ifdef APL
  // This is a stupid workaround to the static value that were not persistent on MAC
  if (!missionx::data_manager::sm.bas)
  {
    missionx::data_manager::sm.bas                 = data_manager::bas;
    missionx::data_manager::sm.ext_sm_init_success = data_manager::ext_sm_init_success;
    missionx::data_manager::sm.ext_bas_open        = data_manager::ext_bas_open;
  }

#endif


  missionx::configureImgWindow(); // Prepare IMGUI Font Atlas
}

// -------------------------------------

void
missionx::Mission::initImguiParametersAtPluginsStart()
{
  // This function will be called after Mission::init() and at the end of plugin::START_MISSION()
  // initialize first time IMGUI settings
  if (Mission::uiImGuiBriefer != nullptr)
  {
    // init setup font scale = should be moved into a function inside Mission class
    if (!Utils::xml_get_node_from_node_tree_IXMLNode(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_SLIDER_FONT_SCALE_SIZE()).isEmpty())
    {

      float fScale = (float)Utils::getNodeText_type_1_5<double>(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_SLIDER_FONT_SCALE_SIZE(), (double)mxconst::DEFAULT_BASE_FONT_SCALE); // default scale size
      if (fScale < missionx::Mission::uiImGuiBriefer->strct_setup_layer.fFontMinScaleSize || fScale > missionx::Mission::uiImGuiBriefer->strct_setup_layer.fFontMaxScaleSize)
        fScale = mxconst::DEFAULT_BASE_FONT_SCALE; // default size = no change in pixel scale

      Mission::uiImGuiBriefer->strct_setup_layer.fPreferredFontScale = fScale;
    }

    Mission::uiImGuiBriefer->strct_setup_layer.bPlaceMarkersAwayFromTarget =
      Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS_AWAY_FROM_TARGET(), false); // display target away from target

    Mission::uiImGuiBriefer->set_vecOverpassUrls_char(missionx::data_manager::vecOverpassUrls); // v3.0.255.4.1 initialize overpass url from conf file

    // v3.303.8.3 add authorization key to the Briefer screen  
    std::string authKey_s = Utils::getNodeText_type_6(system_actions::pluginSetupOptions.node, mxconst::get_SETUP_AUTHORIZATIOJN_KEY(), "");
    std::memcpy(this->uiImGuiBriefer->strct_ext_layer.buf_authorization, authKey_s.substr(0, 255).c_str(), 255); // copy no more than 255 characters because our buffer is 256 in size
    

  } // end if uiImGuiBriefer was init


} // initImguiParametersAtPluginsStart

// -------------------------------------


void
missionx::Mission::prepareMissionBrieferInfo()
{
  const std::string missionx_mission_folder = data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_FLD_MISSIONS_ROOT_PATH(), "", data_manager::errStr); // we can also use data_manager::getMissionsRootPath()
  missionx::ListDir::readMissionsBrieferInfo(missionx_mission_folder, data_manager::mapBrieferMissionList, data_manager::mapBrieferMissionListLocator);

  //// Move Random to the beginning of the list, if it exists
  if (Utils::isElementExists(data_manager::mapBrieferMissionList, mxconst::get_RANDOM_MISSION_DATA_FILE_NAME()))
  {
    // find random sequence image number
    int randomSeq = -1;
    for (auto& img : data_manager::mapBrieferMissionListLocator)
    {
      if (mxconst::get_RANDOM_MISSION_DATA_FILE_NAME().compare(img.second) == 0)
      {
        randomSeq = img.first;
        break;
      }
    }

    if (randomSeq > 1) // if "random.xml" file is not already first
    {
      // switch with the first file
      if (Utils::isElementExists(data_manager::mapBrieferMissionListLocator, 1))
      {
        std::string prevFileName   = mxconst::get_RANDOM_MISSION_DATA_FILE_NAME(); 
        std::string storedFileName = "";

        for (auto& [seq, fileName] : data_manager::mapBrieferMissionListLocator) // push all files so random will switch with first file
        {
          storedFileName                                = data_manager::mapBrieferMissionListLocator[seq];
          data_manager::mapBrieferMissionListLocator[seq] = prevFileName;


          if (seq > 1 && (mxconst::get_RANDOM_MISSION_DATA_FILE_NAME().compare(storedFileName) == 0))
            break;
          else 
            prevFileName = storedFileName;
         
        }

      }
    }
  }
}

// -------------------------------------

void
missionx::Mission::MissionMenuHandler(void* inMenuRef, void* inItemRef)
{
  // Main Menu Dispatcher that calls the function that creates each Widget
  std::string err;
  err.clear();

  switch ((Mission::mx_menuIdRefs)((intptr_t)inItemRef))
  {

    case Mission::mx_menuIdRefs::MENU_OPEN_LIST_OF_MISSIONS:
    {
      missionx::Log::logMsg("[Missionx] Displaying Mission-X Window.");
      this->prepareMissionBrieferInfo(); // v3.0.241.10 b3 refine
    }
    break;
    case Mission::mx_menuIdRefs::MENU_START_MISSION:
      // load dummy mission for tests
      {
        missionx::Log::logMsg("[Missionx] Pressed Start Mission.");
        Mission::START_MISSION();
      }
      break;
    case Mission::mx_menuIdRefs::MENU_STOP_MISSION:
      // stop mission
      {
        missionx::Log::logMsg("[Missionx] Pressed Stop Mission.");
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::stop_mission);
      }
      break;
    case Mission::mx_menuIdRefs::MENU_CREATE_SAVEPOINT:
      // Create Savepoint
      {
        missionx::Log::logMsg("[Missionx] Creating Checkpoint.");
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::create_savepoint); // v3.0.231.1 consolidate actions and code in flcPRE() for both save checkpoint button and menu
      }
      break;
    case Mission::mx_menuIdRefs::MENU_LOAD_SAVEPOINT:
      // load savepoint
      {

        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::load_savepoint); // v3.0.231.1 consolidate actions and code in flcPRE() for both load checkpoint button and menu
      }
      break;
    default:
      break;

  } // end switch

}


// -------------------------------------

bool
missionx::Mission::prepareMission(std::string& outError)
{
  bool        flag_isOK = true;
  std::string missionStartingFlightLeg;
  std::string err;
  outError.clear();
  missionStartingFlightLeg.clear();
  err.clear();

  missionx::data_manager::flag_setupEnableDesignerMode  = Utils::readBoolAttrib(data_manager::xMainNode, mxconst::get_ATTRIB_MISSION_DESIGNER_MODE(), missionx::data_manager::flag_setupEnableDesignerMode);          // v3.0.241.1

  // v3.0.156
  // get starting flightLeg name from Briefer or override global settings
  if (const std::string overrideLegName = Utils::readAttrib(data_manager::mx_global_settings.xDesigner_ptr, mxconst::get_ATTRIB_FORCE_LEG_NAME(), "");
      missionx::data_manager::flag_setupEnableDesignerMode && !overrideLegName.empty())
    missionStartingFlightLeg = overrideLegName;
  else if (data_manager::missionState == missionx::mx_mission_state_enum::mission_loaded_from_savepoint) // v3.0.219.7 fix bug after loading from checkpoint. data_manager::currentLegName was already set by the "load Check point" function,
                                                                                                         // therefore the missionStartGoal should be the same
    missionStartingFlightLeg = data_manager::currentLegName;
  else
    missionStartingFlightLeg = Utils::readAttrib(missionx::data_manager::briefer.node, mxconst::get_ATTRIB_STARTING_LEG(), ""); // v3.0.241.1  //   missionx::data_manager::briefer.getPropertyValue(mxconst::get_ATTRIB_STARTING_LEG(), err);

  if (missionStartingFlightLeg.empty() || !(Utils::isElementExists(data_manager::mapFlightLegs, missionStartingFlightLeg)))
  {
    flag_isOK = false;
    outError += "Flight Leg name is not valid or not in the Mission file. Check mission data file.";
  }
  else // fix bug when loading from checkpoint and redoing same staff
  {
    data_manager::currentLegName = missionStartingFlightLeg;

    // read all images related to mission including 2D Maps from all flight legs
    Mission::readCurrentMissionTextures();

  } // end if Goal name is valid.

  // initialize sound
  if (!QueueMessageManager::sound.wasSoundInitSuccess)
    missionx::QueueMessageManager::sound.initSound();

  missionx::QueueMessageManager::sound.setSoundFilePath(data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_ATTRIB_SOUND_FOLDER_NAME(), "", err));

  return flag_isOK;
}

// -------------------------------------

void
missionx::Mission::START_MISSION()
{
  assert(Mission::uiImGuiBriefer && "uiImGuiBriefer was not initialized correctly.");

  Log::logMsg(" ============================================================ ");
  Log::logMsg(" ============================================================ ");
  Log::logMsg(" ============================================================ ");
  Log::printHeaderToLog(fmt::format("===> STARTING MISSION: {} <===", missionx::data_manager::selectedMissionKey));

  Utils::seqTimerFunc = 0; // v3.305.2

  std::string err;
  err.clear();

  QueueMessageManager::listPoolMsg.clear();
  QueueMessageManager::listPadQueueMessages.clear();

  const std::string mission_state_s = Utils::readAttrib(data_manager::mx_global_settings.node, mxconst::get_PROP_MISSION_STATE(), ""); // empty value means new mission and not loaded checkpoint

  uiImGuiBriefer->clearMessage(); // clear any message when starting a mission
  
  auto n = missionx::data_manager::mapFlightLegs.size(); // DEBUG

  //  Consider adding call external script - mission.postStartMission() function
  if (Mission::prepareMission(err))
  {
    // station parsing if we are NOT in xp11 compatibility mode
    if (missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i != missionx::XP11_COMPATIBILITY)
    {
      char outFileName[512]{ 0 };
      char outPathAndFile[2048]{ 0 };
      XPLMGetNthAircraftModel ( XPLM_USER_AIRCRAFT, outFileName, outPathAndFile ); // we will only return the file name

      if (data_manager::missionState != missionx::mx_mission_state_enum::mission_loaded_from_savepoint) // mission_loaded_from_the_original_file
      {
        missionx::data_manager::set_active_acf_and_gather_info (outFileName);
      }
      else
      { // Loaded from savepoint
        missionx::data_manager::set_acf (outFileName);
      }
    }
    data_manager::dref_acf_station_max_kgs_f_arr.setAndInitializeKey("sim/aircraft/weight/acf_m_station_max");
    data_manager::dref_m_stations_kgs_f_arr.setAndInitializeKey("sim/flightmodel/weight/m_stations");
    // end v24.12.2

    Mission::isMissionValid = true;
    Mission::vecGoalsExecOrder.clear();

    missionx::data_manager::releaseMessageStoryCachedTextures();

    // read all 3D Object files and create reference. Later will be used in instancing
    data_manager::loadAll3dObjectFiles(); // v3.0.200

    Mission::initFlightLegDisplayObjects(); // v3.0.200


    Mission::uiImGuiBriefer->strct_flight_leg_info.strct_story_mode.reset(); // v3.305.1
    Mission::uiImGuiBriefer->initFlightLegChange(); // v3.0.201

    Mission::uiImGuiBriefer->setLayer(missionx::uiLayer_enum::flight_leg_info); // set flightLeg layer to be display


    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::position_plane); // Position plane. Use action in Queue. Should position plane


    // display mxpad
    if (Mission::uiImGuiMxpad) // v3.0.251.1
    {
      Mission::uiImGuiMxpad->resetMxpadWindowPosition();
      if (missionx::mxvr::vr_display_missionx_in_vr_mode)
        Mission::uiImGuiMxpad->execAction(missionx::mx_window_actions::ACTION_HIDE_WINDOW);
      else if (!Mission::uiImGuiMxpad->GetVisible())
        Mission::uiImGuiMxpad->execAction(missionx::mx_window_actions::ACTION_TOGGLE_WINDOW);
    }


    // prepare Cue points
    data_manager::resetCueSettings();
    missionx::data_manager::flc_cue(missionx::cue_actions_enum::cue_action_first_time); // v3.0.202a

    // set designer mode from mission file
    // v3.0.203.5
    missionx::data_manager::flag_setupEnableDesignerMode = Utils::readBoolAttrib(data_manager::xMainNode, mxconst::get_ATTRIB_MISSION_DESIGNER_MODE(), missionx::data_manager::flag_setupEnableDesignerMode);                                                            // v3.0.241.1
    missionx::system_actions::pluginSetupOptions.node.updateAttribute(mxUtils::formatNumber<int>(((missionx::data_manager::flag_setupEnableDesignerMode) ? 1 : 0)).c_str(), mxconst::get_OPT_ENABLE_DESIGNER_MODE().c_str(), mxconst::get_OPT_ENABLE_DESIGNER_MODE().c_str()); // v3.303.8.3



    // v3.0.213.3 // v25.02.1 - one liner
    const bool bStoreWeight = ( data_manager::mx_global_settings.xBaseWeight_ptr.isEmpty () )? false : true;

    // v24.12.2
    if (missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i != missionx::XP11_COMPATIBILITY)
    {
      std::string resetStations_s;
      for (int i = 0; i < data_manager::dref_m_stations_kgs_f_arr.arraySize; ++i)
      {
        resetStations_s.append("0");
        if (i < (data_manager::dref_m_stations_kgs_f_arr.arraySize - 1))
          resetStations_s.append(",");
      }
      int iResult = data_manager::dref_m_stations_kgs_f_arr.setTargetArray<xplmType_FloatArray, float>(data_manager::dref_m_stations_kgs_f_arr.arraySize, resetStations_s, true, ",");
    }

    data_manager::internally_calculateAndStorePlaneWeight(data_manager::mapInventories[mxconst::get_ELEMENT_PLANE()], bStoreWeight, missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i); // v3.303.14.2 added store wait flag to solve weight injection even if there is no weight element in the mission file

    // v3.0.215.1
    if (Mission::uiImGuiMxpad)
      Mission::uiImGuiMxpad->setWasHiddenByAutoHideOption(false);


    // then check global GPS
    if (Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE(), true) == false &&
        mission_state_s.empty()) // if to reveal 1 by one then first check if there is GPS element in LEG if not then check global GPS
    {
      data_manager::clearFMSEntries(); // v3.0.253.7

      if (((!data_manager::xmlGPS.isEmpty() && data_manager::xmlGPS.nChildNode(mxconst::get_ELEMENT_POINT().c_str()) > 1 && data_manager::mapFlightLegs[data_manager::currentLegName].xGPS.isEmpty()) ||
           (!data_manager::mapFlightLegs[data_manager::currentLegName].xGPS.isEmpty() && data_manager::mapFlightLegs[data_manager::currentLegName].xGPS.nChildNode(mxconst::get_ELEMENT_POINT().c_str()) == 0)))
      {
        this->add_GPS_data(0);
        this->add_GPS_data(1); // add the two first GPS locations from global GPS
      }
      else
        this->add_GPS_data();
    }
    else
    {
      missionx::data_manager::setGPS(); // set the whole GPS element into the FMS
    }

    missionx::data_manager::write_fpln_to_external_folder(); // v3.0.241.2

    // v24.12.2 if-init-statement cLion
    if (const bool val_pause_in_2d = Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_PAUSE_IN_2D(), mxconst::DEFAULT_AUTO_PAUSE_IN_2D); missionx::mxvr::vr_display_missionx_in_vr_mode || Mission::uiImGuiBriefer->IsPoppedOut() || (missionx::mxvr::vr_display_missionx_in_vr_mode == false && val_pause_in_2d == false)) // v3.0.253.9.1 do not hide briefer in 2D mode always
      Mission::uiImGuiBriefer->execAction(missionx::mx_window_actions::ACTION_SHOW_WINDOW);                                                              // keep window open. Do not hide it
    else
      Mission::uiImGuiBriefer->execAction(missionx::mx_window_actions::ACTION_HIDE_WINDOW); // fix bug where window was not displayed but mouse did not register the right click actions.
  }
  else
  {
    Log::logMsgErr(err);
    Mission::uiImGuiBriefer->setMessage(err); // v3.0.251.1
    data_manager::missionState = missionx::mx_mission_state_enum::mission_undefined;
  }

  // v3.0.215.1
  this->syncOptionsWithMenu();

  if (mission_state_s.empty()) // reset flight_leg_progress if we did not read it from checkpoint determine in "mission::applyPropertiesToLocal()" function
  {
    this->flight_leg_progress_counter_i = 0;
    missionx::data_manager::mxThreeStoppers.resetAllTimers(); // v3.303.13 reset stoppers if it is a new mission
  }

  // v3.0.221.7 set the shared datarefs
  missionx::data_manager::setSharedDatarefData();

  if (this->flight_leg_progress_counter_i == 0 && !data_manager::mx_global_settings.node.isEmpty()) // new mission ? or use missionx::data_manager::mx_global_settings.flag_loadedFromSavepoint 
  {
    // v3.0.253.7 Fail Timer from global settings
    if (!data_manager::mx_global_settings.xTimer_ptr.isEmpty())
    {
      missionx::Timer failTimer;
      failTimer.node = data_manager::mx_global_settings.xTimer_ptr.deepCopy();
      if (failTimer.parse_node())
      {
        Utils::addElementToMap(missionx::data_manager::mapFailureTimers, failTimer.getName(), failTimer);
      }
    }
  }

  // v3.0.231.1 set choice window
  const std::string active_choice_name = Utils::readAttrib(data_manager::xmlChoices, mxconst::get_ATTRIB_ACTIVE_CHOICE(), "");
  if (!active_choice_name.empty())
  {
    missionx::data_manager::prepare_choice_options(active_choice_name);
  }

  if (missionx::data_manager::missionState == missionx::mx_mission_state_enum::mission_loaded_from_savepoint) // missionx::data_manager::mx_global_settings.flag_loadedFromSavepoint
  {
    // v24.12.2 if-init-statement cLion
    if (const auto current_time_sec = Utils::readNodeNumericAttrib<float>( missionx::data_manager::mx_global_settings.node, mxconst::get_ATTRIB_LOCAL_TIME_SEC(), 0.0f); current_time_sec != 0.0f)
      XPLMSetDataf(drefConst.dref_local_time_sec_f, current_time_sec);

    if (!active_choice_name.empty()) // v3.305.3 force display choice window if is active
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::display_choice_window);


    // v3.305.3 re-init interpolation datarefs
    std::list<std::string> listEraseFromInterpolation{};
    for (auto& [key, dref] : missionx::data_manager::mapInterpolDatarefs)
    {
      dref.readDatarefValue_into_missionx_plugin();
      if (dref.initInterpolationDataFromSaveFile())
      {
        // init interpolation timer
        auto xTimer = dref.node.getChildNode(mxconst::get_ELEMENT_TIMER().c_str());
        bool bResetTimer = false;
        if (!xTimer.isEmpty())
        {
          dref.strctInterData.timerToRun.node = xTimer.deepCopy();
          if (dref.strctInterData.timerToRun.parse_node())
          {
            if (dref.strctInterData.timerToRun.getState() == mx_timer_state::timer_is_set)
            {
              if (dref.strctInterData.timerToRun.flag_loadedFromSavePoint)
                dref.strctInterData.timerToRun.start_timer_after_savepoint_load();
              else
                dref.strctInterData.timerToRun.start_timer_based_on_node();
            }
            else
              bResetTimer = true;
          }
          else
          {
            bResetTimer = true;
          }
        }
        else
        {
          bResetTimer = true;
        }

        if (bResetTimer)
        {
          dref.strctInterData.timerToRun.reset();
          missionx::Timer::start(dref.strctInterData.timerToRun, (float)dref.strctInterData.seconds_to_run_i);
        }


        //dref.strctInterData.timerToRun.reset();
        //missionx::Timer::start(dref.strctInterData.timerToRun, dref.strctInterData.seconds_to_run_i);
        //dref.strctInterData.timerToRun.setSecondsPassedForInterpolationInit( dref.getAttribNumericValue<float>(mxconst::PROP_SECONDS_PASSED_F, 0.0f ) );
        //dref.strctInterData.timerToRun.start_timer_based_on_node(); // we use this function since we init the timepassed variable and we want it to be in the init settings.
      }
      else
        listEraseFromInterpolation.emplace_back(key); // add to delete container
    }
    // delete from interpolation container
    std::for_each(listEraseFromInterpolation.begin(), listEraseFromInterpolation.end(), [](const auto& k) { missionx::data_manager::mapInterpolDatarefs.erase(k); });

  }
  else
  {
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::set_time); // v3.0219.7 Set Mission Time
    missionx::data_manager::timelapse.flag_isActive = true;                             // v3.303.8
  }


  // v3.0.241.3 try to handle pause cases after starting a mission (not after loading a mission)
  if (Utils::readBoolAttrib(data_manager::briefer.xLocationAdjust, mxconst::get_ATTRIB_PAUSE_AFTER_LOCATION_ADJUST(), false))
  {
    missionx::data_manager::postFlcActions.push(missionx::mx_flc_pre_command::pause_xplane); // place action in Queue.

    Mission::uiImGuiBriefer->setPluginPausedSim(false); // this is done so uiImGuiBriefer::flc() won't un-pause the sim if we asked to pause it in the briefer, after start new mission or load mission.
  }

  // initialize stats DB // v3.303.8.3 fixed stats db won't be initialized if 
  if (data_manager::init_stats_db())
  {
    
    data_manager::gather_stats.setDb(&data_manager::db_stats, mission_state_s);
    this->plane_stats.reset();    
    if ( ! mission_state_s.empty() ) // if loaded from checkpoint
      data_manager::gather_stats.init_seq_from_checkpoint_stats();
    
  }
  else
  {
    Log::logMsg("!!! Failed initializing stats db. Quit the mission, delete the file and try again.");
  }

  Mission::uiImGuiBriefer->strct_flight_leg_info.bStatsPressed = false; // force showing image at the end of mission

  // v3.303.9.1
  missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::gather_acf_custom_datarefs); // v3.303.9.1

  // v3.305.2
  this->timerOptLegTriggersTimer.reset();

#ifdef DISPLAY_3D_INSTANCE
  Log::logMsg("<<<< Display 3D Instances Info >>>>\n=============================\n");
  for (auto inst : data_manager::map3dInstances)
  {
    Log::logMsg(data_manager::map3dInstances[inst.first].to_string());
  }
  Log::logMsg("<<<< END Display 3D Instances Info >>>>\n");
#endif

}

// -------------------------------------

void
missionx::Mission::readCurrentMissionTextures()
{
  std::string errorMsg;
  const std::string fldMissionCustom_withSep = data_manager::mapBrieferMissionList[missionx::data_manager::selectedMissionKey].pathToMissionPackFolderInCustomScenery + XPLMGetDirectorySeparator();

  XPLMSetGraphicsState(0 /*Fog*/, 1 /*TexUnits*/, 0 /*Lighting*/, 0 /*AlphaTesting*/, 1 /*AlphaBlending*/, 0 /*DepthTesting*/, 0 /*DepthWriting*/);
  for (const auto &[fileName, filePath] : data_manager::mapCurrentMissionTexturePathLocator)
  {
    mxTextureFile texture;
    texture.setTextureFile(fileName, filePath);

    if (missionx::BitmapReader::loadGLTexture(texture, errorMsg, false)) // load image but do not flip it
    {
      Utils::addElementToMap(data_manager::mapCurrentMissionTextures, fileName, texture); // v3.0.211.2 store texture data in map

      Log::logMsg("Loaded Mission Texture: " + texture.getAbsoluteFileLocation()); // debug
    }
  }

  //// read 2D Maps
  if (Utils::isElementExists(data_manager::mapBrieferMissionList, missionx::data_manager::selectedMissionKey))
  {

    // v3.0.241.7.1 FIX read ALL maps from all flight <leg>s
    for (const auto& [leg_name, legClass] : missionx::data_manager::mapFlightLegs) // v3.303.13
    {
      //const std::string leg_name = legNode.first;
      const int nChilds2 = legClass.node.nChildNode(mxconst::get_ELEMENT_MAP().c_str()); // v3.303.13 //  legNode.second.node.nChildNode(mxconst::get_ELEMENT_MAP().c_str());
      for (int i1 = 0; i1 < nChilds2; i1++)
      {
        IXMLNode xChild = legClass.node.getChildNode(mxconst::get_ELEMENT_MAP().c_str(), i1);
        if (!xChild.isEmpty())
        {
          const std::string file_name = Utils::readAttrib(xChild, mxconst::get_ATTRIB_MAP_FILE_NAME(), "");
          
          if (!file_name.empty() && !Utils::isElementExists(data_manager::mapCurrentMissionTextures, file_name)) // check if file name NOT in texture container
          {
            std::string errorMsg;
            mxTextureFile texture;
            texture.setTextureFile(file_name, fldMissionCustom_withSep);

            if (missionx::BitmapReader::loadGLTexture(texture, errorMsg, false)) // load image but do not flip it
            {
              Utils::addElementToMap(missionx::data_manager::mapFlightLegs[leg_name].map2DMapsNodes, file_name, xChild.deepCopy()); // v3.0.241.7.1
              Utils::addElementToMap(data_manager::mapCurrentMissionTextures, file_name, texture);                                  // store texture data in map

              Log::logMsg("Loaded 2D map for <leg>: " + leg_name + ", Texture: " + texture.getAbsoluteFileLocation()); // debug
            }
          }
          else if (!file_name.empty() && Utils::isElementExists(data_manager::mapCurrentMissionTextures, file_name))
          {
            Utils::addElementToMap(missionx::data_manager::mapFlightLegs[leg_name].map2DMapsNodes, file_name, xChild.deepCopy()); // v3.0.241.7.1                                 // store texture data in map

            Log::logMsg("Reusing 2D map for <leg>: " + leg_name + ", exists texture name: " + file_name ); // debug

          }
          else
            Log::logMsg("[leg: " + leg_name + "] Image map2d" + file_name + " was already loaded. Skipping this map texture..."); // debug v3.0.241.7.1
        }
      } // end loop over item map
      if (nChilds2 <= 0)
        Log::logMsg("No map2d images for:  " + leg_name); // debug v3.0.241.7.1
    }
  }

  //// read End Textures
  // use XML element information to read the files instead the properties
  if (!missionx::data_manager::endMissionElement.node.isEmpty())
  {
#ifdef IBM
    std::string end_file_name = Utils::readAttrib(missionx::data_manager::endMissionElement.node.getChildNode(mxconst::get_ELEMENT_END_SUCCESS_IMAGE().c_str()), mxconst::get_ATTRIB_FILE_NAME(), "");
#else
    IXMLNode    nodeEndSuccess = missionx::data_manager::endMissionElement.node.getChildNode(mxconst::get_ELEMENT_END_SUCCESS_IMAGE().c_str());
    std::string end_file_name  = Utils::readAttrib(nodeEndSuccess, mxconst::get_ATTRIB_FILE_NAME(), "");
#endif
    std::string errorMsg;
    mxTextureFile texture;
    texture.setTextureFile(end_file_name, fldMissionCustom_withSep + "briefer");
    if (missionx::BitmapReader::loadGLTexture(texture, errorMsg, false)) // load image but do not flip it
    {
      Utils::addElementToMap(data_manager::mapCurrentMissionTextures, end_file_name, texture);   // store texture data in map
      Log::logDebugBO("Loaded End Success image Texture: " + texture.getAbsoluteFileLocation()); // debug
    }

#ifdef IBM
    end_file_name = Utils::readAttrib(missionx::data_manager::endMissionElement.node.getChildNode(mxconst::get_ELEMENT_END_FAIL_IMAGE().c_str()), mxconst::get_ATTRIB_FILE_NAME(), "");
#else
    IXMLNode    nodeEndFail    = missionx::data_manager::endMissionElement.node.getChildNode(mxconst::get_ELEMENT_END_FAIL_IMAGE().c_str());
    end_file_name              = Utils::readAttrib(nodeEndFail, mxconst::get_ATTRIB_FILE_NAME(), "");
#endif
    texture.setTextureFile(end_file_name, fldMissionCustom_withSep + "briefer");
    if (missionx::BitmapReader::loadGLTexture(texture, errorMsg, false)) // load image but do not flip it
    {
      Utils::addElementToMap(data_manager::mapCurrentMissionTextures, end_file_name, texture);  // store texture data in map
      Log::logDebugBO("Loaded End Failed image Texture: " + texture.getAbsoluteFileLocation()); // debug
    }

  } // endMissionElement



  XPLMSetGraphicsState(0 /*Fog*/, 0 /*TexUnits*/, 0 /*Lighting*/, 0 /*AlphaTesting*/, 0 /*AlphaBlending*/, 0 /*DepthTesting*/, 0 /*DepthWriting*/);
}



void
missionx::Mission::initFlightLegDisplayObjects()
{
  if (data_manager::currentLegName.empty())
    return;

  // v3.0.241.1 rewrite init all 3d objects from "display_objects" element in flight leg
  for (const auto& [instName, templateName] : data_manager::mapFlightLegs[data_manager::currentLegName].list_displayInstances) // v3.305.1 convert list_displayInstances to a <map> container.   list_.mapProperties) // all properties are ["instance name" : "3d object template name"] values
  {
    const std::string obj_template_name = templateName;    //templateName.getValue(); 

    // ==== New Code Logic ===
    // If instance is not being displayed and the obj3D has an instanced element <display_object> with attribute "instance_name=ins_name"
    if ( ! mxUtils::isElementExists(data_manager::map3dInstances, instName) && mxUtils::isElementExists(data_manager::map3dObj, obj_template_name))
    {
      missionx::obj3d instanced_obj3d;
      instanced_obj3d.node = data_manager::map3dObj[obj_template_name].node.deepCopy(); // copy the node

      // parse_node will read all relevant information // 
      instanced_obj3d.setNodeStringProperty_drillDown(mxconst::get_ATTRIB_INSTANCE_NAME(), instName, instanced_obj3d.node, mxconst::get_ELEMENT_OBJ3D()); // v3.0.241.1 add the instance name to the new node so parse_node will read all relevant information 


      // add logic that reads from the sub element of <obj3d> with and element that has instance_name=inst_name 

#ifndef RELEASE
      {
        Log::logMsg("\n\nInstance Object3D:\n" + Utils::xml_get_node_content_as_text(instanced_obj3d.node) + "\n");
      }
#endif // !RELEASE


      if (instanced_obj3d.parse_node()) // prepare the copy
      {
        Utils::addElementToMap(data_manager::map3dInstances, instName, instanced_obj3d);
        // copy g_reference from 3D Object so instanced object will be created correctly
        data_manager::map3dInstances[instName].g_object_ref = data_manager::map3dObj[obj_template_name].g_object_ref;


        // validate link_task
        std::string link_objective_name = data_manager::map3dInstances[instName].getPropLinkToObjectiveName();
        std::string link_task           = data_manager::map3dInstances[instName].getPropLinkTask();
        if (link_objective_name.empty() || link_task.empty())
        {
          data_manager::map3dInstances[instName].node.deleteAttribute(mxconst::get_ATTRIB_LINK_TASK().c_str());
          data_manager::map3dInstances[instName].node.deleteAttribute(mxconst::get_PROP_LINK_OBJECTIVE_NAME().c_str());
        }
        else
        {
          // validate objective + task name exists in flightLeg
          bool foundObjectiveAndTask = false;
          for (const auto &o : missionx::data_manager::mapFlightLegs[data_manager::currentLegName].listObjectivesInFlightLeg)
          {
            if (o.compare(link_objective_name) == 0)
            {
              // check task
              if (mxUtils::isElementExists(data_manager::mapObjectives[link_objective_name].mapTasks, link_task))
              {
                foundObjectiveAndTask = true;

                data_manager::map3dInstances[instName].setNodeStringProperty(mxconst::get_PROP_LINK_OBJECTIVE_NAME(), link_objective_name); 
                data_manager::map3dInstances[instName].setNodeStringProperty(mxconst::get_ATTRIB_LINK_TASK(), link_task); 
              }
            }
          }

          if (!foundObjectiveAndTask)
          {
            data_manager::map3dInstances[instName].node.deleteAttribute(mxconst::get_ATTRIB_LINK_TASK().c_str());
            data_manager::map3dInstances[instName].node.deleteAttribute(mxconst::get_PROP_LINK_OBJECTIVE_NAME().c_str());
            Log::logMsgWarn("[validate objective/task] Fail to find valid task_link: " + mxconst::get_QM() + link_objective_name + "." + link_task + mxconst::get_QM() + ", removing link_task attributes");
          }
        }
        // End link validation
      }
    } // if instance name is new in container, and object template exists
    else
    {
      Log::logMsg("Instance name: " + mxconst::get_QM() + instName + mxconst::get_QM() + " might be in list or 3D Object template: " + mxconst::get_QM() + obj_template_name + mxconst::get_QM() + " might be wrong, skipping 3D Object...");
    }
    // ==== END New Code Logic ===



  } // end loop over all instance objects



  // end v3.0.200 //
}


// -------------------------------------

void
missionx::Mission::flc()
{
//#ifdef TIMER_FUNC
//  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
//#endif // TIMER_FUNC


  int         countMsg = 0;
  std::string msg;
  data_manager::smPropSeedValues.clear();

  //// v3.0.221.4 flush 50 messages each mission.flc() iteration
  while (!missionx::writeLogThread::qLogMessages_mainThread.empty() && (25 > countMsg))
  {
    msg = missionx::writeLogThread::qLogMessages_mainThread.front();
    XPLMDebugString(msg.c_str());
    missionx::writeLogThread::qLogMessages_mainThread.pop();

    ++countMsg;
  }

  missionx::Log::flc();


  // v3.0.219.9
  this->flc_threads(); // if we are optimizing the apt.dat files, this should monitor the thread and join it once it is finished its work

  //// v3.0.217.8
  //missionx::Log::flc();

  // display Visual Cues only if we are in Designer mode and Cue option is enabled
  if (Utils::readBoolAttrib(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_ENABLE_DESIGNER_MODE(), false) && Utils::readBoolAttrib(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_DISPLAY_VISUAL_CUES(), false))
    this->displayCueInfo = 1; // No, do not display
  else
    this->displayCueInfo = 0; //Utils::readBoolAttrib(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_DISPLAY_VISUAL_CUES(), false);


  // make sure we reset the plugin pause state if simmer unpause the sim
  if (!missionx::dataref_manager::isSimPause())
  {
    Mission::uiImGuiBriefer->setForcePauseSim(false);
    Mission::uiImGuiBriefer->setPluginPausedSim(false);
  }

  Mission::uiImGuiBriefer->flc(); // v3.0.251.1


  if (!missionx::data_manager::queCommandsWithTimer.empty()) // call flcPRE() again if we still have commands to execute
  {
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::execute_xp_command); // this to make the commands fire immediately and not wait for next FlightCallBack iteration. That way simmer should see all commands running at once.
  }

  flcPRE(); // call actions that need to be done or evaluated before the main flight call back.

  dataref_manager::flc(); // store plane & camera current location

  if (Mission::isMissionValid)
  {
    // sound loop
    // mxconst::QMM flc
    if (data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running && data_manager::missionState < missionx::mx_mission_state_enum::stop_all_async_processes) // v3.0.154 fix messages end before broadcast to user
    {
      // v3.0.303.7 Added 
      data_manager::smPropSeedValues.setStringProperty(mxconst::get_EXT_MX_CURRENT_LEG(), data_manager::currentLegName);

      missionx::QueueMessageManager::flc();         // Call mxconst::QMM even if mission ended
      flc_check_plane_in_external_inventory_area(); // v3.0.213.2

      if (!data_manager::mapFlightLegs[data_manager::currentLegName].flag_cue_was_calculated 
         && Utils::readNodeNumericAttrib<int>( missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_ENABLE_DESIGNER_MODE(), false) == 1 
         && Utils::readNodeNumericAttrib<int>( missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_DISPLAY_VISUAL_CUES() , false) == 1 
         )
      {
        missionx::data_manager::resetCueSettings();
        missionx::data_manager::flc_cue(missionx::cue_actions_enum::cue_action_first_time); // v3.0.223.4
      }
    }


    if (data_manager::missionState >= missionx::mx_mission_state_enum::mission_completed_success && data_manager::missionState <= missionx::mx_mission_state_enum::mission_completed_failure) // v3.303.11 continue drawing 3D Objects
    {
      // check which 3D Object to show/hide
      Mission::flc_3d_objects(data_manager::smPropSeedValues);
    }

    // We let mxconst::QMm to be called due to the fact that it might do some asynchronies work, so there is no real reason not to allow it.
    // We can't manage x-plane narrator though, but at least we can pause all sound channels
    if (missionx::dataref_manager::isSimPause()) // v3.0.207.3 skip Mission logic while in pause state
    {

      if (missionx::data_manager::timelapse.flag_ignorePauseMode && missionx::data_manager::timelapse.flag_isActive)
      {
        missionx::data_manager::timelapse.flc_timelapse();
      }    

      return;
    }

    if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running || data_manager::missionState == missionx::mx_mission_state_enum::pre_mission_running)
    {
      std::string err;
      // v3.0.221.15rc5 flc timelapse
      if (missionx::data_manager::timelapse.flag_isActive)
      {
        missionx::data_manager::timelapse.flc_timelapse();
      }

      if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
      {
        missionx::data_manager::flc_acf_change (); // v25.03.1

        data_manager::gather_stats.flc();
        // store last 15m fpm data
        if (data_manager::gather_stats.get_stats_object().fAGL > mxconst::AGL_TO_GATHER_FPM_DATA)
        {
          data_manager::strct_currentLegStats4UIDisplay.reset_fpm_and_gForce_data();
        }
        else if (data_manager::gather_stats.get_stats_object().faxil_gear == 0.0) // if airborne
        {
          data_manager::strct_currentLegStats4UIDisplay.vecFpm15Meters.emplace_back(data_manager::gather_stats.get_stats_object().vh_ind_fpm); // store fpm data
          data_manager::strct_currentLegStats4UIDisplay.vecgForce15Meters.emplace_back(data_manager::gather_stats.get_stats_object().gforce_normal); // store gForce data
        }

        if (missionx::data_manager::strct_sceneryOrPlaneLoadState.getCanPluginContinueEvaluation())
          Mission::flc_legs();


        if (Mission::uiImGuiMxpad)
          Mission::uiImGuiMxpad->flc();
        if (Mission::uiImGuiOptions)
          Mission::uiImGuiOptions->flc();

        // v3.303.13 dataref interpolation
        missionx::data_manager::flc_datarefs_interpolation();

        flc_check_success(); // v3.305.2


      }   // end if mission is running

    } // end if mission is valid and active

  }

  // v3.0.146 // copy all future commands to queFlcActions // most of the time it is probably empty
  // v3.0.225.1 moved post FLC actions after FLC handling
  // v3.0.241.7.1 moved all post action after main flc() condition so "wait_for_mission_to_stop" might work as expected.
  while (!data_manager::postFlcActions.empty())
  {
    missionx::mx_flc_pre_command c = data_manager::postFlcActions.front();
    data_manager::postFlcActions.pop();
    missionx::data_manager::queFlcActions.push(c); // place action in Queue. Should position plane
  }

  missionx::data_manager::prev_vr_is_enabled_state = missionx::mxvr::vr_display_missionx_in_vr_mode; // v3.0.221.6 store VR state each flight call back
}

// -------------------------------------

void
missionx::Mission::exec_apt_dat_optimization()
{
  if (missionx::data_manager::flag_apt_dat_optimization_is_running || OptimizeAptDat::aptState.flagIsActive)
  {
    Log::logAttention("apt dat optimization is currently running. Please wait for it to finish first.");
    XPLMSpeakString("apt dat optimization is currently running. Please wait for it to finish first.");

    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::disable_aptdat_optimize_menu);
  }
  else
  {
    missionx::data_manager::flag_apt_dat_optimization_is_running = true;
    // v3.0.219.9 parse AptDat
    // set folders
    if (data_manager::xplane_ver_i < missionx::XP12_VERSION_NO) // less than 120000 means it is xp11
    {
      missionx::OptimizeAptDat::setAptDataFolders(Utils::getCustomSceneryRelativePath(), "Resources/default scenery/default apt dat/Earth nav data", data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", data_manager::errStr)); // XP11
    }
    else
    {
      missionx::OptimizeAptDat::setAptDataFolders(Utils::getCustomSceneryRelativePath(), "Global Scenery/Global Airports/Earth nav data", data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", data_manager::errStr)); // XP12
    }

    const std::string xplane_ver_s = mxUtils::formatNumber<int>(data_manager::xplane_ver_i);
    Utils::addElementToMap(OptimizeAptDat::aptState.mapValues, mxconst::get_PROP_XPLANE_VERSION(), xplane_ver_s);
    Utils::addElementToMap(OptimizeAptDat::aptState.mapValues, mxconst::get_PROP_XPLANE_INSTALL_PATH(), Utils::getXPlaneInstallFolder());

    // call parsing
    if (data_manager::init_xp_airport_db()) // v3.0.241.10 b2
    {
      this->optAptDat.set_database_pointers(&data_manager::db_xp_airports); // v3.0.241.10
    }

    if (data_manager::init_xp_airport_db2()) // v3.0.255.3
    {
      this->optAptDat.set_database_pointers2(&data_manager::db_cache); // v3.0.255.3
    }

    this->optAptDat.exec_optimize_aptdat_thread();
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::disable_aptdat_optimize_menu);
    // clear cached nav data
    // this->engine.resetCache(); // v24.12.2 deprecated, it does nothing.
  }
}

// -------------------------------------

std::string Mission::checkGLError(const std::string &label)
{
    std::string strResult;
    strResult.clear();

    GLenum errorCode;
        while ((errorCode = glGetError()) != GL_NO_ERROR) {
            const char* error = nullptr;
            switch (errorCode) {
                case GL_INVALID_ENUM:      error = "GL_INVALID_ENUM"; break;
                case GL_INVALID_VALUE:     error = "GL_INVALID_VALUE"; break;
                case GL_INVALID_OPERATION: error = "GL_INVALID_OPERATION"; break;
                case GL_STACK_OVERFLOW:    error = "GL_STACK_OVERFLOW"; break;
                case GL_STACK_UNDERFLOW:   error = "GL_STACK_UNDERFLOW"; break;
                case GL_OUT_OF_MEMORY:     error = "GL_OUT_OF_MEMORY"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: error = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
                default: error = "Unknown Error"; break;
            }
            strResult += "OpenGL Error [" + label + "]: " + std::string(error) + "\n";
        }

    return strResult;
}

// -------------------------------------

void
missionx::Mission::flc_threads()
{
//#ifdef TIMER_FUNC
//  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
//#endif // TIMER_FUNC

  ///////////////////////////////
  //  Apt Dat Thread
  if (this->optAptDat.aptState.flagAbortThread)
  {
    if (this->optAptDat.thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
      this->optAptDat.thread_ref.join();

    this->optAptDat.aptState.init();

    missionx::data_manager::flag_apt_dat_optimization_is_running = false;
    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::enable_aptdat_optimize_menu);
  }
  else
  {
    if (this->optAptDat.aptState.flagIsActive && !this->optAptDat.aptState.flagThreadDoneWork)
    {
      missionx::data_manager::flag_apt_dat_optimization_is_running = true;
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::disable_aptdat_optimize_menu);
    }
    else
    {
      // reset thread
      if (this->optAptDat.aptState.flagThreadDoneWork)
      {
        const std::string msg = "\t\t--- APT.DAT optimization finished (" + OptimizeAptDat::aptState.getDuration() + "s) ---";

        if (this->optAptDat.thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
          this->optAptDat.thread_ref.join();

        this->optAptDat.aptState.init();

        missionx::data_manager::flag_apt_dat_optimization_is_running = false;
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::enable_aptdat_optimize_menu);

        // v3.0.253.6 add progress message
        if (Mission::uiImGuiBriefer != nullptr)
          Mission::uiImGuiBriefer->setMessage(msg);
      }
    }
  } // end AptDat thread handling



  ///////////////////////////////
  //  Random Engine Thread
  if (this->engine.threadState.flagAbortThread)
  {
    if (RandomEngine::thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
      RandomEngine::thread_ref.join();

    missionx::data_manager::overpass_fetch_err.clear(); // v3.0.255.4 clear the cURL error
    if (!this->engine.getErrorMsg().empty())
    {
      Mission::uiImGuiBriefer->setMessage(this->engine.getErrorMsg(), 60);
    }
    RandomEngine::threadState.init(); // init again to reset

    missionx::data_manager::flag_generate_engine_is_running = false;

    Mission::uiImGuiBriefer->flag_generatedRandomFile_success = false; // we should remove this flag, and only use "missionx::data_manager::flag_generate_engine_is_running"
    Mission::uiImGuiBriefer->selectedTemplateKey.clear();              // reset and hide generate file button
    Mission::uiImGuiBriefer->lastSelectedTemplateKey.clear();          // reset and hide generate file button

    missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::enable_generator_menu);

    missionx::data_manager::reset_runway_search_filter(); // reset the property value so it won't conflict with other searches
  }
  else
  {
    if (RandomEngine::threadState.flagIsActive && !RandomEngine::threadState.flagThreadDoneWork)
    {
      missionx::data_manager::flag_generate_engine_is_running   = true;
      Mission::uiImGuiBriefer->flag_generatedRandomFile_success = false; // we should remove this flag, and only use "missionx::data_manager::flag_generate_engine_is_running"

      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::disable_generator_menu);
    }
    else
    {

      // reset thread
      if (RandomEngine::threadState.flagThreadDoneWork)
      {

        missionx::data_manager::overpass_fetch_err.clear();   // v3.0.255.4 clear the cURL error
        missionx::data_manager::reset_runway_search_filter(); // reset the property value so it won't conflict with other searches

        if (RandomEngine::thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
          RandomEngine::thread_ref.join();

        RandomEngine::threadState.init();
        missionx::data_manager::flag_generate_engine_is_running = false;

        Mission::uiImGuiBriefer->setMessage("Finished Generating mission file. [Destinations: " + Utils::formatNumber<int>(this->engine.get_num_of_flight_legs()) + "].  Based on \"" + Mission::uiImGuiBriefer->selectedTemplateKey + "\".", 8);

        if (missionx::data_manager::getGeneratedFromLayer() == missionx::uiLayer_enum::option_external_fpln_layer )
          Mission::uiImGuiBriefer->asyncSecondMessageLine.clear(); // Do not display Flight Plan in "option_external_fpln_layer" layer since we know the route.
        else
          Mission::uiImGuiBriefer->asyncSecondMessageLine = "Last F.Plan: " + this->engine.cumulative_location_desc_s; // display in generate mission user layer.

        Log::logMsg(Mission::uiImGuiBriefer->asyncSecondMessageLine); // v3.0.255.5 added flight plan to log

        // v3.0.223.1 add briefing information
        if (data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running && !Mission::uiImGuiBriefer->selectedTemplateKey.empty()) //
        {
          IXMLNode xBriefer = this->engine.getBrieferNode();
          if (!xBriefer.isEmpty())
          {
            IXMLClear cdata = xBriefer.getClear();
            if (cdata.sValue)
            {
              const std::string desc_s = cdata.sValue;
              if (!desc_s.empty())
              {

                //// set Mission Briefer Info for random
                if (Mission::uiImGuiBriefer->selectedTemplateKey.find(mxconst::get_XML_EXTENTION()) !=
                    std::string::npos) // file ends with XML (means predefined template and not based on "template mission folder" file which ends with no extension.
                {
                  if (Utils::isElementExists(data_manager::mapBrieferMissionList, mxconst::get_RANDOM_MISSION_DATA_FILE_NAME()))
                  {
                    data_manager::mapBrieferMissionList[mxconst::get_RANDOM_MISSION_DATA_FILE_NAME()].setBrieferDescription(desc_s);
                  }
                  // v3.0.251.1 also add the description to the original BrieferInfo
                  if (Utils::isElementExists(data_manager::mapBrieferMissionList, Mission::uiImGuiBriefer->selectedTemplateKey))
                  {
                    data_manager::mapBrieferMissionList[Mission::uiImGuiBriefer->selectedTemplateKey].setBrieferDescription(desc_s);
                  }
                }
                else if (Utils::isElementExists(data_manager::mapBrieferMissionList, (Mission::uiImGuiBriefer->selectedTemplateKey + mxconst::get_XML_EXTENTION())))
                {
                  data_manager::mapBrieferMissionList[(Mission::uiImGuiBriefer->selectedTemplateKey + mxconst::get_XML_EXTENTION())].setBrieferDescription(desc_s);
                }
                else
                  Log::logMsgWarn("Failed to write generated mission description into BrieferInfo !!!\n" + desc_s);


                engine.working_tempFile_ptr->description = desc_s; // v3.0.251.1
                engine.working_tempFile_ptr->prepareSentenceBasedOnString(desc_s); // v3.303.14

              }
            }
          }
        }


        Mission::uiImGuiBriefer->flag_generatedRandomFile_success = true;
        Mission::uiImGuiBriefer->lastSelectedTemplateKey          = Mission::uiImGuiBriefer->selectedTemplateKey; // reset and hide generate file button
        Mission::uiImGuiBriefer->selectedTemplateKey.clear();                                                     // reset and hide generate file button after successful creation

        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::enable_generator_menu);

        // add briefing information
      }
    }
  } // end Random engine thread handling


  //////////////////////////////
  // Optimization Threads  // v3.305.2
  
  #ifdef USE_TRIGGER_OPTIMIZATION
  if (missionx::data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running)
  {
    if (data_manager::optimize_leg_triggers_strct.optLegTriggers_thread_state.abort_thread)
    {
      if (data_manager::optimize_leg_triggers_strct.optLegTriggers_thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
        data_manager::optimize_leg_triggers_strct.optLegTriggers_thread_ref.join();
  
      data_manager::optimize_leg_triggers_strct.optLegTriggers_thread_state.init();
  
      // missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::enable_aptdat_optimize_menu);
    }
    else
    {
  
      if (!data_manager::optimize_leg_triggers_strct.optLegTriggers_thread_state.flagIsActive)
      {
        if (data_manager::optimize_leg_triggers_strct.optLegTriggers_thread_state.thread_done_work)
        {
          if (data_manager::optimize_leg_triggers_strct.optLegTriggers_thread_ref.joinable()) // "join" previous thread before creating new thread. This should be very fast since the threaded function must have finished before reaching this line.
            data_manager::optimize_leg_triggers_strct.optLegTriggers_thread_ref.join();
  
          data_manager::optimize_leg_triggers_strct.optLegTriggers_thread_state.init();
  
          // copy triggers from 
          data_manager::mapFlightLegs[data_manager::currentLegName].listTriggersByDistance.clear();
          data_manager::mapFlightLegs[data_manager::currentLegName].listTriggersByDistance = data_manager::mapFlightLegs[data_manager::currentLegName].listTriggersByDisatnce_thread;
  
          this->timerOptLegTriggersTimer.reset();
          missionx::Timer::start(this->timerOptLegTriggersTimer, 10.0f);
        }
        else if (missionx::data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
        {      
  
          if (Timer::evalTime(this->timerOptLegTriggersTimer))
          {
            this->timerOptLegTriggersTimer.reset();
  
            missionx::data_manager::optimize_leg_triggers_strct.optLegTriggers_thread_planePos = missionx::dataref_manager::getCurrentPlanePointLocation();
  
            missionx::data_manager::optimize_leg_triggers_strct.optLegTriggers_thread_ref = std::thread(&missionx::data_manager::optimizeLegTriggers_thread,
                                                                            &missionx::data_manager::optimize_leg_triggers_strct.optLegTriggers_thread_state,
                                                                            &missionx::data_manager::optimize_leg_triggers_strct.optLegTriggers_thread_planePos,
                                                                            &data_manager::mapTriggers,
                                                                            &data_manager::mapFlightLegs[data_manager::currentLegName].listTriggersByDistance,
                                                                            &data_manager::mapFlightLegs[data_manager::currentLegName].listTriggersByDisatnce_thread);
          }
          else if (this->timerOptLegTriggersTimer.getState() == mx_timer_state::timer_not_set)
          {
            this->timerOptLegTriggersTimer.reset();
            missionx::Timer::start(this->timerOptLegTriggersTimer, 1.0f); // as soon as possible
          }
          
        }  // is mission running ?
      } // end else data_manager::optLegTriggers_thread_state.flagIsActive && !data_manager::optLegTriggers_thread_state.thread_done_work
    }
  } // end if mission is, at least, running
  #endif

} // end flcThread()

// -------------------------------------

void
missionx::Mission::setMissionTime()
{
  dataref_const dc;
  std::string err;

  //  // get information on date
  //  // read time information from mission settings
  std::string current_local_date_days_s = mxUtils::formatNumber<int>(XPLMGetDatai(dc.dref_local_date_days_i));
  auto        current_local_time_sec_f        = XPLMGetDataf(dc.dref_local_time_sec_f);
  const auto  current_localHour_s             = mxUtils::formatNumber<float>(current_local_time_sec_f / 3600.0f, 0); // convert to local hour in a day
  

  missionx::data_manager::set_local_time(mxUtils::stringToNumber<int>(Utils::readAttrib(data_manager::mx_global_settings.xStartTime_ptr, mxconst::get_ATTRIB_TIME_HOURS(), current_localHour_s))
                                       , mxUtils::stringToNumber<int> (Utils::readAttrib(data_manager::mx_global_settings.xStartTime_ptr, mxconst::get_ATTRIB_TIME_MIN(), "0") )
                                       , mxUtils::stringToNumber<int>( Utils::readAttrib(data_manager::mx_global_settings.xStartTime_ptr, mxconst::get_ATTRIB_TIME_DAY_IN_YEAR(), current_local_date_days_s) )
                                       , true // force set time. Ignore timelapse is active
                                       ); 
}

// -------------------------------------

void
missionx::Mission::flc_legs()
{
  // 1. loop over goals/legs.
  // 1.1 Get number of objectives of the active flightLeg/leg
  // 1.2 loop over each objective and
  // 2. check if objective is complete and store if success or failed.
  // 3. If objective is not complete, call objective flc_obj() and send its name.
  // 3.1 Go over objective tasks and check if they are complete and mandatory

#ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), data_manager::currentLegName, false);
#endif // TIMER_FUNC

  bool flag_found; // v3.0.241.1 added for Utils::xml_get_attribute_value_drill() uses
                   // Goal *flightLeg = &missionx::data_manager::mapFlightLegs[data_manager::currentLegName];

  // 3.0.223.7 moved this code to the beginning of the function
  // prepare seed attribute
  data_manager::smPropSeedValues.clear();
  data_manager::smPropSeedValues.setStringProperty(mxconst::get_EXT_MX_CURRENT_LEG(), data_manager::currentLegName);  // v3.0.221.15rc5 support leg - store current Leg

  // fix first time fire after loading from save point.
  // the value for "isFirstTime" is the opposite of the attribute: mxconst::get_PROP_LOADED_FROM_CHECKPOINT() which is present only in saved files.
  data_manager::mapFlightLegs[data_manager::currentLegName].isFirstTime = Utils::readBoolAttrib(data_manager::mapFlightLegs[data_manager::currentLegName].node, mxconst::get_PROP_IS_FIRST_TIME(), true); // v3.0.241.1

  // v3.0.207.5 flightLeg extended features implementation  // This is a one time script and message call
  if (data_manager::mapFlightLegs[data_manager::currentLegName].isFirstTime)
  {
    data_manager::mapFlightLegs[data_manager::currentLegName].isFirstTime = false; // reset state
    data_manager::mapFlightLegs[data_manager::currentLegName].setNodeProperty<bool>(mxconst::get_PROP_IS_FIRST_TIME(), false);

    // v3.303.14 Store/init stats
    missionx::data_manager::gatherFlightLegStartStats();


    if (!data_manager::mapFlightLegs[data_manager::currentLegName].getIsDummyLeg()) // v3.303.12
    {
      // v3.0.303 store target POI
      double                  target_lat = Utils::readNodeNumericAttrib<double>(data_manager::mapFlightLegs[data_manager::currentLegName].node, mxconst::get_ATTRIB_TARGET_LAT(), 0.0);
      double                  target_lon = Utils::readNodeNumericAttrib<double>(data_manager::mapFlightLegs[data_manager::currentLegName].node, mxconst::get_ATTRIB_TARGET_LON(), 0.0);
      missionx::dataref_param dref_target_lat("xpshared/target/lat");
      dref_target_lat.setValue(target_lat);
      dataref_param::set_dataref_values_into_xplane(dref_target_lat);

      missionx::dataref_param dref_target_lon("xpshared/target/lon");
      dref_target_lon.setValue(target_lon);
      dataref_param::set_dataref_values_into_xplane(dref_target_lon);
    }


    // draw script - 1 per Flight leg. If not defined then remove previous draw_script value.
    data_manager::draw_script = Utils::xml_get_attribute_value_drill(data_manager::mapFlightLegs[data_manager::currentLegName].node,
                                                                     mxconst::get_ATTRIB_NAME(),
                                                                     flag_found,
                                                                     mxconst::get_ELEMENT_DRAW_SCRIPT()); 

    // pre script
    const bool        wasPreScriptFired = Utils::readBoolAttrib(data_manager::mapFlightLegs[data_manager::currentLegName].node, mxconst::get_PROP_PRE_LEG_SCRIPT_FIRED(), false);
    const std::string scriptName        = Utils::xml_get_attribute_value_drill(data_manager::mapFlightLegs[data_manager::currentLegName].node, mxconst::get_ATTRIB_NAME(), flag_found, mxconst::get_ELEMENT_PRE_LEG_SCRIPT());
    if (!wasPreScriptFired && !scriptName.empty())
    {
      missionx::data_manager::execScript(scriptName, data_manager::smPropSeedValues, "[flc] Pre Leg script: " + scriptName + " is invalid. Aborting Mission"); // can alter success flag or message
      data_manager::mapFlightLegs[data_manager::currentLegName].setNodeProperty<bool>(mxconst::get_PROP_PRE_LEG_SCRIPT_FIRED(), true);
    }

    // Metar
    injectMetarFile();

    // fail timer
    AddFlightLegFailTimers();

    // Send start Flight Leg message
    const bool        wasStartMessageFired = Utils::readBoolAttrib(data_manager::mapFlightLegs[data_manager::currentLegName].node,
                                                            mxconst::get_PROP_START_LEG_MESSAGE_FIRED(),
                                                            false); // v3.0.213.7 // if has property then probably was fired, if not then we are all good.
    const std::string messageName          = Utils::xml_get_attribute_value_drill(data_manager::mapFlightLegs[data_manager::currentLegName].node, mxconst::get_ATTRIB_NAME(), flag_found, mxconst::get_ELEMENT_START_LEG_MESSAGE());

    if (!messageName.empty() && !wasStartMessageFired)
    {
      missionx::QueueMessageManager::addMessageToQueue(messageName, missionx::EMPTY_STRING, data_manager::errStr);

      data_manager::mapFlightLegs[data_manager::currentLegName].setNodeProperty<bool>(mxconst::get_PROP_START_LEG_MESSAGE_FIRED(), true); // v3.0.241.1
    }

    // v3.0.221.9 call begin Flight Leg commands
    if (!data_manager::mapFlightLegs[data_manager::currentLegName].listCommandsAtTheStartOfFlightLeg.empty())
    {
      while (!data_manager::mapFlightLegs[data_manager::currentLegName].listCommandsAtTheStartOfFlightLeg.empty())
      {
        missionx::data_manager::queCommands.push_back(data_manager::mapFlightLegs[data_manager::currentLegName].listCommandsAtTheStartOfFlightLeg.front());
        data_manager::mapFlightLegs[data_manager::currentLegName].listCommandsAtTheStartOfFlightLeg.pop_front();
      }
      missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::execute_xp_command);
    }

    // v3.303.12 apply weather
    missionx::data_manager::apply_datarefs_from_text_based_on_parent_node_and_tag_name(data_manager::mapFlightLegs[data_manager::currentLegName].node, mxconst::get_ELEMENT_WEATHER());

  } // END if "flight leg" first time

  // loop over flight leg objectives
  unsigned int countMandatory         = 0;
  unsigned int countMandatoryComplete = 0;
  unsigned int countComplete          = 0;
  size_t       badObjs, goodObjs;
  badObjs = goodObjs = 0;


  // Fail Timers
  if (!missionx::data_manager::timelapse.flag_isActive) // v3.0.303.7 evaluate fail timers only if timelapse is not active
    flc_fail_timers(); // v3.0.253.7

  // v3.303.12 loop over Objectives only if the <leg> is not flagged as a "dummy" leg
  if (!data_manager::mapFlightLegs[data_manager::currentLegName].getIsDummyLeg()) // v3.303.12
  {
    for (auto& objName : data_manager::mapFlightLegs[data_manager::currentLegName].listObjectivesInFlightLeg)
    {
      if (!Utils::isElementExists(data_manager::mapObjectives, objName))
      {
        Log::logMsgWarn("Objective: " + mxconst::get_QM() + objName + mxconst::get_QM() + ", is not in Flight Leg map. Check your mission datafile and notify developer. ");
        continue;
      }
      // fail to find objective name - skip
      if (!data_manager::mapObjectives[objName].isValid)
      {
        badObjs++;
        Log::logMsgWarn("Objective: " + mxconst::get_QM() + objName + mxconst::get_QM() + ", is not valid. skipping evaluation.");
        continue;
      }

      Mission::currentObjectiveName = objName; // useful for extSetTask(), flc_trigger() v25.02.1. can see which objective is currently being executed

      // end v3.0.207.5 flightLeg extentions


      // call OBJECTIVE FLC()
      Mission::flc_objective(objName, data_manager::smPropSeedValues);


      // count success
      if (!data_manager::mapObjectives[objName].isValid) // if after testing tasks we invalidate objective just skip
      {
        badObjs++;
        continue;
      }
      else if (data_manager::mapObjectives[objName].isMandatory)
      {
        countMandatory++;
        if (data_manager::mapObjectives[objName].isComplete)
        {
          countComplete++;
          countMandatoryComplete++;
        }
      }
      else if (data_manager::mapObjectives[objName].isComplete)
        countComplete++;

      goodObjs++;

    } // End loop over Objectives and check if all mandatory are complete
  }
  
  // loop over triggers if mission is active
  if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
  {
    #ifdef USE_TRIGGER_OPTIMIZATION
    int counter = 1;
    for (auto& trigName : data_manager::mapFlightLegs[data_manager::currentLegName].listTriggersByDistance)
    {
      if (trigName.empty())
        continue;

      if (counter > 2) // exit loop after 2 tests
        break;

      if (data_manager::mapTriggers[trigName].isEnabled) // v3.0.221.15rc4 added check if trigger is enabled
      {
        if ((counter * data_manager::mapTriggers[trigName].flag_inPhysicalArea_fromThread) )
        {
          Mission::flc_trigger(data_manager::mapTriggers[trigName], data_manager::smPropSeedValues);
          ++counter;

          // execute POST trigger script
          std::string scriptName = Utils::readAttrib(data_manager::mapTriggers[trigName].node, mxconst::get_ATTRIB_POST_SCRIPT(), "");                                             // v3.0.241.1
          missionx::data_manager::execScript(scriptName, data_manager::smPropSeedValues, "[flc_legs] Post Trigger script: " + scriptName + " is invalid. Aborting Mission"); // can alter success flag
        }

      }
      
    }

    for (auto& trigName : data_manager::mapFlightLegs[data_manager::currentLegName].listTriggersOthers)
    {
      if (trigName.empty())
        continue;

      if (data_manager::mapTriggers[trigName].isEnabled) // v3.0.221.15rc4 added check if trigger is enabled
      {
        Mission::flc_trigger(data_manager::mapTriggers[trigName], data_manager::smPropSeedValues);
      }
      // execute POST trigger script
      std::string scriptName = Utils::readAttrib(data_manager::mapTriggers[trigName].node, mxconst::get_ATTRIB_POST_SCRIPT(), "");                                             // v3.0.241.1
      missionx::data_manager::execScript(scriptName, data_manager::smPropSeedValues, "[flc_legs] Post Trigger script: " + scriptName + " is invalid. Aborting Mission"); // can alter success flag
    }

  
    #else 

    for (auto& trigName : data_manager::mapFlightLegs[data_manager::currentLegName].listTriggers)
    {
      if (trigName.empty())
        continue;

      if ((Utils::readBoolAttrib(missionx::data_manager::mapTriggers[trigName].node, mxconst::get_ATTRIB_ENABLED(), true)) && (missionx::data_manager::mapTriggers[trigName].trigState != missionx::mx_trigger_state_enum::never_trigger_again )) // v25.02.1 added never_trigger_again // v3.0.221.15rc4 added check if trigger is enabled
      {       
        this->flc_trigger(data_manager::mapTriggers[trigName], data_manager::smPropSeedValues);
      }
      // execute POST trigger script
      std::string scriptName = Utils::readAttrib(data_manager::mapTriggers[trigName].node, mxconst::get_ATTRIB_POST_SCRIPT(), "");                                             // v3.0.241.1
      missionx::data_manager::execScript(scriptName, data_manager::smPropSeedValues, "[flc_legs] Post Trigger script: " + scriptName + " is invalid. Aborting Mission"); // can alter success flag
    }

    #endif


  } // end execute triggers only if mission is in running state

  // v3.0.200
  if (data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_running)
  {
    // check which 3D Object to show/hide
    Mission::flc_3d_objects(data_manager::smPropSeedValues);

  } // end execute triggers only if mission is in running state



  //----------------------------------------------
  // calculate and decide end of Flight Leg
  //----------------------------------------------  
  // v3.305.1 this logic should eliminate cases where the flight leg is done -
  // but we continue evaluating and reseting the flight leg to success instead of waiting for the flight leg to finish the post actions.
  // So once we reached the state: mx_flightLeg_state::leg_success_do_post or higher we do not recalculate flight leg state anymore.
  if (data_manager::mapFlightLegs[data_manager::currentLegName].getFlightLegState() < missionx::enums::mx_flightLeg_state::leg_success_do_post) 
  {
    // v3.303.12 Handle special case where <leg> is flagged as "dummy"
    if (data_manager::mapFlightLegs[data_manager::currentLegName].getIsDummyLeg())
    {
      const int iCounter = Utils::readNodeNumericAttrib<int>(data_manager::mapFlightLegs[data_manager::currentLegName].node, mxconst::get_ATTRIB_DUMMY_LEG_ITERATIONS(), 0) + 1; // Add 1 to iteration of a dummy leg
      data_manager::mapFlightLegs[data_manager::currentLegName].setNodeProperty<int>(mxconst::get_ATTRIB_DUMMY_LEG_ITERATIONS(), iCounter);                                      // set the first itteration

      if (iCounter > 2)
      { // flag the flight leg as success after 2 itterations
        data_manager::mapFlightLegs[data_manager::currentLegName].setIsComplete(true, missionx::enums::mx_flightLeg_state::leg_success);
        data_manager::mapFlightLegs[data_manager::currentLegName].storeCoreAttribAsProperties();
      }
      else
      {
        data_manager::mapFlightLegs[data_manager::currentLegName].setIsComplete(false, missionx::enums::mx_flightLeg_state::leg_undefined);
        data_manager::mapFlightLegs[data_manager::currentLegName].storeCoreAttribAsProperties();
      }
    }
    else if (data_manager::mapFlightLegs[data_manager::currentLegName].hasMandatory)
    {
      if (countMandatory > 0 && countMandatory <= countMandatoryComplete)
      {
        data_manager::mapFlightLegs[data_manager::currentLegName].setIsComplete(true, missionx::enums::mx_flightLeg_state::leg_success);
        data_manager::mapFlightLegs[data_manager::currentLegName].storeCoreAttribAsProperties();
      }
      else
      {
        data_manager::mapFlightLegs[data_manager::currentLegName].setIsComplete(false, missionx::enums::mx_flightLeg_state::leg_undefined);
        data_manager::mapFlightLegs[data_manager::currentLegName].storeCoreAttribAsProperties();
      }
    }
    else if ((data_manager::mapFlightLegs[data_manager::currentLegName].listObjectivesInFlightLeg.size() - badObjs) <= countComplete) // if objective does not have mandatory, then check optional tasks state.
    {

      data_manager::mapFlightLegs[data_manager::currentLegName].setIsComplete(true, missionx::enums::mx_flightLeg_state::leg_success);
      data_manager::mapFlightLegs[data_manager::currentLegName].storeCoreAttribAsProperties();
    }
    else
    {
      data_manager::mapFlightLegs[data_manager::currentLegName].setIsComplete(false, missionx::enums::mx_flightLeg_state::leg_undefined); // if no mandatory then there should not be failure, always undefined or success
      data_manager::mapFlightLegs[data_manager::currentLegName].storeCoreAttribAsProperties();
    }
  }

} // end flc_legs()


// -------------------------------------

void
missionx::Mission::flc_fail_timers()
{
  auto it    = missionx::data_manager::mapFailureTimers.begin();
  auto itEnd = missionx::data_manager::mapFailureTimers.end();

  std::list<std::map<std::string, missionx::Timer>::iterator> erase_list_iterator;

  auto        lowestStoperTime_f = std::numeric_limits<float>::max();
  std::string lowestStoperName_s{ "" };
  while (it != itEnd)
  {

    if (it->second.isRunning())
    {

      if (missionx::Timer::wasEnded(it->second))
      {
        std::string customFailMessageText = "You failed to complete in time. Aborting mission !!!";
        if (!it->second.node.isEmpty())
        {
          const bool bFailMissionOnTimeout = Utils::readBoolAttrib(it->second.node, mxconst::get_ATTRIB_FAIL_ON_TIMEOUT_B(), true);

          const std::string msgKey = Utils::readAttrib(it->second.node, mxconst::get_ATTRIB_FAIL_MSG(), "");

          if (Utils::isElementExists(missionx::data_manager::mapMessages, msgKey))
          {
            IXMLNode textMixNode =
              Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(missionx::data_manager::mapMessages[msgKey].node, mxconst::get_ELEMENT_MIX(), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), mxconst::get_CHANNEL_TYPE_TEXT());
            customFailMessageText = Utils::xml_read_cdata_node(textMixNode, customFailMessageText);
          }

          if (bFailMissionOnTimeout) // v3.305.3
          {
            if (!customFailMessageText.empty())
              data_manager::mx_global_settings.setNodeStringProperty(mxconst::get_PROP_MISSION_ABORT_REASON(), customFailMessageText); // v3.303.11

            missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::abort_mission);
          }
          else // v3.305.3 do not abort/fail the mission
          {
            if (!msgKey.empty()) // send the fail message name to mxconst::QMM as simple message.
            {
              std::string err, track;
              QueueMessageManager::addMessageToQueue(msgKey, track, err);
            }
          }

          // v3.305.3 Call post script
          std::string scriptName = Utils::readAttrib(it->second.node, mxconst::get_ATTRIB_POST_SCRIPT(), "");
          missionx::data_manager::execScript(scriptName, data_manager::smPropSeedValues, "[" + std::string(__func__) + "] Timer post script: " + scriptName + " is invalid. Aborting Mission");

          break; // exit the while loop
        }
      }
      else if (Utils::readAttrib(it->second.node, mxconst::get_ATTRIB_RUN_UNTIL_LEG(), "").compare(missionx::data_manager::currentLegName) == 0)
      {
        // success
        it->second.setEnd();

        // send success message if there is any
        std::string message = Utils::readAttrib(it->second.node, mxconst::get_ATTRIB_SUCCESS_MSG(), "");
        if (!message.empty())
        {
          std::string err, track;
          QueueMessageManager::addMessageToQueue(message, track, err);
        }

        erase_list_iterator.emplace_back(it);
      }
      else
      {
        // try to figure which timer has the shortest time. This will be used when we display in the UI windows
        const auto seconds_remaining_f = it->second.getRemainingTime();
        if (seconds_remaining_f < lowestStoperTime_f)
        {
          lowestStoperTime_f = seconds_remaining_f;
          lowestStoperName_s = it->first;
        }
      }
    }
    else if (it->second.getState() == mx_timer_state::timer_is_set)
    {
      if (it->second.flag_loadedFromSavePoint)
        it->second.start_timer_after_savepoint_load();
      else
        it->second.start_timer_based_on_node();
    }

    it++;
  } // end while

  // erase from map
  for (auto& it : erase_list_iterator)
  {
#ifndef RELEASE
    Log::logMsg("Released Failure timer: " + missionx::Timer::to_string(it->second));
#endif
    missionx::data_manager::mapFailureTimers.erase(it);
  }

  missionx::data_manager::lowestFailTimerName_s       = lowestStoperName_s;
  missionx::data_manager::formated_fail_timer_as_text = missionx::data_manager::get_fail_timer_in_formated_text();
} // flc_fail_timers


// -------------------------------------


bool
missionx::Mission::handle_choice_option()
{

  IXMLNode xOption_ptr = missionx::data_manager::mxChoice.get_picked_option_node_ptr();
  if (!xOption_ptr.isEmpty())
  {
    std::string err;

    ///// Pre Actions /////
    const bool flag_hide = Utils::readBoolAttrib(xOption_ptr, mxconst::get_ATTRIB_ONETIME_OPTION_B(), false);
    xOption_ptr.updateAttribute((flag_hide) ? mxconst::get_MX_TRUE().c_str() : mxconst::get_MX_FALSE().c_str(), mxconst::get_ATTRIB_HIDE().c_str(), mxconst::get_ATTRIB_HIDE().c_str());
    if (Utils::isElementExists(missionx::data_manager::mxChoice.mapOptions, missionx::data_manager::mxChoice.optionPicked_key_i)) // direct update
      missionx::data_manager::mxChoice.mapOptions[missionx::data_manager::mxChoice.optionPicked_key_i].flag_hidden = flag_hide;

    // execute script
    data_manager::smPropSeedValues.clear();
    data_manager::smPropSeedValues.setStringProperty(mxconst::get_EXT_MX_CURRENT_LEG(), data_manager::currentLegName); // v3.0303.7 seed current Leg. We can only seed flight leg

    const std::string scriptName = Utils::readAttrib(xOption_ptr, mxconst::get_ATTRIB_SCRIPT_NAME_WHEN_FIRED(), "");
    missionx::data_manager::execScript(scriptName, data_manager::smPropSeedValues, "[choice] Choice script: " + scriptName + " is invalid. Aborting Mission");

    // datarefs
    const std::string datarefs = Utils::readAttrib(xOption_ptr, mxconst::get_ATTRIB_DATAREF_TO_EXEC_WHEN_FIRED(), ""); 

    // commands
    const std::string commands = Utils::readAttrib(xOption_ptr, mxconst::get_ATTRIB_COMMANDS_TO_EXEC_WHEN_FIRED(), ""); 
    data_manager::execute_commands(commands);                                                                     // will run next flight loop call


    // send messages
    const std::string messageName = Utils::readAttrib(xOption_ptr, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED(), "");
    missionx::QueueMessageManager::addMessageToQueue(messageName, EMPTY_STRING, err);

    const auto lmbda_find_next_choice = [&](const std::string& inAttribName) {
      std::string next_name = Utils::readAttrib(xOption_ptr, inAttribName, ""); // mxconst::ATTRIB_NEXT_CHOICE_NAME
      if (next_name.empty())
      {
        IXMLNode xParent_ptr = xOption_ptr.getParentNode();
        next_name            = Utils::readAttrib(xParent_ptr, inAttribName, ""); // read attribute from parent level
      }

      return next_name;
    }; // return the next choice name from chold node or its parent node (<option> or <choice>)


    const std::string next_choice_name = lmbda_find_next_choice(mxconst::get_ATTRIB_NEXT_CHOICE());

    ///// Post Actions /////
    if (next_choice_name.empty())
    {
      if (Mission::uiImGuiOptions)
        Mission::uiImGuiOptions->execAction(missionx::mx_window_actions::ACTION_HIDE_WINDOW);

      missionx::data_manager::mxChoice.init();
    }
    else
      missionx::data_manager::prepare_choice_options(next_choice_name);



    return true;
  }

  // reset mxChoice picked option so user can pick same option if it is available
  missionx::data_manager::mxChoice.reset_option_picked_key();

  return false;
}

// -------------------------------------

void
missionx::Mission::flc_objective(std::string& inObjName, mxProperties& inSmPropSeedValues)
{
#ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), inObjName, false);
#endif // TIMER_FUNC

  // loop over all objective tasks
  // if continues check, fire all Task tests
  Objective* obj = &data_manager::mapObjectives[inObjName];
  inSmPropSeedValues.setStringProperty(mxconst::get_EXT_MX_CURRENT_OBJ(), inObjName); // prepare seed attributes


  if ((obj->isComplete && obj->hasTaskWithAlwaysEvalProperty) || (!obj->isComplete))
  {
    unsigned int countMandatoryComplete        = 0;
    const auto mandatoryTaskCountInObjective = obj->vecMandatoryTasks.size();

    // loop over vecNotDependentTasks
    missionx::data_manager::strct_success_timer_info.reset(); // v25.02.1 Reset the "task timer struct" in order to populate it the latest state information. We will use it to evaluate which trigger has the lowest ATTRIB_EVAL_SUCCESS_FOR_N_SEC
    for (const auto &taskName : obj->vecNotDependentTasks)
    {
      // CALL TASK FLC()
      if (flc_task(taskName, data_manager::mapObjectives[inObjName], inSmPropSeedValues) ) // returns boolean, but we do not use it
      {
        const std::string listOfTasks_s = Utils::readAttrib ( obj->mapTasks[taskName].node, mxconst::get_ATTRIB_SET_OTHER_TASKS_AS_SUCCESS(), "");
        if ( !listOfTasks_s.empty() )
          missionx::data_manager::set_success_or_reset_tasks_state(inObjName,  listOfTasks_s, missionx::enums::mx_action_from_trigger_enum::set_success , taskName);
      }
    }

    // calculate - go over mandatory and check if it completed
    for (const auto &taskName : obj->vecMandatoryTasks)
    {
      if (obj->mapTasks[taskName].isComplete)
        countMandatoryComplete++;
    }

    if (mandatoryTaskCountInObjective <= countMandatoryComplete)
    {
      obj->isComplete = true;

      obj->storeCoreAttribAsProperties(); // update the property "table"
    }
    else
    {
      obj->storeCoreAttribAsProperties(); // update the property "table"
    }


  } // end if we need to test objective

  inSmPropSeedValues.mapProperties.erase(mxconst::get_EXT_MX_CURRENT_OBJ()); // remove current objective
}

// -------------------------------------

bool
missionx::Mission::flc_task(const std::string& inTaskName, Objective& obj, mxProperties& inSmPropSeedValues)
{
#ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), inTaskName, false);
#endif // TIMER_FUNC

  // check task exists
  if (!Utils::isElementExists(obj.mapTasks, inTaskName))
  {
    Log::logMsgErr("[Critical]Fail to find task: " + mxconst::get_QM() + inTaskName + mxconst::get_QM() + ", in objective: " + mxconst::get_QM() + obj.name + mxconst::get_QM() + ". Check mission file settings. Mission might fail to execute as expected.\nNotify developer.");
    return false;
  }

  Task& task = obj.mapTasks[inTaskName];

  assert(!task.node.isEmpty() && " Task node must not be empty."); // v3.0.241.1

  #ifndef RELEASE
  // Log::logMsg ( fmt::format ( "Evaluating Task: [{}], is_enabled: {}, is_complete: {}", inTaskName, task.bIsEnabled, task.isComplete ) );
  #endif


  task.hasBeenEvaluated = true; // for debug and monitoring in scripts

  // check if we can skip task tests and return status.
  if (!task.bIsEnabled && (task.vecDepOnMe.empty()))
    return task.isComplete; // return the state of the task and skip rest of code

  // gather task information and dispatch code

  // Skip if task is in state: undefined
  if (task.task_type == missionx::mx_task_type::undefined_task)
  {
    return false;
  } // end if task has written type/state



  if ((task.bForceEvaluationOfTask || !task.isComplete) && task.bIsEnabled)
  {
    inSmPropSeedValues.setStringProperty(mxconst::get_EXT_MX_CURRENT_TASK(), inTaskName); // prepare seed attributes


    switch (task.task_type)
    {
      // TRIGGER based task
      case missionx::mx_task_type::trigger:
      {
        const std::string trigName = Utils::trim(task.action_code_name); // should hold trigger name
        if (trigName.empty())
        {
          Log::logMsgErr("Task: " + mxconst::get_QM() + inTaskName + mxconst::get_QM() + ", does not have trigger defined. Check mission and notify developer !!!");
          return false;
        }
        if (!Utils::isElementExists(missionx::data_manager::mapTriggers, trigName))
        {
          Log::logMsgErr("Trigger: " + mxconst::get_QM() + trigName + mxconst::get_QM() + ", does not exists, check mission definitions and fix !!!");
          return false;
        }


        // check trigger ENABLED
        if (Utils::readBoolAttrib(missionx::data_manager::mapTriggers[trigName].node, mxconst::get_ATTRIB_ENABLED(), true))
        {
          // call trigger FLC
          if (this->flc_trigger(missionx::data_manager::mapTriggers[trigName], inSmPropSeedValues, task.isMandatory))
          {
            #ifndef RELEASE
            Utils::xml_print_node ( task.node );
            #endif

          }

        } // end if trigger enabled

        // execute POST trigger script
        const std::string scriptName = Utils::readAttrib(missionx::data_manager::mapTriggers[trigName].node, mxconst::get_ATTRIB_POST_SCRIPT(), "");                                   // 3.0.241.1
        missionx::data_manager::execScript(scriptName, inSmPropSeedValues, "[flc_task] Post Trigger script: " + scriptName + " is Invalid. Aborting Mission"); // can alter success flag

        if (missionx::data_manager::mapTriggers[trigName].bAllConditionsAreMet)
        {
          task.setIsTaskComplete(true);

          task.setTaskState(missionx::mx_task_state::success);
        }
        else
        {
          task.setIsTaskComplete(false);
          if (task.taskState == missionx::mx_task_state::success)
            task.setTaskState(missionx::mx_task_state::was_success);
        }

        missionx::data_manager::mapTriggers[trigName].progressTriggerStates(); // so trigger won't be stack in "enter" or "left"


      } // end if Trigger
      break;

      // SCRIPT based task
      case missionx::mx_task_type::script:
      {
        // call script
        std::string scriptName = Utils::trim(task.action_code_name);

        if (scriptName.empty())
        {
          Log::logMsgWarn("Task Base Script name is empty. Check mission and notify developer.");
          task.task_type = missionx::mx_task_type::undefined_task;
          return false;
        }


        task.bScriptCondMet = false;                                                                                                                         // reset state to make sure script really modified the flag state.
        missionx::data_manager::execScript(scriptName, inSmPropSeedValues, "[flc_task] Task based script: " + scriptName + " is invalid. Aborting Mission"); // script should directly modify Task state when no timer is involved or .
        Utils::xml_set_attribute_in_node<bool>(task.node, mxconst::get_PROP_SCRIPT_COND_MET_B(), task.bScriptCondMet, mxconst::get_ELEMENT_TASK());                      // v3.0.241.1 update the XML

        //// check task timer flag
        bool timerEnded = true;
        if (task.node.isAttributeSet(mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC().c_str()))
        {
          const int timeSec = Utils::readNodeNumericAttrib<int>(task.node, mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), 0);
          if (timeSec > 0)
          {
            if ((task.bScriptCondMet && !task.isComplete) || (task.bForceEvaluationOfTask && task.bScriptCondMet))
            {
              if (task.timer.getState() == mx_timer_state::timer_not_set) // start timer
              {
                Timer::start(task.timer, (float)timeSec, task.name + "_timer");
              }
            }
            else if ((!task.bScriptCondMet && task.bForceEvaluationOfTask) || (!task.bScriptCondMet && !task.isComplete))
            {
              // reset timer
              if (task.timer.getState() > mx_timer_state::timer_not_set) // reset timer
              {
                task.timer.reset();
                Log::logMsg("[trigger]Reset timer.");
              }
            }

            // test timer and modify "task complete state"
            timerEnded = Timer::wasEnded(task.timer);

          } // end if timer > 0
        }   // end timer test

        // v3.0.95 - re-wrote to be in one logic
        if (timerEnded && task.bScriptCondMet)
        {
          task.setIsTaskComplete(true);
          task.setTaskState(missionx::mx_task_state::success);
        }
        else
        {
          task.setIsTaskComplete(false);

          if (task.taskState == missionx::mx_task_state::success)    // if was success
            task.setTaskState(missionx::mx_task_state::was_success); // monitor progress and understand history of task state
        }
      }
      break;

      // PLACEHOLDER - does nothing and just waits that other code will set it to complete
      case missionx::mx_task_type::placeholder:
      {
        task.bScriptCondMet = true;
      }
      break;


      case missionx::mx_task_type::base_on_command:
      {
        // 1. check if command is valid.
        // 2.0 if command not valid try to remap it.
        // 2.1 if failed to remap then flag task as success
        // 3.0 if command is valid - check if was clicked
        // if was clicked and time has passed, then flag it as success


        // check command is valid and try to remap it if not.
        if (task.command_ref.ref == nullptr)
        {
          task.command_ref.ref = XPLMFindCommand(task.command_ref.getName().c_str());
          if (task.command_ref.ref == nullptr)
          {
            task.setIsTaskComplete(true);
            task.setTaskState(missionx::mx_task_state::failed);
            task.errReason = "[Task " + task.name + "] Failed due to command not exists: " + task.command_ref.getName() + ". Fix the task settings.";
            return false;
          }
        }

        task.bScriptCondMet = true; // since there are no scripts we can flag this as always success

        // flag task as active if not already flagged
        if (!task.command_ref.flag_isActive)
        {
          task.command_ref.flag_isActive = true;
        }

        task.command_ref.flc(); // call command flc()


        //// check task timer flag
        bool timerEnded = true;
        if (task.node.isAttributeSet(mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC().c_str()) && task.command_ref.flag_wasClicked)
        {
          int timeSec = Utils::readNodeNumericAttrib<int>(task.node, mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), 0);
          if (timeSec > 0)
          {
            if ((task.bScriptCondMet && !task.isComplete) || (task.bForceEvaluationOfTask && task.bScriptCondMet))
            {
              if (task.timer.getState() == mx_timer_state::timer_not_set) // start timer
              {
                Timer::start(task.timer, (float)timeSec, task.name + "_timer");
              }
            }
            else if ((!task.bScriptCondMet && task.bForceEvaluationOfTask) || (!task.bScriptCondMet && !task.isComplete))
            {
              // reset timer
              if (task.timer.getState() > mx_timer_state::timer_not_set) // reset timer
              {
                task.timer.reset();
                Log::logMsg("[trigger]Reset timer.");
              }
            }

            // test timer and modify "task complete state"
            timerEnded = Timer::wasEnded(task.timer);

          } // end if timer > 0
        }   // end timer test

        // decide how to flag the task
        if (timerEnded && task.command_ref.flag_wasClicked)
        {
          task.bScriptCondMet = true;

          task.setIsTaskComplete(true);
          task.setTaskState(missionx::mx_task_state::success);
        }
      }
      break;
      case missionx::mx_task_type::base_on_external_plugin:
      {
        const int status_i                  = XPLMGetDatai(XPLMFindDataRef(missionx::drefConst.dref_xpshared_target_status_i_str.c_str()));
        const int flag_someoneIsListening_i = XPLMGetDatai(XPLMFindDataRef(missionx::drefConst.dref_xpshared_target_listen_plugin_available_i_str.c_str()));

        task.bScriptCondMet = true;

        if (flag_someoneIsListening_i)
        {
          if (status_i == 1)
          {
            task.setIsTaskComplete(true);
            task.setTaskState(missionx::mx_task_state::success);
          }
          else if (status_i == -1)
          {
            task.setIsTaskComplete(true);
            task.setTaskState(missionx::mx_task_state::failed);

            missionx::data_manager::setAbortMission("You failed in the external plugin task. Try again next time.");
          }
        }
      }
      break;

      case missionx::mx_task_type::base_on_sling_load_plugin:
      {
        bool        flag_abort_mission = false;
        std::string abort_mission_reason;
        std::string fail_task_reason;

        // fetch if task runs for the first time
        //   if first time (PROP_IS_FIRST_TIME) than we need to fetch the type of sling load and apply the nesesarry datarefs
        //   reset first time
        // for cargo: check position of cargo, if it is in the radius of 10 meters of the target location and the winch was cut then it should mean: SUCCESS

        missionx::dataref_param sling_cargo_start_lat(mxconst::get_DREF_EXTERNAL_HSL_CARGO_SET_LATITUDE());
        missionx::dataref_param sling_cargo_start_lon(mxconst::get_DREF_EXTERNAL_HSL_CARGO_SET_LONGITUDE());
        missionx::dataref_param sling_cargo_mass(mxconst::get_DREF_EXTERNAL_HSL_CARGO_MASS());
        missionx::dataref_param sling_cargo_pos_lat(mxconst::get_DREF_TARGET_POS_LAT_D());
        missionx::dataref_param sling_cargo_pos_lon(mxconst::get_DREF_TARGET_POS_LON_D());
        missionx::dataref_param sling_cargo_connected(mxconst::get_DREF_EXTERNAL_HSL_CARGO_CONNECTED());

        if (sling_cargo_start_lat.flag_paramReadyToBeUsed && sling_cargo_start_lon.flag_paramReadyToBeUsed && sling_cargo_mass.flag_paramReadyToBeUsed 
            && sling_cargo_pos_lat.flag_paramReadyToBeUsed && sling_cargo_pos_lon.flag_paramReadyToBeUsed && sling_cargo_connected.flag_paramReadyToBeUsed
           )
        {
          // Check and handle first time
          if (Utils::readBoolAttrib(task.node, mxconst::get_PROP_IS_FIRST_TIME(), true))
          {
            task.node.updateAttribute("false", mxconst::get_PROP_IS_FIRST_TIME().c_str(), mxconst::get_PROP_IS_FIRST_TIME().c_str()); // reset to false

            bool              flag_found  = false;
            const std::string init_script = Utils::xml_get_attribute_value_drill(task.node, mxconst::get_ATTRIB_INIT_SCRIPT(), flag_found, mxUtils::trim(Utils::readAttrib(task.node, mxconst::get_ATTRIB_BASE_ON_SLING_LOAD(), "")));

            if (init_script.empty()) // use start/end attributes if there is no init_script
            {
              // validate that we have the cargo starting position
              if (task.pSlingStart.getLat() == 0.0 || task.pSlingStart.getLon() == 0.0)
              {
                if (task.isMandatory)
                {
                  flag_abort_mission   = true;
                  abort_mission_reason = "[Task " + task.name + "] " + "\"Sling Load\" starting position is not set. aborting the mission due to the fact that the task is MANDATORY.";
                }
                else
                {
                  fail_task_reason = "[Task " + task.name + "] " + "Sling Load starting position is not set. Will flag the task as success since it is not mandatory.";
                  Log::logMsgErr(fail_task_reason.data());
                  task.setIsTaskComplete(true);
                  task.setTaskState(missionx::mx_task_state::success);
                  task.errReason = fail_task_reason;
                  return false;
                }

              } // end validate starting position of cargo

              // set start cargo position in both "set Latitude/Longitude" and in new custom dataref: "cargo pos lat/lon"
              sling_cargo_start_lat.setValue(task.pSlingStart.getLat());
              dataref_param::set_dataref_values_into_xplane(sling_cargo_start_lat);

              sling_cargo_start_lon.setValue(task.pSlingStart.getLon());
              dataref_param::set_dataref_values_into_xplane(sling_cargo_start_lon);

              sling_cargo_pos_lat.setValue(task.pSlingStart.getLat());
              dataref_param::set_dataref_values_into_xplane(sling_cargo_pos_lat);

              sling_cargo_pos_lon.setValue(task.pSlingStart.getLon());
              dataref_param::set_dataref_values_into_xplane(sling_cargo_pos_lon);

              const double mass_kg = Utils::readNodeNumericAttrib<double>(task.node, mxconst::get_ATTRIB_WEIGHT_KG(), 0.0);
              if (mass_kg > 0.0)
              {
                sling_cargo_mass.setValue(mass_kg);
                dataref_param::set_dataref_values_into_xplane(sling_cargo_mass);
              }

              // fire command "disable + enable HSL" and "place cargo in custom position"
              const std::string commands = mxconst::get_CMD_EXTERNAL_HSL_DISABLE_SLING_LOAD() + "," + mxconst::get_CMD_EXTERNAL_HSL_ENABLE_SLING_LOAD() + "," + mxconst::get_CMD_EXTERNAL_HSL_CARGO_LOAD_ON_COORDINATES();
              data_manager::execute_commands(commands); // will run next flight loop call
            }
            else
            {
              missionx::data_manager::execScript(init_script, inSmPropSeedValues, "[flc_task] init_script for Task based sling: " + init_script + " is invalid. Aborting Mission"); // script should directly modify Task state and initialize any dataref
            }

          } // end first time test
          else
          {
            // get cargo position
            // get connection state
            // test distance to target position
            bool              flag_found    = false;
            const std::string cond_script_s = Utils::xml_get_attribute_value_drill(task.node, mxconst::get_ATTRIB_COND_SCRIPT(), flag_found, mxUtils::trim(Utils::readAttrib(task.node, mxconst::get_ATTRIB_BASE_ON_SLING_LOAD(), "")));

            Point pCargo(sling_cargo_pos_lat.getValue<double>(), sling_cargo_pos_lon.getValue<double>()); // cargo position

            if (cond_script_s.empty())
            {


              if (task.pSlingEnd.getLat() + task.pSlingEnd.getLon() == 0.0)
              {
                flag_abort_mission   = true;
                abort_mission_reason = "[Task " + task.name + "] " + "Cargo target location is not set correctly. Fix <task> settings in mission file or notify the mission creator. Aborting mission. !!!";
              }
              else
              {

                // check cargo is not connected
                if (sling_cargo_connected.getValue<int>() == 0) // if cargo is not connected we can test for success
                {
                  // check distance between "cargo and destination" coordination, is it in the success radius ?
                  if (pCargo.calcDistanceBetween2Points(task.pSlingEnd, missionx::mx_units_of_measure::meter) < mxconst::SLING_LOAD_SUCCESS_RADIUS_MT)
                  {
                    task.bAllConditionsAreMet = true;
                  }
                  else
                  {
                    task.bAllConditionsAreMet = false;
                  }
                } // end if cargo is not connected
                else
                  task.bAllConditionsAreMet = false;


                // Evaluate time in success area: ATTRIB_EVAL_SUCCESS_FOR_N_SEC
                const int timeSec = Utils::readNodeNumericAttrib<int>(task.node, mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), 0);

                if (timeSec > 0)
                {
                  if (task.bAllConditionsAreMet) // cargo is in position and not connected
                  {
                    if (task.timer.getState() == mx_timer_state::timer_not_set) // start timer
                    {
                      Timer::start(task.timer, (float)timeSec, task.name + "_timer");
                      Log::logDebugBO("[Task]Started timer for task: " + task.name); // debug
                    }
                    else if (task.timer.getState() == mx_timer_state::timer_paused) // v3.0.221.11 continue timer if is cumulative
                    {
                      Timer::unpause(task.timer);
                      Log::logDebugBO("[Task]Unpaused timer for task: " + task.name + ". Cumulative time: " + Utils::formatNumber<float>(task.timer.getCumulativeXplaneTimeInSec())); // debug
                    }

                    // Evaluate timer
                    if (!Timer::wasEnded(task.timer))
                      task.bAllConditionsAreMet = false;
                  }
                  else
                  {
                    // reset task timer if timer was active because cargi is either not in position or rope is still connected to it
                    if (task.timer.getState() == mx_timer_state::timer_running) // reset timer only if it is running
                    {
                      if (task.timer.getIsCumulative()) // add cumulative timer support
                      {
                        task.timer.pause();
                        Log::logDebugBO("[task]Paused timer for task: " + task.name + ". Cumulative time so far: " + Utils::formatNumber<float>(task.timer.getCumulativeXplaneTimeInSec()));
                      }
                      else
                      {
                        task.timer.reset();
                        Log::logDebugBO("[task]Reset timer for task: " + task.name);
                      }
                    }
                  }
                }

                //////////////////////////////////////////////
                // DECIDE if task is completed and successful
                task.setIsTaskComplete(task.bAllConditionsAreMet);
                task.setTaskState((missionx::mx_task_state)( static_cast<int> ( missionx::mx_task_state::success ) * task.bAllConditionsAreMet + (int)missionx::mx_task_state::need_evaluation * task.bAllConditionsAreMet));


              } // end evaluate task success/failure based on cargo position
            }
            else
            {
              inSmPropSeedValues.setNumberProperty(mxconst::get_EXT_mxCargoPosLat(), pCargo.getLat()); // prepare seeded cargo position attributes
              inSmPropSeedValues.setNumberProperty(mxconst::get_EXT_mxCargoPosLon(), pCargo.getLon()); // prepare seeded cargo position attributes


              // script should take care of all Cargo tests and decide if it can be flagged as success using task properties
              if (missionx::data_manager::execScript(cond_script_s, inSmPropSeedValues, "Task based \"Sling Load\" has a cond_script with errors: " + cond_script_s + ". Please fix the errors: "))
              {
                // When using cond_script for the sling cargo based task, we test against bScriptCondMet to determine task state
                task.setTaskState((missionx::mx_task_state)((int)missionx::mx_task_state::success * task.bScriptCondMet + (int)missionx::mx_task_state::need_evaluation * task.bScriptCondMet));
              }

              inSmPropSeedValues.removeProperty(mxconst::get_EXT_mxCargoPosLat()); // remove seeded cargo position attributes
              inSmPropSeedValues.removeProperty(mxconst::get_EXT_mxCargoPosLon()); // remove seeded cargo position attributes
            }


          } // end evaluating If this is the tasks first time evaluation
        }
        else
        {
          // Always abort mission if one of the tasks tries to use an external plugin that is not available or have missing datarefs.
          flag_abort_mission   = true;
          abort_mission_reason = "[Task " + task.name + "] " + "Mandatory \"Sling Load\" plugin datarefs are missing. Check if Sling Load plugin is installed. If it does not work, notify the developer!!!";
        }


        if (flag_abort_mission)
        {
          Log::logMsgErr(abort_mission_reason.data());

          data_manager::setAbortMission(abort_mission_reason.data());
          return false;
        } // end abort mission



      } // end sling load task type
      break;

      default: // task type default switch
      {
        inSmPropSeedValues.mapProperties.erase(mxconst::get_EXT_MX_CURRENT_TASK()); // v3.305.2 need to clean before returning false
        return false;
      }
      break;


    } // end switch TASK Type

    inSmPropSeedValues.mapProperties.erase(mxconst::get_EXT_MX_CURRENT_TASK());
  } // end if need to evaluate or task was not complete



  ////////////////////////////////////////////////
  // check if complete and if need to drill down
  if ((task.isComplete && task.taskState != missionx::mx_task_state::failed) /*|| ((trig != nullptr) && (trig->trigState > missionx::mx_trigger_state_enum::never_triggered))*/ // if state is GEATER than "was not triggered"
      + ((task.taskState > missionx::mx_task_state::need_evaluation))                                                                                                            // if state is GEATER than "need_evaluation", which means it was never in success state.
      )                                                                                                                                                                        // if complete or was triggered or was_task_success
  {
    if (task.vecDepOnMe.empty()) // the specific task was finished
      return task.isComplete;

    // loop over dependent tasks, and ALL their sub tasks. All should return true (complete)
    // Please remember that we only count the mandatory tasks to flag the objective as success/fail

    const auto lmbda_check_all_dependent_tasks_are_completed = [&](std::string inSubTaskName)
    {
      if (Mission::flc_task(inSubTaskName, obj, inSmPropSeedValues)) // start recursion
        return true;

      task.listOfIncompleteDependentTasks_s = (task.listOfIncompleteDependentTasks_s.empty()) ? inSubTaskName : task.listOfIncompleteDependentTasks_s + "," + inSubTaskName;
      return false;
    };

    const bool bAllDependentTasksAreComplete = all_of(begin(task.vecDepOnMe), end(task.vecDepOnMe), lmbda_check_all_dependent_tasks_are_completed);

    return bAllDependentTasksAreComplete;
  } // end if complete


  return false;
}

// -------------------------------------

// The function only test physical location, elevation and script.
// There are still timer tests, post_script and "always run" script
bool
missionx::Mission::flc_trigger(Trigger& trig, mxProperties& inSmPropSeedValues, const bool& isTaskMandatory)
{
#ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), trig.getName(), false);
#endif // TIMER_FUNC

  std::string err;
  err.clear();

  // v25.02.1 used for data_manager::set_success_or_reset_tasks_state() function.
  const std::string currentObj  = (mxUtils::isElementExists(inSmPropSeedValues.mapProperties, mxconst::get_EXT_MX_CURRENT_OBJ())) ? inSmPropSeedValues.getPropertyValue(mxconst::get_EXT_MX_CURRENT_OBJ(), err) : "";
  const std::string currentTask = (mxUtils::isElementExists(inSmPropSeedValues.mapProperties, mxconst::get_EXT_MX_CURRENT_TASK())) ? inSmPropSeedValues.getPropertyValue(mxconst::get_EXT_MX_CURRENT_TASK(), err) : ""; 

  //// TRIGGER ////
  #ifndef RELEASE
    std::string trigName = trig.getName();
    // Log::logMsg ( fmt::format ( "\tEvaluating trigger: {}, is_enabled: {}, bAllCondMet: {}", trigName, data_manager::mapTriggers[trigName].isEnabled, data_manager::mapTriggers[trigName].bAllConditionsAreMet ) );
  #endif

  const std::string trigType = Utils::readAttrib(trig.node, mxconst::get_ATTRIB_TYPE(), ""); // v3.0.241.1

  inSmPropSeedValues.setStringProperty(mxconst::get_EXT_MX_CURRENT_TRIGGER(), trig.getName()); // set seeded property for trigger name

  const auto lmbda_get_trigger_current_location_point = [&](const std::string& inTrigType) {
    return (mxconst::get_TRIG_TYPE_CAMERA() == inTrigType) ? missionx::dataref_manager::getCameraPointLocation() : missionx::dataref_manager::getPlanePointLocationThreadSafe();
  }; // check NavInfo if it has the "flag_picked_random_lat_long == true"

  Point pCurrentLocation_point = lmbda_get_trigger_current_location_point(trigType);

  /////////////////////////////////////////////////////
  // check conditions plane/camera in physical area
  ////////////////////////////////////////////////////
  trig.eval_physical_conditions_are_met(pCurrentLocation_point); // v3.305.1c test physical/elevation/onGround logic


  // Check "cond_script" if in physical area
  if (trig.flag_inPhysicalArea && trig.flag_inRestrictElevationArea)
  {
    trig.bPlaneIsInTriggerArea = true; // v3.305.1c

    // Execute condition script
    const std::string condScriptName = Utils::readAttrib(trig.xConditions, mxconst::get_ATTRIB_COND_SCRIPT(), ""); // v3.0.241.1 read from XML node, this fixes the no value in ATTRIB_COND_SCRIPT property
    if (condScriptName.empty())
    {
      trig.bScriptCondMet = true;
    }
    else
    {
      trig.bScriptCondMet = false; // v25.03.1 Force false, since we expect the script to set it to true.
      missionx::data_manager::execScript(condScriptName, inSmPropSeedValues, "[flc_trigger] Triggers condition script: " + condScriptName + " is invalid. Aborting Mission"); // script should directly modify Task state.
    }

    if (trig.flag_isOnGround && !trig.bEnteringPhysicalAreaMessageFiredOnce)
    {
      // v3.305.1b check if we need to call "entering physical area message"
      const auto messageName = Utils::readAttrib(trig.xOutcome, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_ENTERING_PHYSICAL_AREA(), "");
      if (!messageName.empty())
      {
        #ifndef RELEASE
        Log::logMsg("Adding message: " + messageName + ", from Trigger: " + trig.getName());
        #endif // !RELEASE
        missionx::QueueMessageManager::addMessageToQueue(messageName, EMPTY_STRING, err);
      }

      trig.bEnteringPhysicalAreaMessageFiredOnce = true; // don't repeat

    }

  }

  
  //////////////////////////////////////////////////////////
  // Check physical/elevation/ground and script conditions
  /////////////////////////////////////////////////////////
  trig.setAllConditionsAreMet_flag(trig.eval_all_conditions_are_met_exclude_timer()); // set "bAllConditionsAreMet" based on the different trigger logic

  /////////////////////////////////////////
  // Check the last condition: timer
  ////////////////////////////////////////
  // did stopper ended ? In some triggers we ask to call "enter event" after N seconds.
  const int timeSec = static_cast<int> ( Utils::readNumericAttrib ( trig.node, mxconst::get_ATTRIB_EVAL_SUCCESS_FOR_N_SEC(), 0.0 ) );
  if (timeSec > 0)
  {
    if (trig.bAllConditionsAreMet) // if trigger physical tests are true
    {
      if (trig.timer.getState() == mx_timer_state::timer_not_set) // start timer
      {
        Timer::start(trig.timer, static_cast<float> ( timeSec ), trig.getName() + "_timer");
        Log::logDebugBO("[trigger]Started timer for trig: " + trig.getName());
      }
      else if (trig.timer.getState() == mx_timer_state::timer_paused) // v3.0.221.11 continue timer if is cumulative
      {
        Timer::unpause(trig.timer);
        Log::logDebugBO("[trigger]un-paused timer for trig: " + trig.getName() + ". Cumulative time: " + Utils::formatNumber<float>(trig.timer.getCumulativeXplaneTimeInSec()));
      }

      // v25.02.1 we will store success timer info, only for mandatory tasks based on triggers
      if (isTaskMandatory && missionx::data_manager::strct_success_timer_info.prevRemainingTime_f > trig.timer.getRemainingTime())
      {
        missionx::data_manager::strct_success_timer_info.vecTriggersWithActiveTimers.emplace_back(trig.getName()); // v25.02.1 Only add the trigger if the success timer is running
        missionx::data_manager::strct_success_timer_info.triggerNameWithShortestSuccessTimer = trig.getName();
        missionx::data_manager::strct_success_timer_info.prevRemainingTime_f                 = trig.timer.getRemainingTime();
      }
    } // end if trig.bAllConditionsAreMet
    else
    {
      if (trig.timer.getState() == mx_timer_state::timer_running) // reset timer only if it is running
      {
        if (trig.timer.getIsCumulative()) // v3.0.221.11 add cumulative timer support
        {
          trig.timer.pause();
          Log::logDebugBO("[trigger]Paused timer for trigger: " + trig.getName() + ". Cumulative time so far: " + Utils::formatNumber<float>(trig.timer.getCumulativeXplaneTimeInSec()));
        }
        else
        {
          trig.timer.reset();
          Log::logDebugBO("[trigger]Reset timer for trigger: " + trig.getName());
        }
      }
    }

    // Evaluate timer
    if (!Timer::wasEnded(trig.timer))
      trig.setAllConditionsAreMet_flag(false);

  }


  // v3.305.3 added force debug logic
  if (trig.strct_debug.bForceDebug)
  {
    trig.strct_debug.store_and_switch_real_state(trig.trigState);
    trig.bAllConditionsAreMet = trig.strct_debug.eval_allConditionAreMetBasedOnDebugFlagAndState();
  }

  // ------------------------------
  // ------------------------------
  // -------Fire the Enter/Exit----
  // -------------Events-----------
  // ------------------------------
  // ------------------------------
  if (trig.bAllConditionsAreMet) 
  {
    // For DEBUG tab
    trig.strct_debug.setWasEvaluatedSuccessfully(true);

    // modify trigger state
    switch (trig.trigState)
    {
      case missionx::mx_trigger_state_enum::never_triggered:
      case missionx::mx_trigger_state_enum::wait_to_be_triggered_again:
      {
        #ifndef RELEASE
        Log::logMsg("[" + std::string(__func__) + "] Firing Entered trigger: " + trig.getName());
        #endif // !RELEASE
        trig.strct_debug.sStartExecutionTime = Timer::get_debugTimestamp();

        trig.setTrigState(mx_trigger_state_enum::trig_fired); // v3.0.241.1


        std::string scriptName = Utils::readAttrib(trig.xOutcome, mxconst::get_ATTRIB_SCRIPT_NAME_WHEN_FIRED(), ""); // v3.0.241.1

        scriptName = mxUtils::replaceAll(scriptName, mxconst::REPLACE_KEYWORD_SELF, trig.getName()); // v3.305.3
        missionx::data_manager::execScript(scriptName, inSmPropSeedValues, "[flc_trigger] Trigger fired script: " + scriptName + " is invalid. Aborting Mission");

        const std::string messageName = Utils::readAttrib(trig.xOutcome, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED(), ""); // v3.0.241.1
        missionx::QueueMessageManager::addMessageToQueue(messageName, EMPTY_STRING, err);

        // v3.0.221.11+
        const std::string commands = Utils::readAttrib(trig.xOutcome, mxconst::get_ATTRIB_COMMANDS_TO_EXEC_WHEN_FIRED(), ""); // v3.0.241.1
        data_manager::execute_commands(commands);

        // v3.0.221.11+
        const std::string datarefs = Utils::readAttrib(trig.xOutcome, mxconst::get_ATTRIB_DATAREF_TO_EXEC_WHEN_FIRED(), ""); // v3.0.241.1
        data_manager::apply_datarefs_based_on_string_parsing(datarefs);                                                // v3.303.12


        // v3.303.12
        if (!trig.xSetDatarefs.isEmpty())
          missionx::data_manager::apply_datarefs_from_text_based_on_parent_node_and_tag_name(trig.xSetDatarefs, "");

        // v25.02.1 - handle success of other Task states
        const auto set_success_task_list_s = Utils::readAttrib ( trig.xOutcome, mxconst::get_ATTRIB_SET_OTHER_TASKS_AS_SUCCESS(), "");
        if (!set_success_task_list_s.empty())
          missionx::data_manager::set_success_or_reset_tasks_state(Mission::currentObjectiveName, set_success_task_list_s, missionx::enums::mx_action_from_trigger_enum::set_success, currentTask);

        // v25.02.1 - handle reset of other Tasks
        const auto reset_task_list_s = Utils::readAttrib ( trig.xOutcome,mxconst::get_ATTRIB_RESET_OTHER_TASKS_STATE(), "");
        if (!reset_task_list_s.empty())
          missionx::data_manager::set_success_or_reset_tasks_state(Mission::currentObjectiveName, reset_task_list_s, missionx::enums::mx_action_from_trigger_enum::reset, currentTask);

        // v25.02.1 - set other triggers as success
        const auto set_other_trigger_as_success_list_s = Utils::readAttrib ( trig.xOutcome,mxconst::get_ATTRIB_SET_OTHER_TRIGGERS_AS_SUCCESS(), "");
        if (!set_other_trigger_as_success_list_s.empty())
          missionx::data_manager::set_trigger_state(trig, set_other_trigger_as_success_list_s, missionx::enums::mx_action_from_trigger_enum::set_success);


        trig.strct_debug.sEndExecutionTime = Timer::get_debugTimestamp();
      }
      break;
      default:
        break; // do nothing
    }

  }    // end if trigger bAllConditionsMet
  else // timer ended represent we probably triggered "entered" once.
  {    // start NOT all trigger conditions are met

    // handle by trigger state
    switch (trig.trigState)
    {
      case missionx::mx_trigger_state_enum::trig_fired: // v3.0.217.6 fixed bug where "trig.trigState" was never set to "inside_trigger_zone". Instead it was set to "trig_fired" so code never triggered when left the area.
      case missionx::mx_trigger_state_enum::inside_trigger_zone:
      {
        #ifndef RELEASE
        Log::logMsg("[" + std::string(__func__) + "] Firing Leaving trigger: " + trig.getName());
        #endif // !RELEASE
          
        if (trig.bPlaneIsInTriggerArea + trig.strct_debug.bForceDebug) // v3.305.3 added bForceDebug logic to force outcome triggering// in order to leave, we need to at least be in the trigger envelop area (volume)
        {
          trig.strct_debug.sStartExecutionTime = Timer::get_debugTimestamp();

          trig.setTrigState(mx_trigger_state_enum::left); // v3.0.241.1


          std::string scriptName = Utils::readAttrib(trig.xOutcome, mxconst::get_ATTRIB_SCRIPT_NAME_WHEN_LEFT(), ""); // v3.0.241.1

          scriptName = mxUtils::replaceAll(scriptName, mxconst::REPLACE_KEYWORD_SELF, trig.getName()); // v3.305.3
          missionx::data_manager::execScript(scriptName, inSmPropSeedValues, "[flc_trigger] Triggers script when exiting area: " + scriptName + " is invalid. Aborting Mission");

          const std::string messageName = Utils::readAttrib(trig.xOutcome, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_LEFT(), ""); // v3.0.241.1
          missionx::QueueMessageManager::addMessageToQueue(messageName, "", err);

          // v3.0.221.11+
          const std::string commands = Utils::readAttrib(trig.xOutcome, mxconst::get_ATTRIB_COMMANDS_TO_EXEC_WHEN_LEFT(), ""); // v3.0.241.1
          data_manager::execute_commands(commands);

          // v3.0.221.11+
          const std::string datarefs = Utils::readAttrib(trig.xOutcome, mxconst::get_ATTRIB_DATAREF_TO_EXEC_WHEN_LEFT(), ""); // v3.0.241.1
          data_manager::apply_datarefs_based_on_string_parsing(datarefs);                                               // v3.303.12

          // v3.303.12
          if (!trig.xSetDatarefs_on_exit.isEmpty())
            missionx::data_manager::apply_datarefs_from_text_based_on_parent_node_and_tag_name(trig.xSetDatarefs_on_exit, "");

          // v25.02.1 - handle reset of other Tasks
          const auto reset_task_list_s = trig.getStringAttributeValue(mxconst::get_ATTRIB_RESET_OTHER_TASKS_STATE(), "");
          if (!reset_task_list_s.empty())
            missionx::data_manager::set_success_or_reset_tasks_state(Mission::currentObjectiveName, reset_task_list_s, missionx::enums::mx_action_from_trigger_enum::reset, currentTask);


          if (!trig.timer.getIsCumulative() && !trig.strct_debug.bForceDebug) // v3.305.3 added skip reset() if forcedDebug // v3.0.241.7 if timer is not cumulative, we need to reset it for next entrance
            trig.timer.reset();

          trig.bPlaneIsInTriggerArea = false; // v3.305.1c reset

          trig.strct_debug.sEndExecutionTime = Timer::get_debugTimestamp();
        }
      }
      break;
      default:
        break; // do nothing
    }
  } // end NOT all conditions are met
   


  if (trig.strct_debug.bForceDebug)
  {
    Log::logMsg("After executing forced Trigger Event. " + trig.strct_debug.get_string_of_debug_info() + ", for trigger: " + trig.name); // debug

    trig.strct_debug.restore_real_trig_state(trig.trigState);  
  }

  // v3.0.219.1 RE-ARM trigger if is in "left" state
  if (trig.trigState == mx_trigger_state_enum::left && (Utils::readBoolAttrib(trig.node, mxconst::get_ATTRIB_RE_ARM(), false)))
  {
    trig.re_arm();
  }

  // POST trigger script must be called from calling function since it needs to be called in disabled state too


  return trig.bAllConditionsAreMet;
}

// -------------------------------------

void
missionx::Mission::flc_3d_objects(mxProperties& inSmPropSeedValues)
{
//#ifdef TIMER_FUNC
//  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
//#endif // TIMER_FUNC

  std::string            scriptName;
  std::string            instName;
  std::string            obj3dName;
  bool                   reachedFlightLegToHide;
  bool                   planeInDistance;
  bool                   linkedTaskIsActive;
  bool                   hideInstance;
  std::list<std::string> eraseInstanceList;

  std::string err;
  err.clear();

  eraseInstanceList.clear();

  // loop over 3d object instances and evaluate them (condition script, calculate distance to display, seed to script, check link to task status).
  for (auto & [key, instObj] : data_manager::map3dInstances)
  {
    instName  = key; //o.first;
    obj3dName = instObj.getName();
    scriptName.clear();
    reachedFlightLegToHide  = false;
    linkedTaskIsActive = false;
    hideInstance       = false;

    const bool flag_display_at_post_leg = Utils::readBoolAttrib(instObj.node, mxconst::get_ATTRIB_DISPLAY_AT_POST_LEG_B(), false); // v3.303.11 skip instance that has ATTRIB_DISPLAY_AT_POST_LEG_B == false. We will change them at the post flight leg
    IXMLNode   xInstanceDataNode        = (missionx::data_manager::mx_global_settings.flag_loadedFromSavepoint)?instObj.node.getChildNode(mxconst::get_PROP_INSTANCE_DATA_ELEMENT().c_str()) : IXMLNode::emptyIXMLNode; // do we have instance element ? if not then we might need to recalculate position of 3D instance object
    if (flag_display_at_post_leg)
      continue;

    inSmPropSeedValues.setStringProperty(mxconst::get_EXT_MX_CURRENT_3DOBJECT(), obj3dName);  // seed template instObj name
    inSmPropSeedValues.setStringProperty(mxconst::get_EXT_MX_CURRENT_3DINSTANCE(), instName); // seed instance name

    scriptName = Utils::readAttrib(instObj.node, mxconst::get_ATTRIB_COND_SCRIPT(), ""); //.getCondScript();
    // execute script.function to evaluate if to display 3D Object
    if (scriptName.empty())
      instObj.isScriptCondMet = true;
    else
      missionx::data_manager::execScript(scriptName, inSmPropSeedValues, "[flc_3d] 3D Instance condition script: " + scriptName + " is invalid. Aborting Mission"); // will seed mxFuncCall


    Point planePos = dataref_manager::getPlanePointLocationThreadSafe(); // split from original logic, for Linux compiler

    ////////////////////////////////////////////////////////
    // v3.303.11 add support for relative 3D Object location
    std::string relative_pos_bearing_deg_distance_mt_s = Utils::readAttrib(instObj.node, mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT(), "");
    // We need to calculate instance location if "relative_pos_bearing_deg_distance_mt_s" has value and (we started fresh mission OR we loaded from savepoint but there is no instance data yet) 
    if ((!missionx::data_manager::mx_global_settings.flag_loadedFromSavepoint || ( missionx::data_manager::mx_global_settings.flag_loadedFromSavepoint && xInstanceDataNode.isEmpty() ) ) && !relative_pos_bearing_deg_distance_mt_s.empty())
    {
      IXMLNode node_ptr = instObj.node;

      assert(!node_ptr.isEmpty() && "3D Instance node can't be empty");

      if (!node_ptr.isEmpty() && instObj.flagNeedToCalculateRelativePosition)
      {
        instObj.flagNeedToCalculateRelativePosition ^= 1; // change to false
        missionx::data_manager::translate_relative_3d_display_object_text(relative_pos_bearing_deg_distance_mt_s); // replace keywords with number values, like {wing_span}

        // adjust LAT/LON
        std::vector<std::string> vecRelativePos = mxUtils::split_v2(relative_pos_bearing_deg_distance_mt_s, mxconst::get_PIPE_DELIMITER()); // v3.0.219.1
        if (vecRelativePos.size() > 1)
        {
          double newLat, newLon;
          newLat = newLon = 0.0;

          if (planePos.lat != 0.0 && planePos.lon != 0.0)
          {
            // calculate new targetLat/long
            std::string    distance_expresion_s = vecRelativePos.at(1);
            missionx::calc c(distance_expresion_s);
            double         distance_nm = c.calculateExpression() * meter2nm;

            std::string    bearing_expresion_s = vecRelativePos.at(0);
            missionx::calc c2(bearing_expresion_s);
            float          bearing = (float)c2.calculateExpression();

            Utils::calcPointBasedOnDistanceAndBearing_2DPlane(newLat, newLon, planePos.lat, planePos.lon, bearing, distance_nm);

            // set new targetLat/long in instance replace point data
            const std::string newInstanceLat_s = Utils::formatNumber<double>(newLat, 8);
            const std::string newInstanceLon_s = Utils::formatNumber<double>(newLon, 8);
            Utils::xml_search_and_set_attribute_in_IXMLNode(node_ptr, mxconst::get_ATTRIB_LAT(), newInstanceLat_s, node_ptr.getName());
            Utils::xml_search_and_set_attribute_in_IXMLNode(node_ptr, mxconst::get_ATTRIB_LONG(), newInstanceLon_s, node_ptr.getName());

            instObj.displayCoordinate.setLat(newLat);
            instObj.displayCoordinate.setLon(newLon);
            instObj.displayCoordinate.calcSimLocalData();
          }
        }
      }

      node_ptr.updateAttribute(relative_pos_bearing_deg_distance_mt_s.c_str(), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS().c_str(), mxconst::get_ATTRIB_DEBUG_RELATIVE_POS().c_str()); // v3.303.14 Keep the value in a debug attribute
      node_ptr.updateAttribute("", mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT().c_str(), mxconst::get_ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT().c_str());
    } // end v3.303.11 implement ATTRIB_RELATIVE_POS_BEARING_DEG_DISTANCE_MT


    // decide if to display 3D Object instance based on distance. Part of cooling objects // AS of 27jan2018 -instanced object can't be set to hide. We need to "destroy" the instanced object. Waiting for LR to answer my questions
    // (https://developer.x-plane.com/code-sample/x-plane-10-instancing-compatibility-wrapper/)
    planeInDistance  = instObj.isPlaneInDisplayDistance(planePos);




    // check if reached flightLeg
    if (!data_manager::currentLegName.empty() && data_manager::currentLegName.compare(instObj.getPropKeepUntilLeg()) == 0) // v3.303.11 added ignore logic if currentLegName is empty, This occurs at the end of the flight
    {
      reachedFlightLegToHide = true;
      eraseInstanceList.push_back(instName);
    }

    // v3.0.207.4 check if to hide // v3.0.241.7 extended
    hideInstance = Utils::readBoolAttrib(instObj.node, mxconst::get_ATTRIB_HIDE(), false);

    const bool is_target_marker_flag = Utils::readBoolAttrib(instObj.node, mxconst::get_ATTRIB_TARGET_MARKER_B(), false);
    const bool setup_display_target_markers = Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS(), true);
    if (!setup_display_target_markers && is_target_marker_flag)
      hideInstance = true;


    // check if linked task is active. If not then hide.
    // check if objective.task exists
    const std::string objective = instObj.getPropLinkToObjectiveName();
    const std::string task_name = instObj.getPropLinkTask();
    if ( objective.empty() + task_name.empty() == 0) // v3.0.303.6
    {
      if (Utils::isElementExists(data_manager::mapObjectives, objective))
      {
        if (Utils::isElementExists(data_manager::mapObjectives[objective].mapTasks, task_name))
        {
          linkedTaskIsActive = data_manager::mapObjectives[objective].mapTasks[task_name].isActive();
        }
      }
    }
    else
      linkedTaskIsActive = true; // dummy since there is no dependency on task state


    ////////////////////////////////////////////////////////////////////////////
    // If 3D Object exists and plane is in range and script conditions are true
    if (planeInDistance && instObj.isScriptCondMet && !reachedFlightLegToHide && linkedTaskIsActive && !hideInstance)
    {
      // is instance in display set ?
      if (instObj.isInDisplayList) // v3.0.207.1
      {
        instObj.mvStat.flag_wait_for_next_flc = false; // reset state

        // in draw loop back, but we will try to just call it in Flight Loop Back for optimization
        instObj.mvStat.groundElevation = UtilsGraph::getTerrainElevInMeter_FromPoint(
          instObj.displayCoordinate, instObj.mvStat.probeResult); // v3.0.253.6 moved to flc_3d_object // v3.0.207.3
        continue;
      }
      else // first time instance is added it will start in this part of the code to intialize the 3D Object
      {
        // Create an instance reference
        instObj.create_instance();  // create instance from object reference
        if (instObj.g_instance_ref) // if instance was created then
        {

          // v3.0.207.2 // for moving objects get_next_point() which represent the destination
          if (instObj.obj3dType == obj3d::obj3d_type::moving_obj)
            instObj.initPathCycle_new();

          // check if we need to probe terrain
          if (instObj.displayCoordinate.getElevationInFeet() == 0.0) // do we need Terrain Probing ?
            instObj.calculate_real_elevation_to_DisplayCoordination();
          instObj.displayCoordinate.calcSimLocalData(); // calculate local coordination just in case



          // add 3D Object to the relevant list (part of optimization)
          missionx::data_manager::addInstanceNameToDisplayList(instName); // v3.0.207.1

          instObj.isInDisplayList = true; // v3.0.207.1 // set display flag //  removed function from older version

          instObj.positionInstancedObject(); // prepare XPLMDrawInfo_t and call XPLMInstanceSetPosition()

        } // end if instance is valid
      }
    }
    else // Destroy instance of 3D Object and remove from display list since it is not in range or condition failed
    {

      instObj.isInDisplayList = false; // v3.0.207.1 
      XPLMDestroyInstance(instObj.g_instance_ref);

      missionx::data_manager::delInstanceNameFromDisplayList(instName); // v3.0.207.1
    }
  }

  // v3.0.207.4 -  Erase instances based on eraseInstanceList # fix bug where instance will display itself after it reached its flightLeg.
  for (auto& itName : eraseInstanceList)
  {
    data_manager::map3dInstances.erase(itName);

#ifndef RELEASE
    // v3.0.303.6 erase 3D instance cue from listCueInfoToDisplayInFlightLeg
    missionx::data_manager::deleteCueFromListCueInfoToDisplayInFlightLeg(itName);
#endif
  }
  eraseInstanceList.clear();

  ///////////////////////////////////////////////////
  // v3.0.303.6 Calculate cue for 3D Object instances

#ifndef RELEASE
  for (auto& [instName, obj] : data_manager::map3dInstances)
  {
    obj.prepareCueMetaData();
    CueInfo cue = obj.getCopyOfCueInfo();
    
    if (cue.cueSeq <= 0)
    {
      ++missionx::data_manager::seqCueInfo;
      cue.cueSeq = missionx::data_manager::seqCueInfo;
      obj.getCueInfo_ptr().cueSeq = cue.cueSeq;
    }

    switch (obj.obj3dType)
    {
      case missionx::obj3d::obj3d_type::static_obj:
      {
        if (!cue.wasCalculated) // static + moving
        {
          missionx::Point pCenter = obj.getCurrentCoordination();
          cue.calculateCircleFromPoint(pCenter, true, true, missionx::NUM_CIRCLE_POINTS_3D_OBJ);
          obj.getCueInfo_ptr().wasCalculated = true;
          data_manager::mapFlightLegs[data_manager::currentLegName].listCueInfoToDisplayInFlightLeg.push_back(cue);
        }
      }
      break;      
      case missionx::obj3d::obj3d_type::moving_obj:
      {
        missionx::data_manager::deleteCueFromListCueInfoToDisplayInFlightLeg(instName);
        missionx::Point pCenter = obj.getCurrentCoordination();
        cue.calculateCircleFromPoint(pCenter, true, true, missionx::NUM_CIRCLE_POINTS_3D_OBJ);
        data_manager::mapFlightLegs[data_manager::currentLegName].listCueInfoToDisplayInFlightLeg.push_back(cue);
      }
      break;
    } // end switch
  } // end loop over all map3dInstances

#endif // !RELEASE

}

// -------------------------------------

void
missionx::Mission::flc_check_plane_in_external_inventory_area()
{
#ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), "check inventory area", false);
#endif // TIMER_FUNC

  bool foundInv = false; // flag if plane in inventory area

  // loop over all inventories and check if plane is in their area
  for (auto& [name, inv] : data_manager::mapInventories)
  {
    if (mxconst::get_ELEMENT_PLANE() == name) // v3.305.2 skip plane ???
      continue;

    //std::string errLoc = missionx::EMPTY_STRING;
    Point       p      = dataref_manager::getPlanePointLocationThreadSafe();
    Point       camera = dataref_manager::getCameraPointLocation();

    const bool cameraInArea = inv.isInPhysicalArea(camera); // v3.0.223.7 check if camera in area
    const bool inArea       = inv.isInPhysicalArea(p);
    //if (!errLoc.empty())
    //  continue;



    const bool cameraInElev = inv.isInElevationArea(camera); // v3.0.223.7 check camera in elevation
    const bool inElev       = inv.isInElevationArea(p);
    //if (!errLoc.empty())
    //  continue;

    // check if plane is in inventory area
    if ((inArea * inElev) + (cameraInArea * cameraInElev))
    {
      missionx::data_manager::active_external_inventory_name = name; // v3.0.251.1 store active inventory name in "data_manager" instead of the UI classes.

      Mission::uiImGuiBriefer->setExternalInventoryName(name);

      foundInv = true;
      break; // exit the loop
    }
  }

  // notify briefer and mxpad
  if (!foundInv)
  {
    missionx::data_manager::active_external_inventory_name.clear(); // v3.0.251.1 store active "inventory name" state in "data_manager" instead of the UI classes.
    Mission::uiImGuiBriefer->setExternalInventoryName("");
  }
}

// -------------------------------------

void
missionx::Mission::stopDataManagerAndClearScriptManager()
{
  missionx::data_manager::stopMission();
  missionx::script_manager::clear();
}


// -------------------------------------


void
missionx::Mission::drawCallback(const XPLMDrawingPhase& inPhase, const int inIsBefore, void* inRefcon)
{
  static int frameCounter = 0;

  // v3.0.303.4 calc shared missionx/obj3d/rotation_per_frame_1_min
  data_manager::mapSharedParams[missionx::dref::DREF_SHARED_DRAW_ROTATION].setValue(std::fmod(dataref_manager::getTotalRunningTimeSec(), missionx::SECONDS_IN_1MINUTE_F));
  missionx::dataref_param::set_dataref_values_into_xplane(data_manager::mapSharedParams[missionx::dref::DREF_SHARED_DRAW_ROTATION]);

  // Plane position
#ifdef MAC
  if ( this->drawPhase == inPhase || xplm_Phase_Window == inPhase || xplm_Phase_Objects == inPhase /*OpenGL*/ )
#else
  if ((this->drawPhase == inPhase /*What we set might not be Object/Modern phase*/) || (xplm_Phase_Objects == inPhase /*OpenGL*/) || (xplm_Phase_Modern3D == inPhase /*Vulkan/Metal*/))
#endif // MAC
  {
    // Fix for GL lines not drawing correctly. Here is Sydney explanation: This is not a terrain probe problem, instead it's a reverse-y problem when running HDR. The problem here is that X-Plane doesn't correctly publish a reverse-y'd
    // projection matrix to the OpenGL fixed function pipeline, leading to the rendering being done upside down. You can actually get a correctly reverse-y aware projection matrix via datarefs. The following code would work if you don't
    // want to wait for 11.51
    // TODO: Although for what it's worth, you might not even want to use the modern 3D drawing callback at all. Opting into it is very expensive overlays can be drawn much cheaper in the 2D drawing phase. There actually is example code
    // for that available here: https://developer.x-plane.com/code-sample/coachmarks/
#ifndef RELEASE

#ifdef MAC
    if (data_manager::xplane_ver_i < missionx::XP12_VERSION_NO ) //&& data_manager::xplane_using_modern_driver_b && this->drawPhase == inPhase) // v3.303.8 ignore in XP12
#else
    if (data_manager::xplane_ver_i < missionx::XP12_VERSION_NO  ) //&& (data_manager::xplane_using_modern_driver_b && (xplm_Phase_Modern3D == inPhase  || xplm_Phase_Objects == inPhase) ) ) // v3.303.8 ignore in XP12
#endif // MAC
    {
      //const static XPLMDataRef proj_matrix = this->drefConst.dref_projection_matrix_3d_arr;
      {
        GLfloat projection_arr[16];
        XPLMGetDatavf(this->drefConst.dref_projection_matrix_3d_arr, projection_arr, 0, 16);

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(projection_arr);
      }

    // Display Cue Info only in DEBUG mode (not sure if I should do that. I'll have to test performance differences)
      if (data_manager::xplane_ver_i < missionx::XP12_VERSION_NO) // v3.303.8 ignore in XP12
      {
        if (this->displayCueInfo)
        {
          /* Reset the graphics state.  This turns off fog, texturing, lighting,
           * alpha blending or testing and depth reading and writing, which
           * guarantees that our axes will be seen no matter what. */
          XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0);
          data_manager::drawCueInfo();
          XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0);

        }
      }
    } // draw only in XP11
#endif



    // v3.0.207.1 // calculate all moving objects if not empty
    if (!data_manager::listDisplayMoving3dInstances.empty() &&
        !missionx::dataref_manager::isSimPause() ) // v3.0.207.3 do not calculate/progress when in pause - PROBLEM - movement might be jerkey of timer calculates clock time and not running time
    {
      for (const auto &instName : data_manager::listDisplayMoving3dInstances) // loop over each instance
      {
        if (missionx::data_manager::map3dInstances[instName].mvStat.isMoving) // v3.0.207.4 - check moving flag. If we control movment also from script we will need to calculate elapsed time when stopping movement and then resuming it.
        {

          missionx::data_manager::map3dInstances[instName].calcPosOfMovingObject();   // v3.0.207.2
          missionx::data_manager::map3dInstances[instName].positionInstancedObject(); // v3.0.207.2

          if (!missionx::data_manager::map3dInstances[instName].mvStat.flag_wait_for_next_flc)
            missionx::data_manager::map3dInstances[instName].checkAreWeThereYet(); // v3.0.253.6 are we their yet ?

        }
      }
    }
    // v3.0.224.2 add support to draw callback script calls
    if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running && !data_manager::draw_script.empty())
    {
      ++frameCounter;
      if (frameCounter % 4 == 0)
      {
        missionx::data_manager::execScript(data_manager::draw_script, data_manager::smPropSeedValues, "[draw] Draw script is invalid. Aborting Mission"); //
        frameCounter = 0;
      }
    }

    if ((data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running) * (missionx::dataref_manager::isSimRunning()) + (this->plane_stats.burstTimer.isRunning() * missionx::dataref_manager::isSimRunning())) // v3.303.14 added burstTimer condition// v3.303.8.1 Added condition if mission is in running state
    {
      this->gatherStats_identify_takeoff_and_landings(); // v3.0.255.1
    }


    if (data_manager::xplane_ver_i < missionx::XP12_VERSION_NO  )
    {
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
    }

  } // end xplm_Phase_Objects



}

// -------------------------------------

void
missionx::Mission::drawMarkings(XPLMMapLayerID layer, const float* inMapBoundsLeftTopRightBottom, float zoomRatio, float mapUnitsPerUserInterfaceUnit, XPLMMapStyle mapStyle, XPLMMapProjectionID projection, void* inRefcon)
{

  if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running  && !data_manager::currentLegName.empty())
  {
    // The arbitrary OpenGL drawing done for our markings layer.

	  XPLMSetGraphicsState(
			  0 /* no fog */,
			  0 /* 0 texture units */,
			  0 /* no lighting */,
			  0 /* no alpha testing */,
			  1 /* do alpha blend */,
			  1 /* do depth testing */,
			  0 /* no depth writing */
	  );

    data_manager::drawCueInfoOn2DMap(inMapBoundsLeftTopRightBottom, &projection);
  }
}


// -------------------------------------

void
missionx::Mission::gatherStats_identify_takeoff_and_landings()
{

  if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running && data_manager::gather_stats.get_db_ptr() != nullptr)
  {
    // Lambda - get plane phase
    const auto lmbda_get_plane_phase = [&]() {
      switch (this->plane_stats.current_plane_state)
      {
        case mx_plane_stats::mx_plane_status::decide_state:
        {
          if (this->plane_stats.current_faxil == 0.0f)
            this->plane_stats.set_plane_state(mx_plane_stats::mx_plane_status::start_airborn);
          else
            this->plane_stats.set_plane_state(mx_plane_stats::mx_plane_status::start_on_ground);
        }
        break;
        case mx_plane_stats::mx_plane_status::no_status:
        {
          this->plane_stats.set_plane_state(mx_plane_stats::mx_plane_status::decide_state);
        }
        break;
        default:
        {
          if (((this->plane_stats.current_plane_state == mx_plane_stats::mx_plane_status::start_airborn || this->plane_stats.current_plane_state == mx_plane_stats::mx_plane_status::enroute) && this->plane_stats.prev_faxil == 0.0f &&
               this->plane_stats.current_faxil != 0.0f)) // Touchdown
          {
            this->plane_stats.set_plane_state(mx_plane_stats::mx_plane_status::plane_is_landing);
            return mxconst::get_STATS_LANDING();
          }
          else if (((this->plane_stats.current_plane_state == mx_plane_stats::mx_plane_status::start_on_ground || this->plane_stats.current_plane_state == mx_plane_stats::mx_plane_status::landed) && this->plane_stats.prev_faxil != 0.0f &&
                    this->plane_stats.current_faxil == 0.0f)) // takeoff
          {
            this->plane_stats.set_plane_state(mx_plane_stats::mx_plane_status::plane_is_taking_off);
            return mxconst::get_STATS_TAKEOFF();
          }
        }
        break;
      }

      return missionx::EMPTY_STRING; // no special stats need to be written
    }; // end lmbda


    const std::string planePhase = lmbda_get_plane_phase(); // returns: mxconst::get_STATS_LANDING() or mxconst::get_STATS_TAKEOFF()


    if ( (this->plane_stats.current_plane_state == mx_plane_stats::mx_plane_status::plane_is_landing) + (this->plane_stats.current_plane_state == mx_plane_stats::mx_plane_status::plane_is_taking_off) )
    {
      // we need to pick the stats before gathering a new stats because the plugin found that the state is landing, meaning there was a touchdown
      if (this->plane_stats.current_plane_state == mx_plane_stats::mx_plane_status::plane_is_landing)
      {
        data_manager::strct_currentLegStats4UIDisplay.fTouchDown_landing_rate_vh_ind_fpm = missionx::data_manager::gather_stats.get_stats_object().vh_ind_fpm;
        data_manager::strct_currentLegStats4UIDisplay.fTouchDown_gforce_normal           = missionx::data_manager::gather_stats.get_stats_object().gforce_normal;
        data_manager::strct_currentLegStats4UIDisplay.fTouchDownWeight                   = missionx::data_manager::gather_stats.get_stats_object().m_total;
        data_manager::strct_currentLegStats4UIDisplay.fTouchDown_acf_m_max               = dataref_manager::get_acf_m_max_currentMaxPlaneAllowedWeightK();

        float fSum = 0.0f; // calculate avg FPM in last 15 meters
        std::for_each(data_manager::strct_currentLegStats4UIDisplay.vecFpm15Meters.cbegin(), data_manager::strct_currentLegStats4UIDisplay.vecFpm15Meters.cend(), [&fSum](const float& n) { fSum += n; });
        data_manager::strct_currentLegStats4UIDisplay.fLanding_vh_ind_fpm_avg_15_meters = fSum / (float)(data_manager::strct_currentLegStats4UIDisplay.vecFpm15Meters.size());

        fSum = 0.0f; // Calculate avg gForce in last 15 meters
        std::for_each(data_manager::strct_currentLegStats4UIDisplay.vecgForce15Meters.cbegin(), data_manager::strct_currentLegStats4UIDisplay.vecgForce15Meters.cend(), [&fSum](const float& n) { fSum += n; });
        data_manager::strct_currentLegStats4UIDisplay.fLanding_gforce_normal_avg_15_meters = fSum / (float)(data_manager::strct_currentLegStats4UIDisplay.vecgForce15Meters.size());


        data_manager::gather_stats.gather_and_store_stats(planePhase);
      }
      else 
        data_manager::gather_stats.gather_and_store_stats(planePhase);


      // start the burst record rows that is part of touchdown or takeoff activity and it records each frame data into the stats table for 2 seconds.
      this->plane_stats.burstTimer.reset();
      missionx::Timer::start(this->plane_stats.burstTimer, 2, "B-STOPPER"); // 2 seconds

      this->plane_stats.before_transition_state = this->plane_stats.current_plane_state;
      this->plane_stats.set_plane_state(mx_plane_stats::mx_plane_status::transition); // change to transition so takeoff won't pick also landing
    }
    else if (this->plane_stats.burstTimer.isRunning())
    {
      data_manager::gather_stats.gather_and_store_stats(mxconst::get_STATS_TRANSITION()); // v3.303.14 Added bursting stats too
    }
    else if (this->plane_stats.current_plane_state == mx_plane_stats::mx_plane_status::plane_is_taking_off)
    {
      // reset last 15m fpm average result and it vector
      data_manager::strct_currentLegStats4UIDisplay.reset_fpm_and_gForce_data();
    }

    if (missionx::Timer::wasEnded(this->plane_stats.burstTimer))
    {
      this->plane_stats.burstTimer.stop(); // stopTimer();

      switch (this->plane_stats.before_transition_state)
      {
        case mx_plane_stats::mx_plane_status::plane_is_taking_off:
        {
          this->plane_stats.current_plane_state = mx_plane_stats::mx_plane_status::enroute;
        }
        break;
        case mx_plane_stats::mx_plane_status::plane_is_landing:
        {
          this->plane_stats.current_plane_state = mx_plane_stats::mx_plane_status::landed;
        }
        break;
        default:
          break;
      }
    }

    this->plane_stats.prev_faxil = this->plane_stats.current_faxil;
    plane_stats.current_faxil    = XPLMGetDataf(drefConst.dref_faxil_gear_f);
  }
}

// -------------------------------------

void
missionx::Mission::storeCoreAttribAsProperties()
{
  std::string err;

  data_manager::mx_global_settings.setNodeProperty<int>(mxconst::get_OPT_FLIGHT_LEG_PROGRESS_COUNTER(), this->flight_leg_progress_counter_i);

  data_manager::mx_global_settings.setNodeStringProperty(mxconst::get_PROP_MISSION_STATE(), data_manager::translate_enum_mission_state_to_string(data_manager::missionState));// v3.303.11
  data_manager::mx_global_settings.setNodeStringProperty(mxconst::get_PROP_MISSION_CURRENT_LEG(), data_manager::currentLegName); // v3.303.11

  // store plane location
  data_manager::mx_global_settings.setNodeProperty<double>(mxconst::get_SAVEPOINT_PLANE_LATITUDE(), XPLMGetDatad(drefConst.dref_lat_d));
  data_manager::mx_global_settings.setNodeProperty<double>(mxconst::get_SAVEPOINT_PLANE_LONGITUDE(), XPLMGetDatad(drefConst.dref_lon_d));
  data_manager::mx_global_settings.setNodeProperty<double>(mxconst::get_SAVEPOINT_PLANE_ELEVATION(), XPLMGetDatad(drefConst.dref_elev_d));
  data_manager::mx_global_settings.setNodeProperty<float>(mxconst::get_SAVEPOINT_PLANE_SPEED(), XPLMGetDataf(drefConst.dref_groundspeed_f));
  data_manager::mx_global_settings.setNodeProperty<float>(mxconst::get_SAVEPOINT_PLANE_HEADING(), XPLMGetDataf(drefConst.dref_heading_psi_f));

  // v3.0.253.7 store GPS preference at the start of the mission
  if (data_manager::mx_global_settings.node.getAttribute(mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE().c_str()) == nullptr)
    data_manager::mx_global_settings.setNodeProperty<bool>(mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE(), Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE(), true));

  // v3.0.255.3 store original mission file folder
  const std::string key = missionx::data_manager::selectedMissionKey; // v3.0.241.1
  if (!key.empty())
  {
    const std::string pathToMissionFile = data_manager::mapBrieferMissionList[key].getFullMissionXmlFilePath();
    data_manager::mx_global_settings.setNodeStringProperty(mxconst::get_PROP_MISSION_FILE_LOCATION(), pathToMissionFile); // v3.303.11, data_manager::mx_global_settings.node, mxconst::get_GLOBAL_SETTINGS()); // v3.0.255.3
  }

  // v3.303.13 store current weather information
  Utils::xml_delete_all_subnodes(data_manager::mx_global_settings.node, mxconst::get_ELEMENT_SAVED_WEATHER());
  IXMLNode xSavedWeather = data_manager::mx_global_settings.node.addChild(mxconst::get_ELEMENT_SAVED_WEATHER().c_str());
  if (!xSavedWeather.isEmpty())
    Utils::xml_set_text(xSavedWeather, missionx::data_manager::get_weather_state());

  // v25.02.1
  Utils::xml_set_attribute_in_node <int>( data_manager::mx_global_settings.node, mxconst::get_ATTRIB_INVENTORY_LAYOUT(), Inventory::opt_forceInventoryLayoutBasedOnVersion_i, mxconst::get_ELEMENT_COMPATIBILITY());
}

// -------------------------------------

void
missionx::Mission::applyPropertiesToLocal()
{
  if (data_manager::mx_global_settings.node.isAttributeSet(mxconst::get_PROP_MISSION_STATE().c_str())) // v3.0.241.1 //if (data_manager::mx_mission_properties_old.hasProperty(mxconst::get_PROP_MISSION_STATE()))
    data_manager::missionState = data_manager::translate_mission_state_to_enum(Utils::readAttrib(data_manager::mx_global_settings.node, mxconst::get_PROP_MISSION_STATE(), ""));
  if (data_manager::mx_global_settings.node.isAttributeSet(mxconst::get_PROP_MISSION_CURRENT_LEG().c_str()))                              
    data_manager::currentLegName = Utils::readAttrib(data_manager::mx_global_settings.node, mxconst::get_PROP_MISSION_CURRENT_LEG(), ""); 

  if (data_manager::mx_global_settings.node.isAttributeSet(mxconst::get_OPT_FLIGHT_LEG_PROGRESS_COUNTER().c_str()))
    this->flight_leg_progress_counter_i = Utils::readNodeNumericAttrib<int>(data_manager::mx_global_settings.node, mxconst::get_OPT_FLIGHT_LEG_PROGRESS_COUNTER(), 0);
}

// -------------------------------------

bool
missionx::Mission::saveCheckpoint()
{
  /* Build XML in Memory */
  IXMLNode    xmlFileDoc; //
  IXMLNode    xSaveNode;  // root node
  std::string err;

  xmlFileDoc = IXMLNode::createXMLTopNode("xml", TRUE);
  xmlFileDoc.addAttribute(mxconst::get_ATTRIB_VERSION().c_str(), "1.0");
  xmlFileDoc.addAttribute("encoding", "ISO-8859-1");
  // add disclaimer
  xmlFileDoc.addClear("\n\tFile has been created by Mission-X plug-in.\n\tAny modifications might break or invalidate the file.\n\t", "<!--", "-->");

  xSaveNode = xmlFileDoc.addChild(mxconst::get_ELEMENT_SAVE().c_str());
  Utils::xml_copy_node_attributes(missionx::data_manager::xMainNode, xSaveNode); // v3.0.241.1


  // prepare Mission properties
  this->storeCoreAttribAsProperties();

  // Construct XML
  missionx::data_manager::saveCheckpoint(xSaveNode);
  missionx::QueueMessageManager::saveCheckpoint(xSaveNode); // v3.0.223.1 moved code from data_manager to mxconst::QMM
  missionx::script_manager::saveCheckpoint(xSaveNode);

  // v3.0.226.1 rc1 Fix bug where missionSavepointFilePath was not initialized after mission load
  missionx::data_manager::missionSavepointFilePath     = missionx::data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_FLD_MISSIONX_SAVE_PATH(), "", err) + std::string(XPLMGetDirectorySeparator()) + Utils::readAttrib(data_manager::xMainNode, mxconst::get_ATTRIB_NAME(), "") + mxconst::get_MX_FILE_SAVE_EXTENTION();
  missionx::data_manager::missionSavepointDrefFilePath = missionx::data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_FLD_MISSIONX_SAVE_PATH(), "", err) + std::string(XPLMGetDirectorySeparator()) + Utils::readAttrib(data_manager::xMainNode, mxconst::get_ATTRIB_NAME(), "") + mxconst::get_MX_FILE_SAVE_DREF_EXTENTION();
                                                                                      

  // Prepare path and file names
  const std::string savePath     = missionx::data_manager::missionSavepointFilePath;
  const std::string saveDrefPath = missionx::data_manager::missionSavepointDrefFilePath;

  Log::logMsg(savePath);
  Log::logMsg(saveDrefPath + mxconst::get_UNIX_EOL());


  // Write to file
  IXMLRenderer xmlWriter; //
  xmlWriter.writeToFile(xmlFileDoc, savePath.c_str(), "ISO-8859-1");

  // test the file
  std::string errMsg;
  errMsg.clear();
  IXMLDomParser iDom;

  ITCXMLNode saveNode = iDom.openFileHelper(savePath.c_str(), "SAVE", &errMsg); //

  const bool saveOk = ( errMsg.empty () && !saveNode.isEmpty () ); // v3.0.223.6

  // save Datarefs state
  if (saveOk)
  {
    std::string save_err;

    // v3.303.8.3 added copy of stats DB as a backup into same "db" folder. It based on the mission name + stats.db
    const std::string path_to_original_stats_db_s   = std::string(data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", data_manager::errStr)) + "/" + mxconst::get_DB_FOLDER_NAME() + "/" + mxconst::get_DB_STATS_XP();
    const std::string path_to_checkpoint_stats_db_s = std::string(data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", data_manager::errStr)) + "/" + mxconst::get_DB_FOLDER_NAME() + "/" + Utils::readAttrib(data_manager::xMainNode, mxconst::get_ATTRIB_NAME(), "") + "." + mxconst::get_DB_STATS_XP();

    if (path_to_original_stats_db_s != path_to_checkpoint_stats_db_s)
    {
      if (missionx::system_actions::copy_file(path_to_original_stats_db_s, path_to_checkpoint_stats_db_s, save_err))
      {
        Log::logMsg("Saved stats db: " + path_to_checkpoint_stats_db_s);
      }
      else
      {
        Log::logMsg(save_err);
        Mission::uiImGuiBriefer->setMessage(save_err);
      }
    }

    ///// Save Datarefs
    system_actions::save_datarefs_with_savepoint(std::string(data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_PROP_XPLANE_PLUGINS_PATH(), "", save_err)) + "/DataRefs.txt", saveDrefPath, save_err);

    // v3.303.9.1 save ACF custom datarefs
    char outFileName[512]{ 0 };
    char outPathAndFile[2048]{ 0 };
    XPLMGetNthAircraftModel(XPLM_USER_AIRCRAFT, outFileName, outPathAndFile); // we will only return the file name


    std::string target_acf_dataref = std::string(missionx::data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_FLD_MISSIONX_SAVE_PATH(), "", save_err)) + "/" + Utils::readAttrib(data_manager::xMainNode, mxconst::get_ATTRIB_NAME(), "") + "-" + std::string(outFileName) + mxconst::get_MX_FILE_SAVE_DREF_EXTENTION();
    system_actions::save_acf_datarefs_with_savepoint_v2(target_acf_dataref);


    Mission::uiImGuiBriefer->setMessage("Created savepoint: " + missionx::data_manager::missionSavepointFilePath, 20); // v3.0.159
  }
  else // // v3.0.223.6
  {
    Log::logMsg(std::string("Error while creating checkpoint: ") + errMsg);
    Mission::uiImGuiBriefer->setMessage("Error while creating checkpoint: " + errMsg, 20); // v3.0.159
  }

  return saveOk;
}

// -------------------------------------

// -------------------------------------

bool
missionx::Mission::loadCheckpoint()
{
  std::string err;
  err.clear();

  Mission::stopMission(); // will stop mission, clear all data and release sound

  // prepare folders if user will use load checkpoint // v3.0.152
  const std::string pathToSavepointFile = data_manager::missionSavepointFilePath =
    missionx::data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_FLD_MISSIONX_SAVE_PATH(), "", err) + std::string(XPLMGetDirectorySeparator()) + std::string(data_manager::mapBrieferMissionList[missionx::data_manager::selectedMissionKey].missionName) + mxconst::get_MX_FILE_SAVE_EXTENTION();
  const std::string pathToSavepointDatarefFile = data_manager::missionSavepointDrefFilePath =
    missionx::data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_FLD_MISSIONX_SAVE_PATH(), "", err) + std::string(XPLMGetDirectorySeparator()) + std::string(data_manager::mapBrieferMissionList[missionx::data_manager::selectedMissionKey].missionName) + mxconst::get_MX_FILE_SAVE_DREF_EXTENTION();


  // load mission savepoint dataref snapshot
  if (read_mission_file::loadSavePoint())
  {
#ifndef RELEASE
    Log::logMsg(">> Finish loading main checkpoint data. Start post load actions.");
#endif

    // loadSavePoint reset this value so we re-apply it.
    data_manager::missionSavepointFilePath     = pathToSavepointFile;
    data_manager::missionSavepointDrefFilePath = pathToSavepointDatarefFile;


    // position plane
    // read plane location
    double lat     = Utils::readNodeNumericAttrib<double>(data_manager::mx_global_settings.node, mxconst::get_SAVEPOINT_PLANE_LATITUDE(), 0.0);  // v3.0.241.1
    double lon     = Utils::readNodeNumericAttrib<double>(data_manager::mx_global_settings.node, mxconst::get_SAVEPOINT_PLANE_LONGITUDE(), 0.0); // v3.0.241.1
    float  elev    = Utils::readNodeNumericAttrib<float>(data_manager::mx_global_settings.node, mxconst::get_SAVEPOINT_PLANE_ELEVATION(), 0.0);  // v3.0.241.1
    float  speed   = Utils::readNodeNumericAttrib<float>(data_manager::mx_global_settings.node, mxconst::get_SAVEPOINT_PLANE_SPEED(), 0.0);      // v3.0.241.1
    float  heading = Utils::readNodeNumericAttrib<float>(data_manager::mx_global_settings.node, mxconst::get_SAVEPOINT_PLANE_HEADING(), 0.0);    // v3.0.241.1


    if (lat != 0.0 && lon != 0.0)
    {
      XPLMPlaceUserAtLocation(lat, lon, elev, heading, speed); // The following function call will reset Fuel datarefs to defaults and will override our "dataref saved" state mission file one.
    }


#ifndef RELEASE
    Log::logMsg(">> Try to load saved mission dataref file.");
#endif // !RELEASE

    system_actions::read_saved_mission_dataref_file(data_manager::missionSavepointDrefFilePath, err);
    if (!err.empty())
    {
      Mission::uiImGuiBriefer->setMessage("Failed to load dataref file, but savepoint was loaded. Continue at your own risk...", 20);
      Log::logMsgErr(err);
    }
    else 
      Mission::uiImGuiBriefer->setMessage("Savepoint was Loaded. You can press start...", 20);

    // v3.303.9.1 read custom datarefs - based on aircraft custom datarefs
    char outFileName[512]{ 0 };
    char outPathAndFile[2048]{ 0 };
    XPLMGetNthAircraftModel(XPLM_USER_AIRCRAFT, outFileName, outPathAndFile); // we will only return the file name
    std::string target_acf_dataref_s = std::string(missionx::data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_FLD_MISSIONX_SAVE_PATH(), "", err)) + "/" + Utils::readAttrib(data_manager::xMainNode, mxconst::get_ATTRIB_NAME(), "") + "-" + std::string(outFileName) + mxconst::get_MX_FILE_SAVE_DREF_EXTENTION();

    system_actions::read_saved_mission_dataref_file(target_acf_dataref_s, err, true);
    if (!err.empty())
    {
      Mission::uiImGuiBriefer->setMessage("Failed to load aircraft dataref file, but savepoint was loaded. Continue at your own risk...", 20);
      Log::logMsgErr(err);
    }
    else 
      Mission::uiImGuiBriefer->setMessage("Savepoint and aircraft datarefs were Loaded. You can press start...", 20);



    // v3.303.8.3 Copy backup stats database file, if present.
    const std::string path_to_checkpoint_stats_db_s = std::string(data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", data_manager::errStr)) + "/" + mxconst::get_DB_FOLDER_NAME() + "/" + Utils::readAttrib(data_manager::xMainNode, mxconst::get_ATTRIB_NAME(), "") + "." + mxconst::get_DB_STATS_XP();
    const std::string path_to_original_stats_db_s   = std::string(data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_PROP_MISSIONX_PATH(), "", data_manager::errStr)) + "/" + mxconst::get_DB_FOLDER_NAME() + "/" + mxconst::get_DB_STATS_XP();


    missionx::data_manager::db_close_database(data_manager::db_stats);

    data_manager::delete_db_file(path_to_original_stats_db_s);

    if (missionx::system_actions::copy_file(path_to_checkpoint_stats_db_s, path_to_original_stats_db_s,err))
    {
      Log::logMsg("Copied saved stats db: " + path_to_checkpoint_stats_db_s);
    }
    else
    {
      Log::logMsg(err);
      Mission::uiImGuiBriefer->setMessage(err);
    }
    // end v3.303.8.3 finish adding load stats db from checkpoint backup



    data_manager::missionState = mx_mission_state_enum::mission_loaded_from_savepoint; // v3.0.159

    // set designer mode from checkpoint file
    // v3.0.203.5
    missionx::data_manager::flag_setupEnableDesignerMode = Utils::readBoolAttrib(data_manager::xMainNode, mxconst::get_ATTRIB_MISSION_DESIGNER_MODE(), missionx::data_manager::flag_setupEnableDesignerMode); // v3.0.241.1
    missionx::system_actions::pluginSetupOptions.node.updateAttribute(mxUtils::formatNumber<int>(((missionx::data_manager::flag_setupEnableDesignerMode) ? 1 : 0)).c_str(), mxconst::get_OPT_ENABLE_DESIGNER_MODE().c_str(), mxconst::get_OPT_ENABLE_DESIGNER_MODE().c_str()); // v3.303.8.3

    // v3.0.215.1
    if (Mission::uiImGuiMxpad)
      Mission::uiImGuiMxpad->setWasHiddenByAutoHideOption(false); // v3.0.251.1 reset state
  }
  else
  {
    Mission::uiImGuiBriefer->setMessage(missionx::read_mission_file::getErrMsg(), 20);
    return false;
  }

  this->applyPropertiesToLocal();


  Log::logDebugBO(">> Finished Loading checkpoint.");

  return true; // return success
}

// -------------------------------------

void
missionx::Mission::flcPRE()
{
#ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), "flcPRE", false);
#endif // TIMER_FUNC

  if (missionx::data_manager::queFlcActions.empty())
    return;

  while (!data_manager::queFlcActions.empty())
  {
    missionx::mx_flc_pre_command c = data_manager::queFlcActions.front();
    data_manager::queFlcActions.pop();

#ifdef TIMER_FUNC
    Log::logMsg("flcPRE action: " + mxUtils::formatNumber<int>((int)c));
#endif

    switch (c)
    {
      case missionx::mx_flc_pre_command::set_briefer_text_message: // v3.0.219.12+ added support for thread messaging into briefer
      {

        if (!data_manager::queThreadMessage.empty())
        {
          const auto message_s = data_manager::queThreadMessage.front();
          data_manager::queThreadMessage.pop();
        }

        if (!data_manager::queThreadMessage.empty()) // if there are more messages, push them to next flight callback
          missionx::data_manager::postFlcActions.push(missionx::mx_flc_pre_command::set_briefer_text_message);
      }
      break;
      case missionx::mx_flc_pre_command::set_time:
      {
        if (missionx::data_manager::missionState != missionx::mx_mission_state_enum::mission_loaded_from_savepoint)
          setMissionTime();

        missionx::data_manager::timelapse.flag_isActive = false; // v3.303.8        

      }
      break;
      case missionx::mx_flc_pre_command::exec_apt_dat_optimization:
      {
        #ifndef RELEASE
        missionx::Log::logMsg("[Missionx] Menu Optimize \"apt.dat\" files (should take 3-8min, depends on machine, runs in the background).");
        #endif
        exec_apt_dat_optimization();
      }
      break;
      case missionx::mx_flc_pre_command::imgui_reload_templates_data_and_images:
      {
        // reload all templates and do nothing on the UI
        // Command will do the following:
        // 1. clear current templates bitmap and containers
        // 2. call read template including XML and Images
        // 3. if there are templates then set briefer layer and display it

        std::string err;
        err.clear();
        // 1. clear current template map data
        missionx::data_manager::clearRandomTemplateTextures();

        // 2. call read templates
        const std::string template_folder       = data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_FLD_RANDOM_TEMPLATES_PATH(), "", err); // get path to template folder
        const std::string custom_mission_folder = data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_FLD_MISSIONS_ROOT_PATH(), "", err);     // get path to custom missionx folder "/Custom Scenery/missionx"

        if (missionx::ListDir::read_all_templates(template_folder, custom_mission_folder, missionx::data_manager::mapGenerateMissionTemplateFiles))
        {
          missionx::data_manager::mapGenerateMissionTemplateFilesLocator.clear();

          // v3.0.241.9 extract and remove the special BLANK Tempalte from the real template maps
          if (Utils::isElementExists(missionx::data_manager::mapGenerateMissionTemplateFiles, mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI()))
          {
            data_manager::user_driven_template_info = missionx::data_manager::mapGenerateMissionTemplateFiles[mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI()];
            missionx::data_manager::mapGenerateMissionTemplateFiles.erase(mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI());
          }

          // build file locator: mapGenerateMissionTemplateFilesLocator while loading template textures
          missionx::data_manager::mapGenerateMissionTemplateFilesLocator.clear();
          int seq = 0;
          // load textures and build file locator: mapGenerateMissionTemplateFilesLocator

          std::string errorMsg; // v24.06.1
          for (auto& f : missionx::data_manager::mapGenerateMissionTemplateFiles)
          {
            ++seq;
            Utils::addElementToMap(missionx::data_manager::mapGenerateMissionTemplateFilesLocator, seq, f.first);
            f.second.seq = seq;

            // Read textures
            missionx::BitmapReader::loadGLTexture(f.second.imageFile, errorMsg, false); // v3.0.253.9 do not flip image

            if (f.second.imageFile.gTexture != 0)
            {
              Log::logMsg("Loaded Template bitmap: " + f.second.imageFile.getAbsoluteFileLocation()); // debug
            }
            else
            {
              Log::logMsgErr("Failed Loading Template bitmap: " + f.second.imageFile.getAbsoluteFileLocation()); // debug
            }
          }
        } // end read templates

        // 3. display random templates layer ?
        if (missionx::data_manager::mapGenerateMissionTemplateFiles.empty())
        {
          Mission::uiImGuiBriefer->setMessage("No Mission Template File Found.");

          Log::logDebugBO("No mission template file found");
        }
        else
        {

          Mission::uiImGuiBriefer->setMessage("Templates were loaded....");
          Mission::uiImGuiBriefer->execAction(missionx::mx_window_actions::POST_TEMPLATE_LOAD_DISPLAY_IMGUI_GENERATE_TEMPLATES_IMAGES); // v3.0.255.4 missing action, was in "reload_templates_data_and_images". After some tests all seem to
                                                                                                                                        // function as expected so we deprecated: "reload_templates_data_and_images" case.
        }
      }
      break;

      case missionx::mx_flc_pre_command::gather_acf_custom_datarefs:
      {
        if (missionx::data_manager::flag_gather_acf_info_thread_is_running == false) // if thread is not running
        {
          missionx::data_manager::iGatherAcfTryCounter = 0;
          // v3.303.9.1 save ACF custom datarefs
          char outFileName[512]{ 0 };
          char outPathAndFile[2048]{ 0 };
          XPLMGetNthAircraftModel(XPLM_USER_AIRCRAFT, outFileName, outPathAndFile); // we will only return the file name

          const std::string acf_path_s                          = std::string(outPathAndFile);
          missionx::data_manager::flag_abort_gather_acf_info_thread = false;
          missionx::data_manager::mFetchFutures.push_back(std::async(std::launch::async, missionx::data_manager::gather_custom_acf_datarefs_as_a_thread, acf_path_s, &missionx::data_manager::flag_gather_acf_info_thread_is_running, &missionx::data_manager::flag_abort_gather_acf_info_thread));
        }
        else
        {
          Log::logMsg("[gather_acf_custom_datarefs] missionx::data_manager::gather_custom_acf_datarefs_as_a_thread() is already running.");
          ++missionx::data_manager::iGatherAcfTryCounter;
          if (missionx::data_manager::iGatherAcfTryCounter < 30)
          {
            Log::logMsg("[gather_acf_custom_datarefs] Will try next Flight Call Back, try no.: " + mxUtils::formatNumber<int>(missionx::data_manager::iGatherAcfTryCounter));
            missionx::data_manager::postFlcActions.push(missionx::mx_flc_pre_command::gather_acf_custom_datarefs); // v3.303.13 try again next iteration
          }
        }
      }
      break;

      case missionx::mx_flc_pre_command::gather_acf_cargo_data:
      {
        if (missionx::data_manager::flag_gather_acf_info_thread_is_running == true) // if thread is not running
        {
          missionx::data_manager::iGatherAcfTryCounter = 0;
          // v3.303.9.1 save ACF custom datarefs
          char outFileName[512]{ 0 };
          char outPathAndFile[2048]{ 0 };
          XPLMGetNthAircraftModel(XPLM_USER_AIRCRAFT, outFileName, outPathAndFile); // we will only return the file name

          const std::string acf_path_s                          = std::string(outPathAndFile);
          missionx::data_manager::flag_abort_gather_acf_info_thread = false;
          missionx::data_manager::mFetchFutures.push_back(std::async(std::launch::async, missionx::data_manager::gather_custom_acf_datarefs_as_a_thread, acf_path_s, &missionx::data_manager::flag_gather_acf_info_thread_is_running, &missionx::data_manager::flag_abort_gather_acf_info_thread));
        }
        else
        {
          Log::logMsg("[gather_acf_cargo_data] missionx::data_manager::gather_custom_acf_datarefs_as_a_thread() is already running.");
          ++missionx::data_manager::iGatherAcfTryCounter;
          if (missionx::data_manager::iGatherAcfTryCounter < 30)
          {
            Log::logMsg("[gather_acf_cargo_data] Will try next Flight Call Back, try no.: " + mxUtils::formatNumber<int>(missionx::data_manager::iGatherAcfTryCounter));
            missionx::data_manager::postFlcActions.push(missionx::mx_flc_pre_command::gather_acf_cargo_data); // 
          }
        }
      }
      break;

      case missionx::mx_flc_pre_command::imgui_generate_random_mission_file:
      {

        assert(Mission::uiImGuiBriefer && "uiImGuiBriefer was NOT initialized. Fix initialization in plugin.cpp");

        if (Utils::isElementExists(missionx::data_manager::mapGenerateMissionTemplateFiles, Mission::uiImGuiBriefer->selectedTemplateKey) ||
            (Mission::uiImGuiBriefer->selectedTemplateKey) == mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI())
        {
          if (Mission::uiImGuiBriefer->selectedTemplateKey == mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI())
          {
            bool bTemplateExists = Utils::isElementExists(missionx::data_manager::mapGenerateMissionTemplateFiles, Mission::uiImGuiBriefer->selectedTemplateKey);

            if (!bTemplateExists) // try to load the missing template
              bTemplateExists = missionx::data_manager::find_and_read_template_file(mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI());

            // Initialize RandomEngine template pointer, base on bTemplateExists
            if (bTemplateExists) // if we were able to load the template file or Template exists in mapGenerateMissionTemplateFiles
              engine.working_tempFile_ptr = &missionx::data_manager::mapGenerateMissionTemplateFiles[Mission::uiImGuiBriefer->selectedTemplateKey];
            else
              engine.working_tempFile_ptr = &missionx::data_manager::user_driven_template_info; // use a dummy template file, to fail the RandomEngine later on.

            missionx::data_manager::user_driven_template_info.prepareSentenceBasedOnString(""); //, 1, 1);
            engine.flag_rules_defined_by_user_ui = true;
          }
          else
          {
            missionx::data_manager::mapGenerateMissionTemplateFiles[Mission::uiImGuiBriefer->selectedTemplateKey].prepareSentenceBasedOnString(""); // v3.303.14 no need for length values //, 1, 1); // the line width and maxLines values are dummy since empty string will be ignored by the function.
            engine.working_tempFile_ptr          = &missionx::data_manager::mapGenerateMissionTemplateFiles[Mission::uiImGuiBriefer->selectedTemplateKey];
            engine.flag_rules_defined_by_user_ui = false;
          }

          //engine.working_tempFile_ptr->user_pickedReplaceOptions_i = Mission::uiImGuiBriefer->strct_generate_template_layer.user_pick_from_replaceOptions_combo_i; // v3.0.255.4 store user pick option. If it -1 then plugin will ignore it
          //engine.working_tempFile_ptr->mapUserOptionsPicked = Mission::uiImGuiBriefer->strct_generate_template_layer.mapReplaceOption_ui; // v24.12.2 store multi options picked by user.

          if (engine.exec_generate_mission_thread(Mission::uiImGuiBriefer->selectedTemplateKey))
          {
            Mission::uiImGuiBriefer->setMessage("Mission creation is running in the BACKGROUND, please wait for it to finish. file:'" + Mission::uiImGuiBriefer->selectedTemplateKey + "'", 20);
            missionx::data_manager::flag_generate_engine_is_running = true;
          }
          else
          {
            Mission::uiImGuiBriefer->flag_generatedRandomFile_success = false; // it might be still running
            Mission::uiImGuiBriefer->setMessage(engine.getErrorMsg(), 6);
          }
        }
      }
      break;
      case missionx::mx_flc_pre_command::load_mission:
      {
        Mission::loadMission(); // during load we will call stopMission()
      }
      break;
      case missionx::mx_flc_pre_command::load_savepoint:
      {
        if (Mission::loadCheckpoint())
        {
          data_manager::missionState = missionx::mx_mission_state_enum::mission_loaded_from_savepoint;

          // pause in VR mode and in air
          if (missionx::mxvr::vr_display_missionx_in_vr_mode)
          {
            // check faxil
            const float faxil = XPLMGetDataf(missionx::drefConst.dref_faxil_gear_f); // 0 = in air. Anything else = on ground. // "sim/flightmodel/forces/faxil_gear"
            if (faxil == 0.0f)                                                       // is in air
              missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::force_pause_xplane);
          }
        }
      }
      break;
      case missionx::mx_flc_pre_command::create_savepoint:
      {
        Mission::saveCheckpoint();
      }
      break;
      case missionx::mx_flc_pre_command::imgui_prepare_mission_files_briefer_info:
      {
        this->prepareMissionBrieferInfo(); // v3.0.241.10 b3 refresh list of all missions in folders. Should pick new mission files created by plugin and might not be present before
        this->prepareUiMissionList();      // will reload data and images

        assert(Mission::uiImGuiBriefer && "uiImguiBriefer is not initialized correctly !!!");
        Mission::uiImGuiBriefer->strct_pick_layer.bFinished_loading_mission_images = true;
      }
      break;
      case missionx::mx_flc_pre_command::imgui_check_validity_of_db_file: // v3.0.253.9
      {
        if (data_manager::init_xp_airport_db() && missionx::data_manager::db_xp_airports.db_is_open_and_ready)
        {

          std::string stmt_uq_name = "count_tables";
          int         counter      = 0;
          if (data_manager::db_xp_airports.prepareNewStatement(stmt_uq_name, "select count(1) from sqlite_master v1 where v1.type='table'"))
          {
            assert(data_manager::db_xp_airports.mapStatements[stmt_uq_name] != nullptr);
            int rc = data_manager::db_xp_airports.step(data_manager::db_xp_airports.mapStatements[stmt_uq_name]);
            if (rc == SQLITE_ROW)
            {
              counter = sqlite3_column_int(data_manager::db_xp_airports.mapStatements[stmt_uq_name], 0); // there is only one column in this query
            }

            sqlite3_clear_bindings(data_manager::db_xp_airports.mapStatements[stmt_uq_name]);

            if (sqlite3_reset(data_manager::db_xp_airports.mapStatements[stmt_uq_name]) != SQLITE_OK) // reset stmt pointer
            {
              Log::logMsg(std::string("ERROR: [db exec error] reset: ") + sqlite3_errmsg(data_manager::db_xp_airports.db) + "\n");
            }

          } // end query from db

          if (counter > 1)
          {
            Mission::uiImGuiBriefer->strct_user_create_layer.layer_state       = missionx::mx_layer_state_enum::success_can_draw;
            Mission::uiImGuiBriefer->strct_generate_template_layer.layer_state = missionx::mx_layer_state_enum::success_can_draw;

            if (Mission::uiImGuiBriefer->getCurrentLayer() == missionx::uiLayer_enum::option_generate_mission_from_a_template_layer)
            {
              Mission::uiImGuiBriefer->strct_generate_template_layer.bFinished_loading_templates = false;
              Mission::uiImGuiBriefer->setMessage("Please wait while loading mission templates...", 8);
              missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::imgui_reload_templates_data_and_images);
            }
          }
          else
          {
            Mission::uiImGuiBriefer->strct_user_create_layer.layer_state       = missionx::mx_layer_state_enum::failed_data_is_not_present;
            Mission::uiImGuiBriefer->strct_generate_template_layer.layer_state = missionx::mx_layer_state_enum::failed_data_is_not_present;
            Mission::uiImGuiBriefer->setMessage("Failed to validate the presence of OPTIMIZED Database. Suggestion: Re-Run apt.dat optimization.");
          }
        }
        else
        {
          // Failure to initiate the database
          Mission::uiImGuiBriefer->strct_user_create_layer.layer_state       = missionx::mx_layer_state_enum::fatal_database_is_not_initializing_correctly;
          Mission::uiImGuiBriefer->strct_generate_template_layer.layer_state = missionx::mx_layer_state_enum::fatal_database_is_not_initializing_correctly;
          Mission::uiImGuiBriefer->setMessage("ERROR!!! Mission-X plugin failed to load/open the local database in {missionx/db} folder. Try to delete it and rerun apt.dat optimization.");
        }
      }
      break;
      case missionx::mx_flc_pre_command::imgui_check_presence_of_db_ils_data:
      {
        std::string err;
        assert(Mission::uiImGuiBriefer && "uiImguiBriefer is not initialized correctly - imgui_check_presence_of_db_ils_data !!!");

        if (data_manager::init_xp_airport_db() && missionx::data_manager::db_xp_airports.db_is_open_and_ready)
        {
          std::string stmt_uq_name = "count_xp_loc";
          int         counter      = 0;

          // check xp_loc - localizer table row count
          if (data_manager::db_xp_airports.prepareNewStatement(stmt_uq_name, "select count(1) as count_rows from xp_loc"))
          {
            assert(data_manager::db_xp_airports.mapStatements[stmt_uq_name] != nullptr);

            int rc = data_manager::db_xp_airports.step(data_manager::db_xp_airports.mapStatements[stmt_uq_name]);
            if (rc == SQLITE_ROW)
            {
              counter = sqlite3_column_int(data_manager::db_xp_airports.mapStatements[stmt_uq_name], 0); // there is only one column in this query
            }

            sqlite3_clear_bindings(data_manager::db_xp_airports.mapStatements[stmt_uq_name]);

            if (sqlite3_reset(data_manager::db_xp_airports.mapStatements[stmt_uq_name]) != SQLITE_OK) // reset stmt pointer
            {
              Log::logMsg(std::string("ERROR: [db exec error] reset: ") + sqlite3_errmsg(data_manager::db_xp_airports.db) + "\n");
            }
          }

          // check xp_rw - runway table
          if (counter > 1000) // if previous query return acceptable number of rows (should be  > 4000 ) then do the same against runway table
          {
            counter      = 0;
            stmt_uq_name = "count_xp_rw";
            if (data_manager::db_xp_airports.prepareNewStatement(stmt_uq_name, "select count(1) as count_rows from xp_rw"))
            {
              assert(data_manager::db_xp_airports.mapStatements[stmt_uq_name] != nullptr);

              int rc = data_manager::db_xp_airports.step(data_manager::db_xp_airports.mapStatements[stmt_uq_name]);
              if (rc == SQLITE_ROW)
              {
                counter = sqlite3_column_int(data_manager::db_xp_airports.mapStatements[stmt_uq_name], 0); // there is only one column in this query
              }

              sqlite3_clear_bindings(data_manager::db_xp_airports.mapStatements[stmt_uq_name]);

              if (sqlite3_reset(data_manager::db_xp_airports.mapStatements[stmt_uq_name]) != SQLITE_OK) // reset stmt pointer
              {
                Log::logMsg(std::string("ERROR: [db exec error] reset: ") + sqlite3_errmsg(data_manager::db_xp_airports.db) + "\n");
              }


              if (counter < 10000) // if we have more than 10000 rows of runway information then we estimate that the database holds valid data.
                counter = 0;
            }
          }

          // Decisions
          if (counter <= 0)
          {
            Mission::uiImGuiBriefer->strct_ils_layer.layer_state = missionx::mx_layer_state_enum::failed_data_is_not_present;
            Mission::uiImGuiBriefer->setMessage("Failed to validate the presence of valid ILS data information. Suggestion: Re-Run apt.dat optimization.");
          }
          else
          {
            Mission::uiImGuiBriefer->strct_ils_layer.layer_state = missionx::mx_layer_state_enum::success_can_draw;
          }

        } // end if airports_db is open and ready
        else
        {
          // Failure to initiate the database
          Mission::uiImGuiBriefer->strct_ils_layer.layer_state = missionx::mx_layer_state_enum::fatal_database_is_not_initializing_correctly;
          Mission::uiImGuiBriefer->setMessage("ERROR!!! Mission-X plugin failed to load/open the local database in {missionx/db} folder. Try to delete it and rerun apt.dat optimization.");
        }
      }
      break;
      case missionx::mx_flc_pre_command::start_random_mission: // v3.0.219.1
      {
        this->prepareMissionBrieferInfo(); // v3.0.241.10 b3 refresh list of all missions in folders. Should pick new mission files created by plugin and might not be present before
        this->prepareUiMissionList();      // will reload data and images

        if (missionx::data_manager::selectedMissionKey.empty())                                // v3.0.241.10 b3 do not automatically assume to use "random.xml" file since it might be based on "template mission folder" = "template.xml".
          missionx::data_manager::selectedMissionKey = mxconst::get_RANDOM_MISSION_DATA_FILE_NAME(); // v3.0.241.1 replaced uiWinBriefer.mediaBriefer.selectedMissionKey // reset and hide generate file button after successfull creation

        Mission::loadMission(); // during load we will call stopMission()
        if (data_manager::missionState == missionx::mx_mission_state_enum::mission_loaded_from_the_original_file)
          this->START_MISSION(); //
      }
      break;
      case missionx::mx_flc_pre_command::start_mission:
      {
        this->START_MISSION(); //
      }
      break;
      case missionx::mx_flc_pre_command::position_camera:
      {
        // position camera view
        missionx::data_manager::set_camera_poistion_loc_rule(0); // Zero means we do not loss control of camera to XP

        XPLMControlCamera(xplm_ControlCameraUntilViewChanges, missionx::setCameraPosition, nullptr);

        this->delay_camera_position_change = 0;
        missionx::data_manager::postFlcActions.push(missionx::mx_flc_pre_command::stop_position_camera); // Position plane. Use action in Queue. Should position plane
      }
      break;
      case missionx::mx_flc_pre_command::stop_position_camera:
      {
        XPLMCameraControlDuration outCameraControlDuration;
        if (XPLMIsCameraBeingControlled(&outCameraControlDuration))        
        {
          if (this->delay_camera_position_change >= 1)
          {
            data_manager::execute_commands("sim/view/free_camera");
            this->delay_camera_position_change = 0; // reset state
          }
          else
          {
            ++this->delay_camera_position_change;
            missionx::data_manager::postFlcActions.push(missionx::mx_flc_pre_command::stop_position_camera); // Position plane. Use action in Queue. Should position plane
          }

        }
      }
      break;
      case missionx::mx_flc_pre_command::dont_control_camera:
      {
        XPLMCameraControlDuration outCameraControlDuration;
        if (XPLMIsCameraBeingControlled(&outCameraControlDuration))        
        {
          XPLMDontControlCamera();
        }
      }
      break;
      case missionx::mx_flc_pre_command::position_plane:
      {
        if (data_manager::missionState == mx_mission_state_enum::mission_loaded_from_the_original_file)
        {
          // v3.0.221.15rc4 disable position plane is override flightLeg name was issued
          std::string       err                      = "";
          const std::string overrideLegName          = Utils::readAttrib(data_manager::mx_global_settings.xDesigner_ptr, mxconst::get_ATTRIB_FORCE_LEG_NAME(), "");
          const bool        flag_auto_position_plane = Utils::readBoolAttrib(data_manager::mx_global_settings.xPosition_ptr, mxconst::get_ATTRIB_AUTO_POSITION_PLANE(), true); // v3.0.241.1  


          if (overrideLegName.empty() && flag_auto_position_plane)
            data_manager::briefer.positionPlane(missionx::data_manager::flag_setupForcePlanePositioningAtMissionStart, missionx::data_manager::flag_setupChangeHeadingEvenIfPlaneIn_20meter_radius, data_manager::xplane_ver_i);

          // v3.303.12 apply weather from global settings
          if (!missionx::data_manager::mx_global_settings.xWeather_ptr.isEmpty())
          {
            missionx::data_manager::apply_datarefs_from_text_based_on_parent_node_and_tag_name(missionx::data_manager::mx_global_settings.xWeather_ptr, ""); // empty tag name means use the parent node as the weather one
            missionx::data_manager::execute_commands("sim/operation/regen_weather"); // immediately load the weather set, do not interpolate it
          }

          data_manager::missionState = missionx::mx_mission_state_enum::pre_mission_running;                              // we will change to running after another flight callback
          missionx::data_manager::postFlcActions.push(missionx::mx_flc_pre_command::post_mission_load_change_to_running); // buying another flight callback
        }
        else if (data_manager::missionState == mx_mission_state_enum::mission_loaded_from_savepoint)
        {
          // we force heading after mission was loaded in the hope to solve a bug where the plane always rotats to 180 deg
          const float heading_psi_f = Utils::readNodeNumericAttrib<float>(data_manager::mx_global_settings.node, mxconst::get_ATTRIB_HEADING_PSI(), XPLMGetDataf(drefConst.dref_heading_psi_f) ); // v3.303.8.3
          data_manager::briefer.setPlaneHeading(heading_psi_f);

          // v3.303.13 apply SAVED weather from global settings
          if (!missionx::data_manager::mx_global_settings.xSavedWeather_ptr.isEmpty())
          {
            missionx::data_manager::apply_datarefs_from_text_based_on_parent_node_and_tag_name(missionx::data_manager::mx_global_settings.xSavedWeather_ptr, ""); // empty tag name means use the parent node as the weather one
            missionx::data_manager::execute_commands("sim/operation/regen_weather"); // immediately load the weather set, do not interpolate it
          }


          data_manager::missionState = missionx::mx_mission_state_enum::pre_mission_running; // we will change to running after another flight callback

          missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::post_position_plane);
          missionx::data_manager::postFlcActions.push(missionx::mx_flc_pre_command::post_mission_load_change_to_running); // buying another flight callback
        }

        // v3.0.221.10 start cold and dark ?
        const bool flag_startColdAndDark =
          Utils::readBoolAttrib(data_manager::briefer.xLocationAdjust, mxconst::get_ATTRIB_START_COLD_AND_DARK(), false); //  
        if (flag_startColdAndDark)
        {
          this->start_cold_and_dark();
        }

        missionx::data_manager::refresh_3d_objects_and_cues_after_location_transition(); // v225.1 force re-calculation of CueInfo and 3D instances
      }
      break;
      case missionx::mx_flc_pre_command::post_mission_load_change_to_running:
      {
        data_manager::missionState = missionx::mx_mission_state_enum::mission_is_running; // we are buying more flight callback to allow time and other pre-mission settings to set before calling any script that may fail the mission.
      }
      break;
      case missionx::mx_flc_pre_command::post_position_plane:
      {
        data_manager::briefer.postPositionPlane(); // currently function doesn't do anything
        const std::string mission_state_s = Utils::readAttrib(data_manager::mx_global_settings.node, mxconst::get_PROP_MISSION_STATE(), ""); // empty value means new mission and not loaded checkpoint
        if (mission_state_s.empty())
          missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::set_time);


        if ((!data_manager::briefer.xLocationAdjust.isEmpty() && Utils::readBoolAttrib(data_manager::briefer.xLocationAdjust, mxconst::get_ATTRIB_PAUSE_AFTER_LOCATION_ADJUST(), false)) || Briefer::need_to_pause_xplane)
          missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::pause_xplane); // place action in Queue.
        else
          missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::unpause_xplane); // place action in Queue

        // re-calculate 3D objects local + re-position + Cue Points
        missionx::data_manager::refresh3DInstancesAndPointLocation();

      }
      break;
      case missionx::mx_flc_pre_command::pause_xplane:
      {
        const bool val_pause_in_vr = Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_PAUSE_IN_VR(), mxconst::DEFAULT_AUTO_PAUSE_IN_VR);
        const bool val_pause_in_2d = Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_PAUSE_IN_2D(), mxconst::DEFAULT_AUTO_PAUSE_IN_2D);

        if (!missionx::dataref_manager::isSimPause() && ((!missionx::mxvr::vr_display_missionx_in_vr_mode && val_pause_in_2d) || val_pause_in_vr /*v3.0.221.6 dont pause in VR*/)) // if x-plane not in pause mode
        {
          XPLMCommandKeyStroke(xplm_key_pause);
          if (missionx::dataref_manager::isSimPause())
          {
            Mission::uiImGuiBriefer->setPluginPausedSim(true); //
          }
        }
      }
      break;
      case missionx::mx_flc_pre_command::force_pause_xplane:
      {
        // We use this call when loading savepoint and we are in VR and in air. In 2D mode we are in pause mode so the is not an issue.
        if (!missionx::dataref_manager::isSimPause()) // v3.0.231.1 force pause if not in pause. no matter if you are in VR or not VR mode. Also ignore user options.
        {
          XPLMCommandKeyStroke(xplm_key_pause);
          if (missionx::dataref_manager::isSimPause())
          {
            Mission::uiImGuiBriefer->setPluginPausedSim(true); //
            Mission::uiImGuiBriefer->setForcePauseSim(true);   //
          }
        }
      }
      break;
      case missionx::mx_flc_pre_command::unpause_xplane:
      {
        if (missionx::dataref_manager::isSimPause()) // if x-plane in pause mode release it
        {
          XPLMCommandKeyStroke(xplm_key_pause);
        }
      }
      break;
      case missionx::mx_flc_pre_command::create_savepoint_and_quit:
      {
        Mission::saveCheckpoint();
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::stop_mission); // TODO: We should consider an intermediate state
      }
      break;
      case missionx::mx_flc_pre_command::stop_mission:
      {
        if (data_manager::missionState <= missionx::mx_mission_state_enum::mission_loaded_from_savepoint)
        {
          Mission::uiImGuiBriefer->setLayer(missionx::uiLayer_enum::imgui_home_layer); // v3.0.251.1
        }
        else
        {
          Mission::uiImGuiBriefer->setLayer(missionx::uiLayer_enum::imgui_home_layer); // v3.0.251.1
        }

        Mission::stopMission(); // need to replace this with MenuHandler ?

        missionx::data_manager::selectedMissionKey.clear(); // v3.0.241.1
        missionx::Message::lineAction4ui.init();
      }
      break;
      case missionx::mx_flc_pre_command::open_inventory_layout:
      {
        if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
        {
          Mission::uiImGuiBriefer->execAction(missionx::mx_window_actions::ACTION_TOGGLE_INVENTORY);
        }
        // end if running
      }
      break;
      case missionx::mx_flc_pre_command::open_map_layout:
      {
        if (data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
        {
          Mission::uiImGuiBriefer->execAction(missionx::mx_window_actions::ACTION_TOGGLE_MAP);
        }
      }
      break;
      case missionx::mx_flc_pre_command::open_story_layout: // v3.305.1
      {
#ifndef RELEASE
        Log::logMsg("Trying to open Story Layout"); // debug
#endif
        Mission::uiImGuiBriefer->execAction(missionx::mx_window_actions::ACTION_OPEN_STORY_LAYOUT);
      }
      break;
      case missionx::mx_flc_pre_command::toggle_auto_hide_show_mxpad_option: // v3.0.215.1
      {

        const bool val = Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_AUTO_HIDE_SHOW_MXPAD(), true); // toggle MXPAD if options is set

        missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(
          mxconst::get_OPT_AUTO_HIDE_SHOW_MXPAD(), !val); 
        missionx::system_actions::store_plugin_options();

        if (Mission::uiImGuiMxpad)
          Mission::uiImGuiMxpad->setWasHiddenByAutoHideOption(false); // reset flag state

        this->syncOptionsWithMenu(); // v3.0.215.1
      }
      break;
      case missionx::mx_flc_pre_command::abort_mission:
      {
        data_manager::missionState = mx_mission_state_enum::mission_aborted; // same as completed failure

        this->setUiEndMissionTexture(); // v3.0.157 // should load the failure texture
      }
      break;
      case missionx::mx_flc_pre_command::disable_aptdat_optimize_menu:
      {
        const std::string msg = "Processing - APT.DAT optimization (" + OptimizeAptDat::aptState.getDuration() + "s)";
        XPLMSetMenuItemName(this->missionMenuEntry, this->mx_menu.optimizeAptDatMenu, msg.c_str(), 0);
        XPLMEnableMenuItem(this->missionMenuEntry, this->mx_menu.optimizeAptDatMenu, false);

        // v3.0.253.6 add progress message
        if ((missionx::data_manager::flag_apt_dat_optimization_is_running || OptimizeAptDat::aptState.flagIsActive) && Mission::uiImGuiBriefer != nullptr)
        {
          Mission::uiImGuiBriefer->setMessage(msg, 1);
        }
      }
      break;
      case missionx::mx_flc_pre_command::enable_aptdat_optimize_menu:
      {
        std::string msg = "apt.dat optimization (run in background: 3-8min)";
        if (!OptimizeAptDat::aptState.duration_s.empty())
          msg += "(last run: " + OptimizeAptDat::aptState.duration_s + "s)";

        XPLMSetMenuItemName(this->missionMenuEntry, this->mx_menu.optimizeAptDatMenu, msg.c_str(), 0);
        XPLMEnableMenuItem(this->missionMenuEntry, this->mx_menu.optimizeAptDatMenu, true);
      }
      break;
      case missionx::mx_flc_pre_command::disable_generator_menu:
      {
        XPLMEnableMenuItem(this->missionMenuEntry, this->mx_menu.optimizeAptDatMenu, false);
      }
      break;
      case missionx::mx_flc_pre_command::enable_generator_menu:
      {
        XPLMEnableMenuItem(this->missionMenuEntry, this->mx_menu.optimizeAptDatMenu, true);

        this->engine.working_tempFile_ptr = nullptr;
      }
      break;
      case missionx::mx_flc_pre_command::convert_icao_to_xml_point:
      {
        if (this->engine.shared_navaid_info.parentNode_ptr.isEmpty())
          break;

        // loop over all <icao> and convert to <point>
        int nChilds = this->engine.shared_navaid_info.parentNode_ptr.nChildNode(mxconst::get_ELEMENT_ICAO().c_str());

        for (int i1 = 0; i1 < nChilds; ++i1)
        {
          IXMLNode icaoNode = this->engine.shared_navaid_info.parentNode_ptr.getChildNode(mxconst::get_ELEMENT_ICAO().c_str(), i1); // we always take first element since we delete the node after handling it.
          if (icaoNode.isEmpty())
            continue;

          const std::string ID = Utils::readAttrib(icaoNode, mxconst::get_ATTRIB_NAME(), EMPTY_STRING);

          if (ID.empty())
            continue;

          NavAidInfo navAid;
          navAid.navRef = XPLMFindNavAid (nullptr, ID.c_str(), nullptr, nullptr, nullptr, xplm_Nav_Airport);

          // skip if no navaid found
          if (navAid.navRef == XPLM_NAV_NOT_FOUND)
          {
            Log::logAttention("\t[convert_icao_to_xml_point] Failed to find ICAO: " + ID + ". Skipping...");
            continue;
          }

          // fetch and store information on navAid if in correct distance
          XPLMGetNavAidInfo(navAid.navRef, &navAid.navType, &navAid.lat, &navAid.lon, &navAid.height_mt, &navAid.freq, &navAid.heading, navAid.ID, navAid.name, navAid.inRegion);
          navAid.synchToPoint();

          // v3.0.253.7 try to pick ramp
          std::string err;
          this->engine.filterAndPickRampBasedOnPlaneType(navAid, err, missionx::mxFilterRampType::airport_ramp); // this can cause performance issues

          // Add point element
          IXMLNode p = this->engine.shared_navaid_info.parentNode_ptr.addChild(navAid.node.deepCopy());
          if (p.isEmpty())
            Log::logAttention(std::string("\t[convert_icao_to_xml_point] Failed to add point to element: ") + this->engine.shared_navaid_info.parentNode_ptr.getName());
        }

        // remove all <icao> childs - to be on the safe side
        for (int i1 = (nChilds - 1); i1 >= 0; --i1)
          this->engine.shared_navaid_info.parentNode_ptr.getChildNode(mxconst::get_ELEMENT_ICAO().c_str(), i1).deleteNodeContent();

        nChilds = this->engine.shared_navaid_info.parentNode_ptr.nChildNode(mxconst::get_ELEMENT_ICAO().c_str());

        this->engine.threadState.thread_wait_state = missionx::mx_random_thread_wait_state_enum::finished_plugin_callback_job;
      }
      break;
      case missionx::mx_flc_pre_command::guess_waypoints_from_external_fpln_site:
      {
        constexpr double REGECT_WP_DISTANCE_OF_GUESSED_NAME = 0.3;
        // loop over all missionx::data_manager::tableExternalFPLN_vec, and try to find a nav aid for each listNavPoints
        for (auto& fpln : missionx::data_manager::tableExternalFPLN_vec)
        {
          // loop over all fpln points
          for (auto vec2d : fpln.listNavPoints)
          {

            missionx::mx_wp_guess_result result = data_manager::get_nearest_guessed_navaid_based_on_coordinate(vec2d);
            if (result.distance_d > REGECT_WP_DISTANCE_OF_GUESSED_NAME)
              result.name.clear();

            fpln.listNavPointsGuessedName.emplace_back(result); // the number of variables in fpln.listNavPointsGuessedName must be same as fpln.listNavPoints
            fpln.formated_nav_points_with_guessed_names_s.append(mxUtils::formatNumber<float>(vec2d.lat, 8) + ", " + mxUtils::formatNumber<float>(vec2d.lon, 8) +
                                                                 ((result.name.empty()) ? "\n" : " (" + result.name + ") " + Utils::getNavType_Translation(result.nav_type) + "\n")); // display lat/lon coordinates
          }
        }

        Mission::uiImGuiBriefer->strct_ext_layer.fetch_state = missionx::mxFetchState_enum::fetch_guess_wp_ended;
      }
      break;
      case missionx::mx_flc_pre_command::get_is_point_wet:
      {
        engine.shared_navaid_info.isWet = false;
        engine.shared_navaid_info.isWet = missionx::Point::probeIsWet(engine.shared_navaid_info.p, engine.shared_navaid_info.p.probe_result);

        this->engine.threadState.thread_wait_state = missionx::mx_random_thread_wait_state_enum::finished_plugin_callback_job;
      }
      break;
      case missionx::mx_flc_pre_command::calculate_slope_for_build_flight_leg_thread:
      {
        std::string err;
        NavAidInfo  navAidToCalcSlope;
        navAidToCalcSlope.lat = this->engine.threadState.pipeProperties.getAttribNumericValue<float>(mxconst::get_ATTRIB_LAT(), 0.0f);
        navAidToCalcSlope.lon = this->engine.threadState.pipeProperties.getAttribNumericValue<float>(mxconst::get_ATTRIB_LONG(), 0.0f);
        navAidToCalcSlope.p   = engine.shared_navaid_info.p; // v3.0.241.10 b3 replaced pipeProperties with Point p
        navAidToCalcSlope.syncPointToNav();

        const double slope = (double)this->engine.calc_slope_at_point_mainThread(navAidToCalcSlope);

        this->engine.threadState.pipeProperties.setNumberProperty(mxconst::get_ATTRIB_TERRAIN_SLOPE(), slope);
        this->engine.threadState.thread_wait_state = missionx::mx_random_thread_wait_state_enum::finished_plugin_callback_job;
      }
      break;
      case missionx::mx_flc_pre_command::gather_random_airport_mainThread:
      {
        // TODO - Deprecate this option as long as we use the database as a source for NavAid search
        assert(1 == 2 && "gather_random_airport_mainThread Should not be called"); // always fail // v3.303.12_r2


      }
      break;
      case missionx::mx_flc_pre_command::get_nearest_nav_aid_to_randomLastFlightLeg_mainThread:
      {
        #ifndef RELEASE
        auto startClock = std::chrono::steady_clock::now();
        #endif

        XPLMNavRef nav_ref = XPLMFindNavAid (nullptr, nullptr, &this->engine.lastFlightLegNavInfo.lat, &this->engine.lastFlightLegNavInfo.lon, nullptr, xplm_Nav_Airport);
        if (!(nav_ref == XPLM_NAV_NOT_FOUND))
        {
          XPLMGetNavAidInfo(nav_ref,
                            &this->engine.shared_navaid_info.navAid.navType,
                            &this->engine.shared_navaid_info.navAid.lat,
                            &this->engine.shared_navaid_info.navAid.lon,
                            &this->engine.shared_navaid_info.navAid.height_mt,
                            &this->engine.shared_navaid_info.navAid.freq,
                            &this->engine.shared_navaid_info.navAid.heading,
                            this->engine.shared_navaid_info.navAid.ID,
                            this->engine.shared_navaid_info.navAid.name,
                            this->engine.shared_navaid_info.navAid.inRegion);
        }

        this->engine.threadState.thread_wait_state = missionx::mx_random_thread_wait_state_enum::finished_plugin_callback_job;


        #ifndef RELEASE
        const auto   endThreadClock = std::chrono::steady_clock::now();
        const auto   diff           = endThreadClock - startClock;
        const double duration       = std::chrono::duration<double, std::milli>(diff).count();

        Log::logMsg("*** Finished get_nearest_nav_aid_to_coordiante_mainThread. Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec)  ****");
        #endif
      }
      break;
      case missionx::mx_flc_pre_command::get_nearest_nav_aid_to_custom_lat_lon_mainThread: // v3.0.241.10 b2 use this option to see if for example: osm location is correct, especially for helipads
      {
        #ifndef RELEASE
        auto startClock = std::chrono::steady_clock::now();
        #endif

        XPLMNavRef nav_ref = XPLMFindNavAid (nullptr, nullptr, &this->engine.shared_navaid_info.navAid.lat, &this->engine.shared_navaid_info.navAid.lon, nullptr, xplm_Nav_Airport);
        if (nav_ref != XPLM_NAV_NOT_FOUND)
        {
          XPLMGetNavAidInfo(nav_ref,
                            &this->engine.shared_navaid_info.navAid.navType,
                            &this->engine.shared_navaid_info.navAid.lat,
                            &this->engine.shared_navaid_info.navAid.lon,
                            &this->engine.shared_navaid_info.navAid.height_mt,
                            &this->engine.shared_navaid_info.navAid.freq,
                            &this->engine.shared_navaid_info.navAid.heading,
                            this->engine.shared_navaid_info.navAid.ID,
                            this->engine.shared_navaid_info.navAid.name,
                            this->engine.shared_navaid_info.navAid.inRegion);
        }

        this->engine.threadState.thread_wait_state = missionx::mx_random_thread_wait_state_enum::finished_plugin_callback_job;


        #ifndef RELEASE
        const auto   endThreadClock = std::chrono::steady_clock::now();
        const auto   diff           = endThreadClock - startClock;
        const double duration       = std::chrono::duration<double, std::milli>(diff).count();

        Log::logMsg("*** Finished get_nearest_nav_aid_to_custom_lat_lon_mainThread. Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec)  ****");
        #endif
      }
      break;
      case missionx::mx_flc_pre_command::get_nav_aid_info_mainThread: // v3.0.241.10 b2 use this option to see if for example: osm location is correct, especially for helipads
      {
#ifndef RELEASE
        auto startClock = std::chrono::steady_clock::now();
#endif

#ifdef IBM
        this->engine.shared_navaid_info.navAid = data_manager::getICAO_info(this->engine.shared_navaid_info.navAid.getID());
#else
        auto tempNav                           = data_manager::getICAO_info(this->engine.shared_navaid_info.navAid.getID());
        this->engine.shared_navaid_info.navAid = tempNav;
#endif


        this->engine.threadState.thread_wait_state = missionx::mx_random_thread_wait_state_enum::finished_plugin_callback_job;


#ifndef RELEASE
        const auto   endThreadClock = std::chrono::steady_clock::now();
        const auto   diff           = endThreadClock - startClock;
        const double duration       = std::chrono::duration<double, std::milli>(diff).count();

        Log::logMsg("*** Finished get_nav_aid_info_mainThread. Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec)  ****");
#endif
      }
      break;

      case missionx::mx_flc_pre_command::execute_xp_command:
      {
        if (!missionx::data_manager::queCommands.empty())
        {
          const std::string command_name = missionx::data_manager::queCommands.front();
          missionx::data_manager::queCommands.pop_front();
          if (!command_name.empty() && Utils::isElementExists(missionx::data_manager::mapCommands, command_name))
          {
            if (missionx::data_manager::mapCommands[command_name].secToExecute > 0.0f)
            {
              missionx::data_manager::queCommandsWithTimer.push_back(command_name);
            }
            XPLMCommandOnce(missionx::data_manager::mapCommands[command_name].ref);
#ifndef RELEASE
            Log::logMsg("[flcPre]executing registered command: " + command_name);
#endif
          }
          else
          {
#ifndef RELEASE
            Log::logMsg("[flcPre]Fail to find X-Plane command: " + command_name);
#endif
          }


          if (!missionx::data_manager::queCommands.empty()) // call flcPRE() again if we still have commands to execute
            missionx::data_manager::queFlcActions.push(
              missionx::mx_flc_pre_command::execute_xp_command); // v3.0.221.10 this to make the commands fire immediately and not wait for next FlightCallBack iteration. That way simmer should see all commands running at once.
        }

        ///// Manage timer based commands = commands that has press time
        std::queue<std::string> queEraseTimeBaseCommand;
        if (!missionx::data_manager::queCommandsWithTimer.empty())
        {
          for (const auto &command_name : missionx::data_manager::queCommandsWithTimer)
          {
            missionx::data_manager::mapCommands[command_name].flc();

            // decide if to erase from timer list
            if (missionx::data_manager::mapCommands[command_name].phase == missionx::BindCommand::mxstatus_command::cmd_end)
              queEraseTimeBaseCommand.push(command_name); // erase command after finish its timer

          } // end loop over commands with Timer

          // erase timer commands if there are any to erase due to their timer being achieved.
          while (!queEraseTimeBaseCommand.empty())
          {
            const std::string cmdToErase = queEraseTimeBaseCommand.front();
            queEraseTimeBaseCommand.pop();
            auto iter = missionx::data_manager::queCommandsWithTimer.begin();

#ifndef RELEASE
            Log::logMsg("[flcPre] Erase timer command: " + cmdToErase);
#endif


            while (iter != missionx::data_manager::queCommandsWithTimer.end())
            {
              std::string cmd = *(iter);

              if (cmdToErase.compare(cmd) == 0 && iter != missionx::data_manager::queCommandsWithTimer.end())
              {
                missionx::data_manager::queCommandsWithTimer.erase(iter);
                break; // exit for loop
              }
              ++iter;

            } // end loop over command with timer queue

            missionx::data_manager::mapCommands[cmdToErase].phase = BindCommand::mxstatus_command::cmd_not_pressed; // reset command

          } // end loop over commands to remove from queCommandsWithTimer queu.


        } // end if queCommandTimer has elements
      }
      break;
      case missionx::mx_flc_pre_command::inject_metar_file:
      {
        std::string error_msg;
        error_msg.clear();
        system_actions sa;
        // bool status = false;
        static std::string metar_folder_path     = data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_ATTRIB_METAR_FOLDER_NAME(), "", error_msg);
        static std::string xplane_install_folder = Utils::getXPlaneInstallFolder();

        // check flightLeg has metar
        if (!data_manager::metar_file_to_inject_s.empty())
        {
          std::string metar_file = data_manager::metar_file_to_inject_s;

          if (missionx::data_manager::xplane_ver_i < missionx::XP12_VERSION_NO)
            dataref_manager::set_xplane_dataref_value("sim/weather/download_real_weather", FALSE);


          const bool status = missionx::system_actions::copy_file(metar_folder_path + metar_file, xplane_install_folder + mxconst::get_XPLANE_METAR_FILENAME(), /*"METAR.rwx", "r", "w",*/ error_msg);
          if (status) // if true = copy success
          {
            Mission::usingCustomMetarFile      = true; // this will allow us to evaluate force metar checks.
            data_manager::current_metar_file_s = data_manager::metar_file_to_inject_s;
            data_manager::metar_file_to_inject_s.clear();


            if (missionx::data_manager::xplane_ver_i < missionx::XP12_VERSION_NO)
            {
              dataref_manager::set_xplane_dataref_value("sim/weather/use_real_weather_bool", TRUE);

              // Manually execute load_real_weather command
              const std::string command_scan_weather = "sim/operation/load_real_weather";
              XPLMCommandRef    cmdRef               = XPLMFindCommand(command_scan_weather.c_str());
              if (cmdRef == nullptr)
                Log::logMsgErr(">>> [Metar Scan Error] Fail to find scan command <<< ");
              else
              {
                XPLMCommandOnce(cmdRef);
                Log::logDebugBO("[metar] Injected custom METAR file: " + metar_file);
              }
            } // end if XP11
          }
          else
            Log::logMsgErr(error_msg);
        }
      }
      break;
      case missionx::mx_flc_pre_command::display_choice_window:
      {
        if (missionx::data_manager::mxChoice.is_choice_set())
        {

          // v3.0.251.1 imgui choice window
          const auto lmbda_prepare_uiImguiOptions = [&]() {
            int left, top, right, bottom;
            Utils::getWinCoords(left, top, right, bottom); // 410, 200, 75, 30,
            static constexpr const int _2_LINES = 2;

            if (Mission::uiImGuiOptions == nullptr) // if Options window was never set
            {

              if (Mission::uiImGuiMxpad && Mission::uiImGuiMxpad->mWindow && Mission::uiImGuiMxpad->IsInsideSim() && Mission::uiImGuiMxpad->GetVisible())
              {
                XPLMGetWindowGeometry(Mission::uiImGuiMxpad->mWindow, &left, &top, &right, &bottom);
                // same width & position as uiMxPad
                // left & right are the same
                // place choice window below mxPad in 2D mode
                top = bottom - 10;
              }
              else
              {
                // Position widget relative to windows borders
                // left & right
                left  = right - missionx::WinImguiOptions::MAX_WIDTH - 20;
                right = left + missionx::WinImguiOptions::MAX_WIDTH;
                // top & bottom
                top = top - 270; // top = mxPad top - height constant. This will be the position top of the options window
              }
              bottom = top - (((int)missionx::data_manager::mxChoice.mapOptions.size() + _2_LINES) * missionx::WinImguiOptions::LINE_HEIGHT) + missionx::WinImguiOptions::OPTION_BOTTOM_PADDING;

              // Create OPTIONS/Choices screen in 2D
              Mission::uiImGuiOptions = std::make_shared<WinImguiOptions>(left, top, right, bottom, xplm_WindowDecorationSelfDecoratedResizable, xplm_WindowLayerFloatingWindows); // decoration and layer will use default values
              if (Mission::uiImGuiOptions && Mission::uiImGuiMxpad)
                Mission::uiImGuiMxpad->optionsWindow = Mission::uiImGuiOptions->mWindow;

            } // end if  Mission::uiImGuiOptions == nullptr
            else
            {

              if (!missionx::mxvr::vr_display_missionx_in_vr_mode && Mission::uiImGuiOptions != nullptr && Mission::uiImGuiOptions->mWindow != nullptr)
              {
                if (Mission::uiImGuiMxpad && Mission::uiImGuiMxpad->mWindow && Mission::uiImGuiMxpad->IsInsideSim())
                {
                  XPLMGetWindowGeometry(Mission::uiImGuiMxpad->mWindow, &left, &top, &right, &bottom);
                  top = bottom - 10;
                }
                else
                  XPLMGetWindowGeometry(Mission::uiImGuiOptions->mWindow, &left, &top, &right, &bottom);

                bottom = top - (((int)missionx::data_manager::mxChoice.mapOptions.size() + _2_LINES) * missionx::WinImguiOptions::LINE_HEIGHT) + missionx::WinImguiOptions::OPTION_BOTTOM_PADDING;
                XPLMSetWindowGeometry(Mission::uiImGuiOptions->mWindow, left, top, right, bottom);
              }
            }


            return (Mission::uiImGuiOptions != nullptr);
          }; // end lambda

          if (lmbda_prepare_uiImguiOptions())
          {
            assert(Mission::uiImGuiOptions != nullptr && "uiImGuiOptions was not initialized correctly. ");
            Mission::uiImGuiOptions->SetVisible(true);
          }
        }
        else
        {
          if (this->uiImGuiBriefer)
            this->uiImGuiBriefer->setMessage("Can't open choice window, configuration is invalid. Check name of 'choice' or if you set the name before opening it.");

          Log::logMsgWarn("[display_choice_window] Choice window options are not set correctly. Check your implementation.");
        }
      }
      break;
      case missionx::mx_flc_pre_command::hide_choice_window:
      {
        // v3.0.251.1 hide choice window (imgui)
        if (Mission::uiImGuiOptions && Mission::uiImGuiOptions->GetVisible())
        {
          Mission::uiImGuiOptions->SetVisible(false);
        }
        else if (!missionx::mxvr::flag_in_vr)
        {
          Log::logMsg("[hide_choice_window] Choice window is already hidden.");
        }

        missionx::data_manager::mxChoice.currentChoiceName_beingDisplayed_s.clear(); // v24.02.6 remove4 current choice name to hide choice window in VR too


      }
      break;
      case missionx::mx_flc_pre_command::handle_option_picked_from_choice:
      {
        // We first check if the Choice UI is visible in 2D or If we are in VR (where there is no 2D choice window). Last we test if user picked one of the options
        if (((Mission::uiImGuiOptions && Mission::uiImGuiOptions->GetVisible()) || (missionx::mxvr::vr_display_missionx_in_vr_mode)) && missionx::data_manager::mxChoice.optionPicked_key_i >= 0)
        {
#ifndef RELEASE
          Log::logMsg("[handle_option_picked_from_choice] Handling option: " + Utils::formatNumber<int>(missionx::data_manager::mxChoice.optionPicked_key_i));
#endif // !RELEASE

          Mission::handle_choice_option(); // Execute the choice handler code

          // Position Choice window again if needed
          assert(Mission::uiImGuiOptions->mWindow != NULL && "uiImGuiOptions->mWindow should not be 0");
          assert(Mission::uiImGuiMxpad->mWindow != NULL && "uiImGuiMxpad->mWindow should not be 0");
          if (Mission::uiImGuiOptions->GetVisible() && Mission::uiImGuiMxpad != nullptr && Mission::uiImGuiMxpad->GetVisible())
          {
            int left, top, right, bottom;
            //XPLMGetWindowGeometry(Mission::uiImGuiOptions->mWindow, &left, &top, &right, &bottom);
            XPLMGetWindowGeometry(Mission::uiImGuiMxpad->mWindow, &left, &top, &right, &bottom);
            static constexpr int _2_LINES = 2;
            top                           = bottom - _2_LINES;
            bottom                        = top - (((int)missionx::data_manager::mxChoice.mapOptions.size() + _2_LINES) * missionx::WinImguiOptions::LINE_HEIGHT) + missionx::WinImguiOptions::OPTION_BOTTOM_PADDING;
            XPLMSetWindowGeometry(Mission::uiImGuiOptions->mWindow, left, top, right, bottom);
          }
        }
        else
        {
          Log::logMsg("[handle_option_picked_from_choice] Choice window is hidden or options picked is not valid.");
        }
      }
      break;
      case missionx::mx_flc_pre_command::eval_end_flight_leg_after_all_qmm_broadcasted:
      {
        if (QueueMessageManager::is_queue_empty())
        {
          Log::logDebugBO("Flight Leg messages are clean");
          data_manager::mapFlightLegs[data_manager::currentLegName].setIsComplete(true, missionx::enums::mx_flightLeg_state::leg_is_ready_for_next_flight_leg);
        }

      }
      break;
      case missionx::mx_flc_pre_command::end_mission_after_all_qmm_broadcasted:
      {
        if (QueueMessageManager::is_queue_empty())
        {
          Log::logDebugBO("Calling UI End mission.");
          setUiEndMissionTexture();
        }
        else
        {
          Log::logDebugBO("Still waiting for mxconst::QMM to clear its Queue");

          missionx::data_manager::postFlcActions.push(missionx::mx_flc_pre_command::end_mission_after_all_qmm_broadcasted);
        }
      }
      break;
      case missionx::mx_flc_pre_command::special_test_place_instance:
      {
        static int seq = 0;
        seq++;
        std::string inst_name = "test_inst_" + Utils::formatNumber<int>(seq);
        // get camera terrain elevation
        Point p1 = missionx::data_manager::getCameraLocationTerrainInfo();
        obj3d obj3d_inst;

        obj3d_inst.g_object_ref = XPLMLoadObject("Resources/default scenery/airport scenery/Common_Elements/Lighting/Dir_Flood_Sm.obj"); // Load the 3D Object
        if (obj3d_inst.g_object_ref)                                                                                                     // debug
        {

          Log::logMsg("Loaded: " + obj3d_inst.file_and_path + mxconst::get_UNIX_EOL());

          // obj3d_inst.parse_node();
          obj3d_inst.name = inst_name; // setName(inst_name);
          obj3d_inst.deqPoints.clear();
          obj3d_inst.addPoint(p1);
          Utils::addElementToMap(data_manager::map3dInstances, inst_name, obj3d_inst);

          data_manager::map3dInstances[inst_name].g_object_ref      = obj3d_inst.g_object_ref;
          data_manager::map3dInstances[inst_name].displayCoordinate = data_manager::map3dInstances[inst_name].calculateCenterOfShape(data_manager::map3dInstances[inst_name].deqPoints);
          data_manager::map3dInstances[inst_name].calculate_real_elevation_to_DisplayCoordination();
          data_manager::map3dInstances[inst_name].node.deleteAttribute(mxconst::get_ATTRIB_LINK_TASK().c_str());
          data_manager::map3dInstances[inst_name].node.deleteAttribute(mxconst::get_PROP_LINK_OBJECTIVE_NAME().c_str());

          data_manager::map3dInstances[inst_name].create_instance();
          data_manager::map3dInstances[inst_name].positionInstancedObject();
        }
        else
          Log::logMsgErr("Fail to create special Test instance");
      }
      break;
      case missionx::mx_flc_pre_command::restart_all_plugins:
      {
        XPLMReloadPlugins();
      }
      break;
      case missionx::mx_flc_pre_command::abort_random_engine:
      {
        this->engine.abortThread();
      }
      break;
      case missionx::mx_flc_pre_command::toggle_target_marker_option: // v3.0.253.9.1 store the toggle option
      {
        const auto val = Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_DISPLAY_TARGET_MARKERS(), true); // toggle

        missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_SETUP_DISPLAY_TARGET_MARKERS(), !val);
      }
      break;
      case missionx::mx_flc_pre_command::hide_target_marker_option: // v3.0.253.9.1 store the toggle option
      {
        missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_SETUP_DISPLAY_TARGET_MARKERS(), false); // false = hide //  // 0 = hide
      }
      break;
      case missionx::mx_flc_pre_command::show_target_marker_option: // v3.0.253.9.1 store the toggle option
      {
        missionx::system_actions::pluginSetupOptions.setSetupNodeProperty<bool>(mxconst::get_SETUP_DISPLAY_TARGET_MARKERS(), true); // true = show // 1 = show
      }
      break;
      case missionx::mx_flc_pre_command::save_user_setup_options: // v3.0.255.4.2 save user prefers settings
      {
        missionx::system_actions::store_plugin_options();
      }
      break;
      case missionx::mx_flc_pre_command::write_fpln_to_external_folder:
      {
        std::string errView = missionx::data_manager::write_fpln_to_external_folder();
        

        if (errView.empty())
          Mission::uiImGuiBriefer->setMessage("Wrote to External FMS.", 5); // v3.305.3          
        else
          Log::logMsgErr(errView);
      }
      break;
      case missionx::mx_flc_pre_command::update_point_in_file_with_template_based_on_probe:
      {
#ifndef RELEASE
        missionx::Log::logMsg("[Missionx] update points.xml template (hover/land) base on terrain slope and water presence. Will create the file: points_mod.xml as output.");
#endif
        mx_mission_state_enum prevState = data_manager::missionState;
        if (data_manager::missionState != mx_mission_state_enum::mission_is_running && data_manager::missionState != mx_mission_state_enum::process_points)
        {

          auto start = std::chrono::steady_clock::now();

          data_manager::missionState = mx_mission_state_enum::process_points;
          if (!read_and_update_xml_file_point_elements_base_on_terrain_probing(""))
            XPLMSpeakString("Failure during point read/write. Check Log for errors.");

          const auto   end      = std::chrono::steady_clock::now();
          const auto   diff     = end - start;
          const double duration = std::chrono::duration<double, std::milli>(diff).count();
          Log::logMsgNone("[processed points] Duration: " + Utils::formatNumber<double>(duration, 3) + "ms (" + Utils::formatNumber<double>((duration / 1000), 3) + "sec)");
        }
        else
          XPLMSpeakString("Can't execute point probe while mission is running or processing points.");


        data_manager::missionState = prevState;
      }
      break;
      case missionx::mx_flc_pre_command::generate_mission_from_littlenavmap_fpln:
      {
        assert(Mission::uiImGuiBriefer != nullptr);

        const std::string filename = (Mission::uiImGuiBriefer->strct_conv_layer.file_picked_i > -1) ? Mission::uiImGuiBriefer->strct_conv_layer.vecFileList_char.at(Mission::uiImGuiBriefer->strct_conv_layer.file_picked_i) : mxconst::get_CONVERTER_FILE();
        
        if (missionx::data_manager::generate_missionx_mission_file_from_convert_screen( missionx::data_manager::prop_userDefinedMission_ui
                                                                                      // , filename
                                                                                      , Mission::uiImGuiBriefer->strct_conv_layer.xConvMainNode
                                                                                      , Mission::uiImGuiBriefer->strct_conv_layer.xSavedGlobalSettingsNode // v3.305.1
                                                                                      , missionx::data_manager::map_tableOfParsedFpln
                                                                                      , Mission::uiImGuiBriefer->strct_conv_layer.flag_store_state
                                                                                      , !Mission::uiImGuiBriefer->strct_conv_layer.flag_use_loaded_globalSetting_from_conversion_file ) // v3.305.1 send the oposite value of flag_use_loaded_globalSetting_from_conversion_file 
           )
        {
          Mission::uiImGuiBriefer->setMessage(R"(Mission file was generated as a "random" mission. You can run it from the "Load Mission" screen.)");

          // v3.305.1 refresh global_settings buffer if needed
          if (!Mission::uiImGuiBriefer->strct_conv_layer.flag_use_loaded_globalSetting_from_conversion_file) // if we created new global_Settings file then refresh the buffer
          {
            Mission::uiImGuiBriefer->strct_conv_layer.set_global_settings_into_buffer(Mission::uiImGuiBriefer->strct_conv_layer.xSavedGlobalSettingsNode); // if we reached here, xSavedGlobalSettingsNode should have been refreshed from generate_missionx_mission_file_from_convert_screen() function
          }
        }

      }
      break;
      case missionx::mx_flc_pre_command::read_async_inv_image_files:
      {

        for (const auto &[imgName, btnTexture] : data_manager::xp_mapInvImages)
        {
          glDeleteTextures(1, (const GLuint*)&btnTexture.gTexture); //
        }

        auto future = std::async(std::launch::async, &missionx::data_manager::loadInventoryImages);
      }
      break;
      case missionx::mx_flc_pre_command::post_async_inv_image_binding:
      {
        for (auto& [file, textureFile] : missionx::data_manager::xp_mapInvImages)
        {
          if (file.empty() || textureFile.getWidth() == 0 || textureFile.getHeight() == 0) // v3.0.303.7 hopefully will solve a bug if image file was not found 
            continue;

          XPLMGenerateTextureNumbers(&textureFile.gTexture, 1);
          XPLMBindTexture2d(textureFile.gTexture, 0);
          glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // added from imgui // v24.06.1 disabled

          const std::string err = this->checkGLError("After XPLMBindTexture2d");
          if (err.empty()){
#ifdef FLIP_IMAGE
              glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLint)inTextureFile.sImageData.Width, (GLint)inTextureFile.sImageData.Height, 0, ((inTextureFile.sImageData.Channels < 4) ? GL_RGB : GL_RGBA), GL_UNSIGNED_BYTE, img);
#else
              glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLint)textureFile.sImageData.Width, (GLint)textureFile.sImageData.Height, 0, ((textureFile.sImageData.Channels < 4) ? GL_RGB : GL_RGBA), GL_UNSIGNED_BYTE, textureFile.sImageData.pData);
#endif

              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
              // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // removed v3.0.251.1
              // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // removed v3.0.251.1
          }
          else
          {
              missionx::Log::logMsgThread("failed to set image: " + file + ", Error: " + err); // v24.06.1
          }

          stbi_image_free(textureFile.sImageData.pData);
#ifdef FLIP_IMAGE
          stbi_image_free(img);
#endif

          textureFile.sImageData.pData = nullptr;  
        }
      }
      break;
      case missionx::mx_flc_pre_command::set_story_auto_pause_timer:
      {
        if ( ! Message::lineAction4ui.vals[mxconst::get_STORY_PAUSE_TIME()].empty() && mxUtils::is_digits(Message::lineAction4ui.vals[mxconst::get_STORY_PAUSE_TIME()]) )
          this->uiImGuiBriefer->strct_flight_leg_info.strct_story_mode.setAutoSkipTimer(mxUtils::stringToNumber<float>(Message::lineAction4ui.vals[mxconst::get_STORY_PAUSE_TIME()])); // v3.305.1
        else 
          this->uiImGuiBriefer->strct_flight_leg_info.strct_story_mode.setAutoSkipTimer(mxconst::DEFAULT_SKIP_MESSAGE_TIMER_IN_SEC_F); // v3.305.1
      }
      break;
      case missionx::mx_flc_pre_command::post_story_message_cache_cleanup:
      {
#ifndef RELEASE
        Log::logMsg("Clearing story message cache");
#endif // !RELEASE

        missionx::data_manager::releaseMessageStoryCachedTextures(); // v3.305.1
      }
      break;
      case missionx::mx_flc_pre_command::hide_briefer_window_in_2D:
      {
        #ifndef RELEASE
          Log::logMsg("Hiding Main window in 2D mode only");
        #endif // !RELEASE

        if (!this->uiImGuiBriefer->IsInVR())
          this->uiImGuiBriefer->execAction(missionx::mx_window_actions::ACTION_HIDE_WINDOW);
      }
      break;
      case missionx::mx_flc_pre_command::post_async_story_image_binding:
      {
        for (auto& [file, textureFile] :missionx::Message::mapStoryCachedImages)
        {
          // we skip images we already bind using XPLMGenerateTextureNumbers and XPLMBindTexture2d
          if (file.empty() || textureFile.sImageData.Width == 0 || textureFile.sImageData.Height == 0 || textureFile.sImageData.pData == nullptr) // v3.305.1 if pData is not nullptr then it needs to be bind
            continue;

          XPLMGenerateTextureNumbers(&textureFile.gTexture, 1);
          XPLMBindTexture2d(textureFile.gTexture, 0);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

          glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // added from imgui
#ifdef FLIP_IMAGE
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLint)inTextureFile.sImageData.Width, (GLint)inTextureFile.sImageData.Height, 0, ((inTextureFile.sImageData.Channels < 4) ? GL_RGB : GL_RGBA), GL_UNSIGNED_BYTE, img);
#else
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLint)textureFile.sImageData.Width, (GLint)textureFile.sImageData.Height, 0, ((textureFile.sImageData.Channels < 4) ? GL_RGB : GL_RGBA), GL_UNSIGNED_BYTE, textureFile.sImageData.pData);
#endif


          stbi_image_free(textureFile.sImageData.pData);
#ifdef FLIP_IMAGE
          stbi_image_free(img);
#endif

          textureFile.sImageData.pData = nullptr;  
        }

        Message::lineAction4ui.state = missionx::enum_mx_line_state::action_ended;
      }
      break;
      case missionx::mx_flc_pre_command::get_current_weather_state_and_store_in_RandomEngine:
      {
        missionx::RandomEngine::current_weather_datarefs_s = missionx::data_manager::get_weather_state();
        this->engine.threadState.thread_wait_state         = missionx::mx_random_thread_wait_state_enum::finished_plugin_callback_job;
      }
      break;
      case missionx::mx_flc_pre_command::sound_abort_all_channels:
      {
        missionx::QueueMessageManager::stopAllPoolChannels();
      }
      break;
      case missionx::mx_flc_pre_command::toggle_designer_mode:
      {
        int val = Utils::readNodeNumericAttrib<int>( missionx::system_actions::pluginSetupOptions.node,mxconst::get_OPT_ENABLE_DESIGNER_MODE(), false); // 0 = false

        if (val == 0)
        {
          val = 1;
          if (!data_manager::currentLegName.empty() && Utils::isElementExists(data_manager::mapFlightLegs, data_manager::currentLegName))
            data_manager::mapFlightLegs[data_manager::currentLegName].flag_cue_was_calculated = false; // force recalculate of cues
        }
        else
          val = 0;

        missionx::system_actions::pluginSetupOptions.node.updateAttribute(mxUtils::formatNumber<int>(val).c_str(), mxconst::get_OPT_ENABLE_DESIGNER_MODE().c_str(), mxconst::get_OPT_ENABLE_DESIGNER_MODE().c_str()); // v3.303.8.3

        this->syncOptionsWithMenu();
      }
      break;
      case missionx::mx_flc_pre_command::toggle_cue_info_mode:
      {
          int val = Utils::readNodeNumericAttrib<int>( missionx::system_actions::pluginSetupOptions.node,mxconst::get_OPT_DISPLAY_VISUAL_CUES(), false);

          if (val == 0)
            val = 1;
          else
            val = 0;

          missionx::system_actions::pluginSetupOptions.node.updateAttribute(mxUtils::formatNumber<int>(val).c_str(), mxconst::get_OPT_DISPLAY_VISUAL_CUES().c_str(), mxconst::get_OPT_DISPLAY_VISUAL_CUES().c_str()); // v3.303.8.3

          this->syncOptionsWithMenu();
      }
      break;
      case missionx::mx_flc_pre_command::save_notes_info:
      {
        auto xNotes = Utils::xml_get_or_create_node_ptr( missionx::data_manager::xMissionxPropertiesNode, mxconst::get_ELEMENT_NOTES() );
        if ( !xNotes.isEmpty() && this->uiImGuiBriefer != nullptr )
        {
          Utils::xml_delete_all_subnodes(xNotes); // delete all subnodes if exists.
          for (const auto &[key, value] : this->uiImGuiBriefer->strct_flight_leg_info.mapNoteFieldShort )
          {
            if (mxUtils::isElementExists(missionx::enums_translation::trnsEnumNoteShort, key))
            {
              auto sName = missionx::enums_translation::trnsEnumNoteShort.at(key);
              auto xChild = xNotes.addChild(sName);
              if ( !xChild.isEmpty() )
                Utils::xml_set_text(xChild, value);
            }
          }

          for (const auto &[key, value] : this->uiImGuiBriefer->strct_flight_leg_info.mapNoteFieldLong )
          {
            if (mxUtils::isElementExists(missionx::enums_translation::trnsEnumNoteLong, key))
            {
              auto sName = missionx::enums_translation::trnsEnumNoteLong.at(key);
              auto xChild = xNotes.addChild(sName);
              if ( !xChild.isEmpty() )
                Utils::xml_set_text(xChild, value);
            }
          }

         missionx::system_actions::store_plugin_options();
         this->uiImGuiBriefer->setMessage("Saved Notes Data to Properties File.", 5);
        }
      }
      break;
      case missionx::mx_flc_pre_command::load_notes_info:
      {
        auto xNotes = Utils::xml_get_or_create_node_ptr( missionx::data_manager::xMissionxPropertiesNode, mxconst::get_ELEMENT_NOTES() );
        if ( !xNotes.isEmpty() && this->uiImGuiBriefer != nullptr )
        {
          // this->uiImGuiBriefer->strct_flight_leg_info.mapNoteFieldShort.clear();
          // this->uiImGuiBriefer->strct_flight_leg_info.mapNoteFieldLong.clear();
          this->uiImGuiBriefer->strct_flight_leg_info.initNoteMaps();

          for (const auto &[keyEnum, tagName] : missionx::enums_translation::trnsEnumNoteShort )
          {
            auto xTag = xNotes.getChildNode(tagName);
            if (!xTag.isEmpty())
            {
              std::string txt = Utils::xml_get_text(xTag,"\0");
              this->uiImGuiBriefer->strct_flight_leg_info.setNoteShortField(keyEnum, txt);
            }


            for (const auto &[keyEnum, tagName] : missionx::enums_translation::trnsEnumNoteLong )
            {
              auto xTag = xNotes.getChildNode(tagName);
              if (!xTag.isEmpty())
              {
                std::string txt = Utils::xml_get_text(xTag,"\0");
                this->uiImGuiBriefer->strct_flight_leg_info.setNoteLongField(keyEnum, txt);
              }
            }

            this->uiImGuiBriefer->setMessage("Loaded Notes Data from Properties File.", 5);
          }
        }
      }
      break;
      case missionx::mx_flc_pre_command::fetch_metar_data_after_nav_info:
      {
        // v24.03.2 Call METAR thread only if it is not active
        if (this->uiImGuiBriefer != nullptr)
        {
          if (missionx::data_manager::threadStateMetar.flagIsActive)
          {
            this->uiImGuiBriefer->setMessage("METAR thread is active, will skip metar information call.", 6);
          }
          else
          {
            bool bLock = true;
            data_manager::threadStateMetar.init();
            missionx::data_manager::mFetchFutures.push_back(
              std::async(std::launch::async, missionx::data_manager::fetch_METAR, &this->uiImGuiBriefer->strct_ils_layer.mapNavaidData, &this->uiImGuiBriefer->strct_ils_layer.fetch_metar_state, &this->uiImGuiBriefer->strct_ils_layer.asyncMetarFetchMsg_s, &Mission::uiImGuiBriefer->asyncSecondMessageLine, &bLock));
          }
        }
      }
      break;
      case missionx::mx_flc_pre_command::fetch_simbrief_fpln:
      {
        missionx::Mission::uiImGuiBriefer->execAction (missionx::mx_window_actions::ACTION_FETCH_FPLN_FROM_SIMBRIEF_SITE);
      }
      case missionx::mx_flc_pre_command::load_briefer_textures:
      {
        missionx::data_manager::readPluginTextures ();
      }
      break;
      default:
      {
      }
      break;
    }
  }
}

// -------------------------------------

void
missionx::Mission::injectMetarFile()
{
  /*

  Check if current Goal has metar file property.

  Datarefs to control weather:

  sim/weather/use_real_weather_bool
  sim/weather/download_real_weather

  * Set the "download real weather - first
  * Then copy over the new METAR file
  * Set: "use_real_weather" to true.
  * disable weather in step ( usingMetarFile = true )
  *
  */

  std::string error_msg;
  error_msg.clear();
  system_actions sa;

  // check flightLeg has metar
  bool              flag_found;
  const std::string metar_file_s = Utils::xml_get_attribute_value_drill(data_manager::mapFlightLegs[data_manager::currentLegName].node, mxconst::get_ATTRIB_METAR_FILE_NAME(), flag_found, mxconst::get_ELEMENT_METAR());
  if (!metar_file_s.empty())
  {
    // moving the code to flcPRE() and using data_manager as the main hub of the action we can do.
    missionx::data_manager::inject_metar_file(metar_file_s); // v3.0.241.1
  }
}

// -------------------------------------

void
missionx::Mission::AddFlightLegFailTimers()
{
  for (auto& [name, tmr] : missionx::data_manager::mapFlightLegs[missionx::data_manager::currentLegName].mapFailTimers)
  {
    if (!Utils::isElementExists(missionx::data_manager::mapFailureTimers, name)) // add fail timer only if it exists
    {
      Utils::addElementToMap(missionx::data_manager::mapFailureTimers, name, tmr);
#ifndef RELEASE
      Log::logMsg("Added Failure timer: " + missionx::Timer::to_string(missionx::data_manager::mapFailureTimers[name]));
#endif
    }
    break; // we add only one failure timer per flight leg from <xml> mission
  }
}

// -------------------------------------

void
missionx::Mission::start_cold_and_dark()
{
  // split missionx::data_manager::start_cold_and_dark_drefs to {dref_key, value}
  // create DREF param for each key and the value we will assign and then apply
  const bool val =
    Utils::getNodeText_type_1_5<bool>(missionx::system_actions::pluginSetupOptions.node,
                                      mxconst::get_OPT_DISABLE_PLUGIN_COLD_AND_DARK_WORKAROUND(),
                                      mxconst::DEFAULT_DISABLE_PLUGIN_COLD_AND_DARK); 
  if (val)
    return; // skip

  missionx::data_manager::start_cold_and_dark_drefs = "sim/flightmodel/controls/parkbrake=1|sim/flightmodel/engine/ENGN_mixt=0.0|sim/cockpit/electrical/avionics_on=0|sim/cockpit2/engine/actuators/throttle_ratio_all=0.0"; // +


  IXMLNode          xColdDark                     = data_manager::briefer.node.getChildNode(mxconst::get_ELEMENT_DATAREFS_START_COLD_AND_DARK().c_str());
  const std::string customMissionStartColdAndDark = Utils::xml_get_text(xColdDark); 
  if (!customMissionStartColdAndDark.empty())
  {
    missionx::data_manager::start_cold_and_dark_drefs = customMissionStartColdAndDark; // +"," + missionx::data_manager::start_cold_and_dark_drefs;
  }

  missionx::data_manager::apply_datarefs_based_on_string_parsing(missionx::data_manager::start_cold_and_dark_drefs); // v3.303.12
}

// -------------------------------------

void
missionx::Mission::flc_check_success()
{
//#ifdef TIMER_FUNC
//  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
//#endif // TIMER_FUNC

  // decide if to transition to the next Flight Leg or call end
  if (data_manager::mapFlightLegs[data_manager::currentLegName].getIsComplete())
  {
    

    if (data_manager::mapFlightLegs[data_manager::currentLegName].getFlightLegState() == missionx::enums::mx_flightLeg_state::leg_success)
    {
      // v3.305.1 change state to "mx_flightLeg_state::leg_success_do_post"
      data_manager::mapFlightLegs[data_manager::currentLegName].setIsComplete(true, missionx::enums::mx_flightLeg_state::leg_success_do_post);


     // v3.305.3 stop the first <timer> that was defined at the leg level if "stop_on_leg_end_b=true" and __only__ call the success message
      auto nTimer_ptr = Utils::xml_get_node_from_node_tree_IXMLNode(data_manager::mapFlightLegs[data_manager::currentLegName].node, mxconst::get_ELEMENT_TIMER(), false);
      if (!nTimer_ptr.isEmpty())
      {
        const std::string timerName = Utils::readAttrib(nTimer_ptr, mxconst::get_ATTRIB_NAME(), "");
        if (mxUtils::isElementExists(missionx::data_manager::mapFailureTimers, timerName)    // if exists
          && (Timer::wasEnded(missionx::data_manager::mapFailureTimers[timerName]) == false) // not ended
          && Utils::readBoolAttrib(missionx::data_manager::mapFailureTimers[timerName].node, mxconst::get_ATTRIB_STOP_ON_LEG_END_B(), false)) // flag stop at end of current flight leg
        {
          #ifndef RELEASE
          Log::logMsg("Calling success message for timer: '" + timerName + "', if it was defined.");
          #endif // !RELEASE

          missionx::data_manager::mapFailureTimers[timerName].setEnd(); // stop the timer
          // send success message if there is any
          std::string message = Utils::readAttrib(missionx::data_manager::mapFailureTimers[timerName].node, mxconst::get_ATTRIB_SUCCESS_MSG(), "");
          if (!message.empty())
          {
            std::string err, track;
            QueueMessageManager::addMessageToQueue(message, track, err);
          }
        }
      }


      // v3.0.221.9 call end Flight Leg commands
      if (!data_manager::mapFlightLegs[data_manager::currentLegName].listCommandsAtTheEndOfTheFlightLeg.empty())
      {
        while (!data_manager::mapFlightLegs[data_manager::currentLegName].listCommandsAtTheEndOfTheFlightLeg.empty())
        {
          missionx::data_manager::queCommands.push_back(data_manager::mapFlightLegs[data_manager::currentLegName].listCommandsAtTheEndOfTheFlightLeg.front());
          data_manager::mapFlightLegs[data_manager::currentLegName].listCommandsAtTheEndOfTheFlightLeg.pop_front();
        }
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::execute_xp_command);
      }


      // v3.0.207.5 Flight Leg extended features implementation
      bool flag_found_attrib;
      std::string err;

      const std::string scriptName = Utils::xml_get_attribute_value_drill(data_manager::mapFlightLegs[data_manager::currentLegName].node, mxconst::get_ATTRIB_NAME(), flag_found_attrib, mxconst::get_ELEMENT_POST_LEG_SCRIPT()); // v3.0.241.1
      if (!scriptName.empty())
      {
        missionx::data_manager::execScript(scriptName, data_manager::smPropSeedValues, "[flc] Post Leg script: " + scriptName + " is Invalid. Aborting mission."); // can alter success flag or message
      }

      // Send end flightLeg message
      const std::string messageName = Utils::xml_get_attribute_value_drill(data_manager::mapFlightLegs[data_manager::currentLegName].node, mxconst::get_ATTRIB_NAME(), flag_found_attrib, mxconst::get_ELEMENT_END_LEG_MESSAGE());
      if (!messageName.empty())
      {
        missionx::QueueMessageManager::addMessageToQueue(messageName, missionx::EMPTY_STRING, err);
      }

      // v3.303.11 Add post display objects rules - Since leg was ended, reset all "display_at_post_leg" that were flaged "true" to empty ones (which means "false" by default).
      for (auto& [key, instObj] : data_manager::map3dInstances)
      {

        if (Utils::readBoolAttrib(instObj.node, mxconst::get_ATTRIB_DISPLAY_AT_POST_LEG_B(), false))
        {
          instObj.node.updateAttribute("false", mxconst::get_ATTRIB_DISPLAY_AT_POST_LEG_B().c_str(), mxconst::get_ATTRIB_DISPLAY_AT_POST_LEG_B().c_str()); // We empty the attribute value so the instance should be displayed in he next flc_3d_objects() iteration. This also means that the flight leg is finished and we are at the post "leg" stage.

          // modify the original flight leg settings for the savepoint
          const std::string objName  = instObj.getName();         // debug
          const std::string instName = instObj.getInstanceName(); // debug
          if (Utils::isElementExists(data_manager::map3dInstances, instName))
            data_manager::map3dInstances[instName].node.updateAttribute("false", mxconst::get_ATTRIB_DISPLAY_AT_POST_LEG_B().c_str(), mxconst::get_ATTRIB_DISPLAY_AT_POST_LEG_B().c_str());

          if (Utils::isElementExists(data_manager::map3dObj, objName))
          {
            auto dNode = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(data_manager::map3dObj[objName].node, mxconst::get_ELEMENT_DISPLAY_OBJECT(), mxconst::get_ATTRIB_INSTANCE_NAME(), instName, false);
            if (!dNode.isEmpty())
              dNode.updateAttribute("false", mxconst::get_ATTRIB_DISPLAY_AT_POST_LEG_B().c_str(), mxconst::get_ATTRIB_DISPLAY_AT_POST_LEG_B().c_str());
          }
        }
      } // end loop
    }


    // check if success or failure
    switch (data_manager::mapFlightLegs[data_manager::currentLegName].getFlightLegState())
    {
      case missionx::enums::mx_flightLeg_state::leg_is_ready_for_next_flight_leg:
      {
        // search for next flightLeg
        const std::string nextFlightLeg = Utils::readAttrib(data_manager::mapFlightLegs[data_manager::currentLegName].node, mxconst::get_ATTRIB_NEXT_LEG(), ""); // v3.0.241.1
        if (nextFlightLeg.empty())
        {
          // store end stats
          missionx::data_manager::gatherFlightLegEndStatsAndStoreInVector();

          data_manager::missionState = missionx::mx_mission_state_enum::mission_completed_success;
          data_manager::currentLegName.clear();

          // debug
          Log::logDebugBO("Need to call End Mission.");

          missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::end_mission_after_all_qmm_broadcasted); // v3.0.241.7.1
          return;
        }
        else
        {
          // v3.0.219.9 adding another abort reason. If next_leg name is equal to currentGoalName
          if (!(Utils::isElementExists(missionx::data_manager::mapFlightLegs, nextFlightLeg)) || (data_manager::currentLegName.compare(nextFlightLeg) == 0))
          {
            Log::logMsgErr("There is a problem with Flight Leg: " + mxconst::get_QM() + nextFlightLeg + mxconst::get_QM() + ". Check if it exists or if it repeat itself. Check your mission data file and notify developer if need. Aborting mission.", false);
            data_manager::missionState = missionx::mx_mission_state_enum::mission_aborted;


            setUiEndMissionTexture();        // v3.0.156
            Mission::isMissionValid = false; // set special flag
            return;
          }
          else // next flight leg is legit
          {
            // store end stats
            missionx::data_manager::gatherFlightLegEndStatsAndStoreInVector();



            data_manager::currentLegName = nextFlightLeg; // next flightLeg exists

            this->flight_leg_progress_counter_i++; // progress sequence of flight legs // v3.303.12 moved below

            // v3.0.200 init all 3d objects from "display_objects" element
            Mission::initFlightLegDisplayObjects();
            // end v3.0.200 //

            Mission::uiImGuiBriefer->initFlightLegChange(); // v3.0.303.5 added missing flight_leg change initialization

            // prepare Cue points // v3.0.203
            data_manager::resetCueSettings();

            // v3.303.12 skip GPS settings for dummy leg, since it dows represent real location
            if (!data_manager::mapFlightLegs[data_manager::currentLegName].getIsDummyLeg())
            {
              // Reveal GPS next location if options is set correctly
              if (Utils::getNodeText_type_1_5<bool>(system_actions::pluginSetupOptions.node, mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE(), true) == false ||
                  Utils::readBoolAttrib(missionx::data_manager::mx_global_settings.node, mxconst::get_OPT_GPS_IMMEDIATE_EXPOSURE(), true) == false) // Check setup preference or from checkpoint global_settings preference to keep consistency when we started the mission
              {
                // Display from global GPS
                if ((!data_manager::xmlGPS.isEmpty() && data_manager::xmlGPS.nChildNode(mxconst::get_ELEMENT_POINT().c_str()) > 1 && data_manager::mapFlightLegs[data_manager::currentLegName].xGPS.isEmpty()) ||
                    (!data_manager::mapFlightLegs[data_manager::currentLegName].xGPS.isEmpty() && data_manager::mapFlightLegs[data_manager::currentLegName].xGPS.nChildNode(mxconst::get_ELEMENT_POINT().c_str()) == 0))
                {
                  this->add_GPS_data(flight_leg_progress_counter_i + 1); // if we are using the global GPS then flight_leg_progress_counter_i is one below the GPS
                }
                else
                  this->add_GPS_data();
              }
            }

            // v3.0.221.7 set the shared datarefs
            missionx::data_manager::setSharedDatarefData();
          }
        }
      } // end case leg_is_ready_for_next_flight_leg
      break;
      case missionx::enums::mx_flightLeg_state::leg_success_do_post:
      {
        // v3.305.1
        missionx::data_manager::queFlcActions.push(missionx::mx_flc_pre_command::eval_end_flight_leg_after_all_qmm_broadcasted);
      }
      break;
      default: // fail the mission
      {
        // debug
        Log::logMsg("Flight Leg: " + mxconst::get_QM() + data_manager::currentLegName + mxconst::get_QM());

        data_manager::missionState = missionx::mx_mission_state_enum::mission_completed_failure;
        setUiEndMissionTexture(); // v3.0.156

        data_manager::currentLegName.clear();
      }
      break;

    } // end switch flight leg state

  } // handle next flightLeg if current flightLeg is complete
}

// -------------------------------------

void
missionx::Mission::stopMission()
{
  data_manager::missionState = missionx::mx_mission_state_enum::mission_stopped;

  data_manager::releaseMissionInstancesAnd3DObjects(); // v3.0.200

  if (missionx::data_manager::flag_gather_acf_info_thread_is_running)
    missionx::data_manager::flag_abort_gather_acf_info_thread = true;

  XPLMDebugString("\nClearing action queues"); // debug
  missionx::data_manager::clearAllQueues();
  XPLMDebugString("\nClearing SM"); // debug
  Mission::stopDataManagerAndClearScriptManager();
  XPLMDebugString("\nStop all channels"); // debug
  // missionx::QueueMessageManager::sound.stopAllPoolChannels();
  missionx::QueueMessageManager::stopAllPoolChannels(); // v24.03.2

  missionx::QueueMessageManager::initStatic();


  XPLMDebugString("\nmissionx: Release textures\n");    // debug
  data_manager::clearMissionLoadedTextures(); // v3.0.156

  if (Mission::uiImGuiOptions)
    Mission::uiImGuiOptions->execAction(missionx::mx_window_actions::ACTION_HIDE_WINDOW); // v3.0.251.1
  if (Mission::uiImGuiMxpad)
    Mission::uiImGuiMxpad->execAction(missionx::mx_window_actions::ACTION_HIDE_WINDOW); // v3.0.251.1

}

// -------------------------------------

void
missionx::Mission::loadMission()
{
  missionx::Log::logMsg("[Mission-X] Try to load Mission.");
  // std::string err; // v25.03.1 deprecated

  // try to load a new mission only if BrieferInfo is present and there is no active mission
  if ( // const std::string key = missionx::data_manager::selectedMissionKey; !key.empty()
        !missionx::data_manager::selectedMissionKey.empty ()
        && mxUtils::isElementExists(data_manager::mapBrieferMissionList, missionx::data_manager::selectedMissionKey)
        && data_manager::missionState != missionx::mx_mission_state_enum::mission_is_running)
  {
    const std::string pathToMissionRootFolder = data_manager::mapBrieferMissionList[missionx::data_manager::selectedMissionKey].pathToMissionPackFolderInCustomScenery; // v3.0.156
    const std::string pathToMissionFile       = data_manager::mapBrieferMissionList[missionx::data_manager::selectedMissionKey].getFullMissionXmlFilePath();

    if (read_mission_file::load_mission_file ( pathToMissionFile, pathToMissionRootFolder ) )
    {
      // data_manager::missionState = missionx::mx_mission_state_enum::mission_loaded_from_the_original_file; // v25.03.1
      missionx::QueueMessageManager::sound.release(); // release Sound resources. Will initialize during "START_MISSION" phase.

      // Prepare mission filename for savepoint and dataref
      missionx::data_manager::missionSavepointFilePath = missionx::data_manager::mx_folders_properties.getNodeStringProperty(mxconst::get_FLD_MISSIONX_SAVE_PATH(), "")
        + std::string(XPLMGetDirectorySeparator())
        + Utils::readAttrib(data_manager::xMainNode, mxconst::get_ATTRIB_NAME(), "")
        + mxconst::get_MX_FILE_SAVE_EXTENTION();
      missionx::data_manager::missionSavepointDrefFilePath = missionx::data_manager::mx_folders_properties.getNodeStringProperty(mxconst::get_FLD_MISSIONX_SAVE_PATH(), "")
        + std::string(XPLMGetDirectorySeparator())
        + Utils::readAttrib(data_manager::xMainNode, mxconst::get_ATTRIB_NAME(), "")
        + mxconst::get_MX_FILE_SAVE_DREF_EXTENTION();

      if (missionx::data_manager::missionSavepointFilePath.empty () * missionx::data_manager::missionSavepointDrefFilePath.empty ())
        data_manager::missionState = missionx::mx_mission_state_enum::mission_undefined;
      else
        data_manager::missionState = missionx::mx_mission_state_enum::mission_loaded_from_the_original_file;
    }

    // Log::logMsgNone(err);

    if (data_manager::missionState == missionx::mx_mission_state_enum::mission_loaded_from_the_original_file)
    {
      Mission::uiImGuiBriefer->setMessage("Mission Loaded. You can press start...", 8);
    }

    else
    {
      Mission::uiImGuiBriefer->setMessage("FAILED to load Mission. Check Log file !!!\n" + data_manager::load_error_message, 60);
    }

  } // end if Mission information is available
  else
  {
    const std::string msg = "[load mission] selectedMissionKey is empty. Can't load a mission. Notify programmer. ";
    Log::logMsgErr(msg);

    Mission::uiImGuiBriefer->setMessage(msg);
  }
}

// -------------------------------------

void
missionx::Mission::setUiEndMissionTexture()
{
  std::string end_desc;
  end_desc.clear();

  
  if (data_manager::missionState == mx_mission_state_enum::mission_completed_success)
  {

    if (!data_manager::endMissionElement.node.isEmpty())
    {
#ifdef IBM
      const std::string file = Utils::readAttrib(data_manager::endMissionElement.node.getChildNode(mxconst::get_ELEMENT_END_SUCCESS_IMAGE().c_str()), mxconst::get_ATTRIB_FILE_NAME(), "");
#else
      IXMLNode          xNode = data_manager::endMissionElement.node.getChildNode(mxconst::get_ELEMENT_END_SUCCESS_IMAGE().c_str());
      const std::string file  = Utils::readAttrib(xNode, mxconst::get_ATTRIB_FILE_NAME(), ""); 
#endif
      if (mxUtils::isElementExists(data_manager::mapCurrentMissionTextures, file))
      {
        Mission::uiImGuiBriefer->strct_flight_leg_info.endTexture = data_manager::mapCurrentMissionTextures[file];
      }

      end_desc = Utils::xml_read_cdata_node(data_manager::endMissionElement.node.getChildNode(mxconst::get_ELEMENT_END_SUCCESS_MSG().c_str()), "");
      if (!end_desc.empty())
      {

        Mission::uiImGuiBriefer->strct_flight_leg_info.end_description = end_desc;
      }
    }
  }
  else if (data_manager::missionState == mx_mission_state_enum::mission_completed_failure || data_manager::missionState == mx_mission_state_enum::mission_aborted)
  {
    if (!data_manager::endMissionElement.node.isEmpty())
    {
#ifdef IBM
      const std::string file = Utils::readAttrib(
        data_manager::endMissionElement.node.getChildNode(mxconst::get_ELEMENT_END_FAIL_IMAGE().c_str()), mxconst::get_ATTRIB_FILE_NAME(), ""); 
#else
      IXMLNode          xNode = data_manager::endMissionElement.node.getChildNode(mxconst::get_ELEMENT_END_FAIL_IMAGE().c_str());
      const std::string file  = Utils::readAttrib(xNode, mxconst::get_ATTRIB_FILE_NAME(), ""); 
#endif

      if (mxUtils::isElementExists(data_manager::mapCurrentMissionTextures, file))
        Mission::uiImGuiBriefer->strct_flight_leg_info.endTexture = data_manager::mapCurrentMissionTextures[file];

      end_desc.clear();
      if (data_manager::missionState == mx_mission_state_enum::mission_aborted)
        end_desc = Utils::readAttrib(data_manager::mx_global_settings.node, mxconst::get_PROP_MISSION_ABORT_REASON(), "aborting mission...");

      if (end_desc.empty()) // read failure message
        end_desc = Utils::xml_read_cdata_node(data_manager::endMissionElement.node.getChildNode(mxconst::get_ELEMENT_END_FAIL_MSG().c_str()), "");

      if (!end_desc.empty())
      {
        Mission::uiImGuiBriefer->strct_flight_leg_info.end_description = end_desc;
      }
    }
  }

  Mission::uiImGuiBriefer->execAction(missionx::mx_window_actions::ACTION_SHOW_END_SUMMARY_LAYER);
}

// -------------------------------------

void
missionx::Mission::prepareUiMissionList()
{
  // load all mission briefer info
  void* inItemRef = (void*)Mission::mx_menuIdRefs::MENU_OPEN_LIST_OF_MISSIONS;
  this->MissionMenuHandler (nullptr, inItemRef);

  // load all images
  missionx::data_manager::loadAllMissionsImages(); // v3.0.251.1 replace uiWinBriefer.loadAllMissionsImages()
}
// -------------------------------------
void
missionx::Mission::stop_plugin()
{
  // abort aptdat thread if active
  this->optAptDat.stop_plugin(); // v3.0.219.9
  this->engine.stop_plugin();    // v3.0.219.12
  missionx::QueueMessageManager::stopAllPoolChannels(); // v24.03.2
  missionx::writeLogThread::stop_plugin(); // v3.305.2
}

// -------------------------------------

bool
missionx::Mission::read_and_update_xml_file_point_elements_base_on_terrain_probing(std::string inFileName)
{
  std::string errMsg;

  //// set folders path ////
  const std::string pathToRandomRootFolder = data_manager::mx_folders_properties.getAttribStringValue(mxconst::get_FLD_MISSIONS_ROOT_PATH(), "", data_manager::errStr);
  const std::string pathToRandomFolder     = pathToRandomRootFolder + mxconst::get_FOLDER_SEPARATOR() + mxconst::get_FOLDER_RANDOM_MISSION_NAME(); // +mxconst::get_FOLDER_SEPARATOR() + mxconst::FOLDER_METADATA_NAME;
  const std::string input_file             = pathToRandomFolder + mxconst::get_FOLDER_SEPARATOR() + "points.xml";
  const std::string output_file            = pathToRandomFolder + mxconst::get_FOLDER_SEPARATOR() + "points_mod.xml";


#ifndef RELEASE
  Log::logMsgNone("[DEBUG points probe] Read file: " + input_file);
  Log::logMsgNone("[DEBUG points probe] Write to file: " + output_file);
#endif

  IXMLDomParser iDomTemplate;
  ITCXMLNode    xTemplateNode = iDomTemplate.openFileHelper(input_file.c_str(), mxconst::get_TEMPLATE_ROOT_DOC().c_str(), &errMsg); // parse xml into ITCXMLNode
  ITCXMLNode    xMappingNode  = iDomTemplate.openFileHelper(input_file.c_str(), mxconst::get_MAPPING_ROOT_DOC().c_str(), &errMsg);  // parse xml into ITCXMLNode
  IXMLNode      xRootTemplate = xTemplateNode.deepCopy();                                                                     // convert ITCXMLNode to IXMLNode. IXMLNode allow to modify itself
  if (!errMsg.empty() && xRootTemplate.isEmpty())                                                                             // check if there is any failure during read
  {
    Log::logMsgErr(errMsg);
    return false;
  }

  // recursivly pick each child element and search for "point" element.
  // if found point element then read its lat/long and template. If template is not empty then continue. Else check lat/lon not 0 or empty and probe their location
  int counter = this->parseAndModifyChildPoints(xRootTemplate, 0);

  Log::logMsgNone("[processed points] Finish parsing: " + input_file + ", proccessed: " + Utils::formatNumber<int>(counter) + " <points>");


  ////////////////////////////
  // Write to file  /////////
  IXMLRenderer  xmlWriter; //
  IXMLErrorInfo errInfo = xmlWriter.writeToFile(xRootTemplate, output_file.c_str(), "ISO-8859-1");
  if (errInfo != IXMLErrorInfo::IXMLError_None)
  {
    std::string translatedError;
    translatedError.clear();

    translatedError = xmlWriter.getErrorMessage(errInfo);

    Log::logMsgErr("[random] Error code while writing: " + translatedError + " (Check folder exists: " + output_file);
    return false;
  } // end if fail to write
  std::ofstream outWriteMapping;
  outWriteMapping.open(output_file.c_str(), std::ios::app); // can we create/open the file ?
  if (outWriteMapping.fail())
  {
    Log::logAttention((std::string("Fail to create file: ") + output_file + "\n"));
  }
  else
  {
    outWriteMapping << "\n";
    outWriteMapping << xmlWriter.getString(xMappingNode);
  }
  if (outWriteMapping.is_open())
    outWriteMapping.close();

  Log::logMsgNone("[processed points] Finish writing to: " + output_file);


  return true;
}

// -------------------------------------

int
missionx::Mission::parseAndModifyChildPoints(IXMLNode& inParent, int inLevel)
{
  int counter = 0;

  if (inLevel > 10)
    return 0; // exit level

  // read all points and process them
  int nChilds = inParent.nChildNode(mxconst::get_ELEMENT_POINT().c_str());
  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    IXMLNode c1 = inParent.getChildNode(mxconst::get_ELEMENT_POINT().c_str(), i1);
    if (c1.isEmpty())
      continue;

    const std::string template_s = Utils::readAttrib(c1, mxconst::get_ATTRIB_TEMPLATE(), "");
    double            lat_d      = Utils::readNumericAttrib(c1, mxconst::get_ATTRIB_LAT(), 0.0);
    double            lon_d      = Utils::readNumericAttrib(c1, mxconst::get_ATTRIB_LONG(), 0.0);

    if (!template_s.empty())
    {
      Log::logDebugBO("[parseAndModifyChildPoints] Found point with template. skipping probe...");
      continue;
    }

    if (lat_d == 0.0 || lon_d == 0.0)
    {
      Log::logDebugBO("[parseAndModifyChildPoints] Found point with invalid lat/lon [" + Utils::formatNumber<double>(lat_d, 8) + ", " + Utils::formatNumber<double>(lon_d, 8) + "] skipping probe...");
      continue;
    }

    NavAidInfo na;
    na.lat = (float)lat_d;
    na.lon = (float)lon_d;
    na.synchToPoint();

    const float slope_f = this->engine.calc_slope_at_point_mainThread(na);
    const bool  isWet   = missionx::Point::probeIsWet(na.p, na.p.probe_result);

    if (slope_f > missionx::data_manager::Max_Slope_To_Land_On || isWet)
      c1.updateAttribute(mxconst::get_FL_TEMPLATE_VAL_HOVER().c_str(), mxconst::get_ATTRIB_TEMPLATE().c_str(), mxconst::get_ATTRIB_TEMPLATE().c_str());
    else
      c1.updateAttribute(mxconst::get_FL_TEMPLATE_VAL_LAND().c_str(), mxconst::get_ATTRIB_TEMPLATE().c_str(), mxconst::get_ATTRIB_TEMPLATE().c_str());

    ++counter;
  }
  // end processing all points at this level

  // read all childs that are not names <point> and process their sub childs
  nChilds = inParent.nChildNode();
  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    IXMLNode c1 = inParent.getChildNode(i1);
    if (c1.isEmpty())
      continue;

    if (c1.getName() == mxconst::get_ELEMENT_POINT()) // skip <point> childs
      continue;

    int subCounter = this->parseAndModifyChildPoints(c1, (inLevel + 1));
    counter += subCounter;
  }

  return counter;
}


// -------------------------------------
// -------------------------------------
// -------------------------------------
