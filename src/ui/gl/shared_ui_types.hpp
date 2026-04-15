//
// Created by saar.nagar on 4/15/2026.
//

#ifndef MISSIONX_SHARED_UI_TYPES_H
#define MISSIONX_SHARED_UI_TYPES_H

#include "../../core/xx_mission_constants.hpp"
#include "../../core/data_manager.h"

namespace missionx
{
// ---------------------- GLOBAL PARAMETERS ----------
static constexpr int DEFAULT_MESSAGE_TIME_I = 8;
static constexpr int MIN_RAD_UI_VALUE_MT = 5;
static constexpr int MAX_RAD_UI_VALUE_MT = 50000;



const std::string POPUP_FLIGHT_LEG_SETTINGS      = "Leg Detail Popup"; // v3.0.301
const std::string POPUP_BRIEFER_SETTINGS         = "Briefer Settings Popup"; // v3.0.301
const std::string POPUP_TRIGGER_SETTINGS         = "Trigger Settings Popup"; // v3.0.301
const std::string POPUP_DATAREF_SETTINGS         = "Dataref Settings Popup"; // v3.0.301
const std::string POPUP_GLOBAL_SETTINGS          = "GlobalSettings Popup"; // v3.305.1
const std::string POPUP_USER_LAT_LON             = "User Lat/Lon"; // v3.0.301
const std::string POPUP_TRIG_OUTCOME             = "Outcome Trigger Element"; // v3.0.301
const std::string POPUP_PICK_GLOBAL_SETTING_NODE = "Pick GlobalSettings to store Popup"; // v3.305.1
const std::string POPUP_ONLINE_SCRIPT_EDIT       = "Script Online Edit"; // v3.305.3
const std::string POPUP_ONLINE_GLOBALS_EDIT      = "Globals Online Edit"; // v3.305.3
const std::string POPUP_FPLN_EXTRA_DATA          = "FPLN Extra Data"; // v25.06.1
const std::string POPUP_LOAD_WARNINGS            = "LOAD Warnings"; // v26.01.1


// ---------------------- ENUMS ----------------------

enum class mx_btn_coordinate_state_enum
  : uint8_t
{
  none   = 0,
  plane  = 1,
  camera = 2
} ;



// ---------------------- STRUCTS --------------------

// v26.03.1
struct mx_header_state
{
  bool        bState{ false };
  std::string title{ "n/a" };

  mx_header_state () {};
  mx_header_state (const std::string& inVal_s, const bool inBool)
  {
    bState = inBool;
    title  = inVal_s;
  }

  void setState (const bool inState)
  {
    if (this->bState != inState)
    {
      this->bState ^= 1;

      if (bState)
        ImGui::SetScrollHereY ();
    }
  }
};


struct mx_popup_adv_settings_strct
  {
    // random date and time  // v3.303.10
    bool                                  flag_includeNightHours{ false }; // v3.303.10
    bool                                  flag_checkAnyMonth = false;
    bool                                  checkPartOfDay_b   = false;
    missionx::mx_ui_random_date_time_type iRadioRandomDateTime_pick{ missionx::mx_ui_random_date_time_type::xplane_day_and_time }; // v3.303.10
    char                                  selected_dateTime_by_user_arr[3][4] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }; // represent month selected
    const char                            selected_month_no[3][4]             = { { 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { 9, 10, 11, 12 } }; // represent month numer
    const std::string                     selected_lbl[3][4]                  = { { "Jan", "Feb", "Mar", "Apr" }, { "May", "Jun", "Jul", "Aug" }, { "Sep", "Oct", "Nov", "Dec" } }; // represent month label

    char              selectedTime[4][2]      = { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } }; // represent month selected
    const char        selected_time_no[4][2]  = { { 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 } }; // represent which cell was picked
    const std::string selected_time_lbl[4][2] = { { "Early morning 5am to 8am", "Morning 8am to 10am" }, { "Late morning 11am to 12pm", "Early afternoon 1pm to 3pm" }, { "Late afternoon 4pm to 5pm", "Early evening 5pm to 7pm" }, { "Late evening 7pm to 9pm", "Night 9pm to 4am" } }; // represent which cell was picked


    int  iClockHourPicked{ 9 }; // v3.303.8 default hour is 09:00 in the morning
    int  iClockMinutesPicked{ 0 }; // v3.303.8 default hour is xx:00 in the morning
    int  iClockDayOfYearPicked{ 0 }; // v3.303.8 default hour is xx:00 in the morning
    bool flag_firstTimeOpenBriefer{ true }; // v3.303.10

    // Weather Related settings

    bool flag_use_custom_weather_settings{ false };
    bool flag_pickAnyWeatherType{ false };
    // bool bDisableCustomWeatherWidgets{ true };

    static const int weather_y = 2, weather_x = 5; // array size

    // XP 11 add hock weather
    int               selected_weather_by_user_arr_0_1_xp11[weather_y][weather_x] = { { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 } };
    const int         selected_weather_code_xp11[weather_y][weather_x]            = { { 0, 1, 2, 3, 4 }, { 5, 6, 7, -1, -1 } }; // -1 means can not be picked
    const std::string selected_weather_lbl_xp11[weather_y][weather_x]             = { { "Clear", "Cirrus", "Scattered", "Broken", "Overcast" }, { "Low Visibility", "Foggy", "Stormy", "", "" } };
    // XP 12 add hock weather
    int               selected_weather_by_user_arr_0_1_xp12[weather_y][weather_x] = { { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 } };
    const int         selected_weather_code_xp12[weather_y][weather_x]            = { { 0, 1, 2, 3, 4 }, { 5, 6, 7, 8, -1 } }; // -1 means can not be picked
    const std::string selected_weather_lbl_xp12[weather_y][weather_x]             = { { "Clear", "VFR", "VFR Scattered", "VFR Broken", "VFR Overcast" }, { "IFR N.P", "IFR.P", "Convective", "Thunder Storms", "" } };

    // pointer to one of the weather types
    int (*ptr_selected_weather_by_user_arr)[weather_y][weather_x]       = ((missionx::data_manager::xplane_ver_i < missionx::XP12_VERSION_NO) ? &selected_weather_by_user_arr_0_1_xp11 : &selected_weather_by_user_arr_0_1_xp12);
    const int (*ptr_selected_weather_code)[weather_y][weather_x]        = ((missionx::data_manager::xplane_ver_i < missionx::XP12_VERSION_NO) ? &selected_weather_code_xp11 : &selected_weather_code_xp12);
    const std::string (*ptr_selected_weather_lbl)[weather_y][weather_x] = ((missionx::data_manager::xplane_ver_i < missionx::XP12_VERSION_NO) ? &selected_weather_lbl_xp11 : &selected_weather_lbl_xp12);

    // XP 12 add hock weather change mode: sim/weather/region/change_mode:	How the weather is changing.
    // 0 = Rapidly Improving, 1 = Improving, 2 = Gradually Improving, 3 = Static, 4 = Gradually Deteriorating, 5 = Deteriorating, 6 = Rapidly Deteriorating, 7 = Using Real Weather
    static const int  DEFAULT_WEATHER_MODE_Y = 1, DEFAULT_WEATHER_MODE_X = 0; // array size
    static const int  weather_mode_y = 2, weather_mode_x = 3; // array size
    int               selected_weather_mode_by_user_arr_0_1_xp12[weather_mode_y][weather_mode_x] = { { 0, 0, 0 }, { 1, 0, 0 } };
    const int         selected_weather_mode_code_xp12[weather_mode_y][weather_mode_x]            = { { 0, 1, 2 }, { 3, 4, 5 } };
    const std::string selected_weather_mode_lbl_xp12[weather_mode_y][weather_mode_x]             = { { "Rapidly Improving", "Improving", "Gradually Improving" }, { "Static", "Gradually Deteriorating", "Rapidly Deteriorating" } };


    std::string get_weather_picked_by_user ()
    {
      std::string propValue_s{ "" };

      for (int yy = 0; yy < weather_y; yy++)
        for (int xx = 0; xx < weather_x; xx++)
        {
          if ((*ptr_selected_weather_by_user_arr)[yy][xx] > 0) // We store only picked weather, meaning value must be greater than 0
            propValue_s += (!propValue_s.empty ()) ? "," + mxUtils::formatNumber<int> ((*ptr_selected_weather_code)[yy][xx]) : mxUtils::formatNumber<int> ((*ptr_selected_weather_code)[yy][xx]);
        }

      return propValue_s;
    }

    std::string get_weather_change_mode_picked_by_user ()
    {
      std::string propValue_s{ "" };

      for (int yy = 0; yy < weather_mode_y; yy++)
        for (int xx = 0; xx < weather_mode_x; xx++)
        {
          if (selected_weather_mode_by_user_arr_0_1_xp12[yy][xx] > 0) // We store only picked weather, meaning value must be greater than 0
            propValue_s += (!propValue_s.empty ()) ? "," + mxUtils::formatNumber<int> (selected_weather_mode_code_xp12[yy][xx]) : mxUtils::formatNumber<int> (selected_weather_mode_code_xp12[yy][xx]);
        }

      return propValue_s;
    }

    const char *windSpeeds_arr[31] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30" }; // v3.303.12
    int         windSpeeds_user_picked_i{ 0 };
    int         iWindMin         = 0;
    int         iWindMax         = 0;
    std::string windSpeedMin_lbl = "Min Speed: " + mxUtils::formatNumber<int> (iWindMin);
    std::string windSpeedMax_lbl = "Max Speed: " + mxUtils::formatNumber<int> (iWindMax);

    missionx::mx_ui_random_weather_options iWeatherType_user_picked{ missionx::mx_ui_random_weather_options::use_xplane_weather }; // v3.303.12

    int         cloudElevInputField_i{ 500 };
    int         iCloudMin        = 0;
    int         iCloudMax        = 0;
    std::string cloudElevMin_lbl = "Min Elev: " + mxUtils::formatNumber<int> (iCloudMin);
    std::string cloudElevMax_lbl = "Max Elev: " + mxUtils::formatNumber<int> (iCloudMax);


    // Weight
    bool flag_add_default_weight_settings{ true }; // v3.303.14
    int  pilot_base_weight_i{ 85 }; // v25.02.1
    int  passengers_base_weight_i{ 70 }; // v25.02.1
    int  cargo_base_weight{ 10 }; // v25.02.1


  };

  // ---------------------------------
  // -- Setup Layer Struct
  // ---------------------------------
struct mx_setup_layer
  {
    static const int OSM_BUFF_SIZE_I = 499;
    bool             bDisplayTargetMarkers{ true };
    bool             bOverrideExpectedTargetDistance{ false };
    bool             bPauseIn2D{ false };
    bool             bPauseInVR{ false }; // v25.06.1 should always be false
    bool             bCycleLogFiles{ false };
    bool             bAddCountdown{ false };
    bool             bGPSImmediateExposure{ false };
    bool             bForceNormalizedVolume{ false }; // v3.0.303.6
    bool             bSuppressDistanceMessages{ false }; // v25.02.1

    int iNormalizedVolume_val{ mxconst::DEFAULT_SETUP_MISSION_VOLUME_I }; // v3.0.303.6
    int iMinDistanceSlider{ static_cast<int> (mxconst::SLIDER_MIN_RND_DIST) }; // init with 5
    #ifdef LIN
    int                            iLinuxFlavor_val{0}; // v3.303.8.1 - will hold the linux flavor to deal with
    const std::vector<const char*> vecLinuxComboCodes_s{"Debian / Ubuntu based distros like Mint, Pop!OS and the likes", "Arch based distros like Manjaro, Garuda, Endeavour and the likes", "other - not in the list"};
    #endif

    // int   iPreferredFontSize{ 0 }; // v3.303.14

    float fPreferredFontPixelSize{ mxconst::FONT_PIXEL_13 };
    float fFontMaxPixelSize{ mxconst::DEFAULT_MAX_FONT_PIXEL_SIZE };
    float fFontMinPixelSize{ mxconst::DEFAULT_MIN_FONT_PIXEL_SIZE };

    float fPreferredFontScale{ 1.0f };
    float fFontMaxScaleSize{ mxconst::DEFAULT_MAX_FONT_SCALE }; // 1.4f as of this writing
    float fFontMinScaleSize{ mxconst::DEFAULT_MIN_FONT_SCALE }; // 0.8f as of this writing


    bool bPlaceMarkersAwayFromTarget{ false };
    bool bOverideCustomExternalFPLN_folders{ false };
    // bool bWriteCacheDataIntoDB{ false }; // v3.303.14 deprecated - always on

    bool                      is_first_time{ true };
    char                      overpass_url_buf[OSM_BUFF_SIZE_I + 1]{ "" };
    std::vector<const char *> vecOverpassUrls_char;
    bool                      flag_lock_overpass_url{ false }; // lock user picked url so when fetching from overpass we won't cycle

    const char *font_list[2] = { "default - DejaVuSans", "Internal" };
    int         user_font_picked_i{ 0 };
    std::string user_preferred_font_path_s;

    char default_scoring_buf[4096]{ "" };
    char buf_pilotName[24]{ '\0' };
    char buf_simbrief_pilot_id[10]{ '\0' }; // v25.03.3
    bool flag_load_extra_data_from_simbrief_to_notes {false}; // v26.04.1

    bool            bPressedPlane;
    bool            bPressedCamera;
    missionx::Point coord;

    mx_btn_coordinate_state_enum btn_coord_state = { mx_btn_coordinate_state_enum::none };


    int setPilotName (const std::string &inPilotName) { return snprintf (buf_pilotName, sizeof (buf_pilotName) - 1, "%s", inPilotName.c_str ()); }

    int setSimbriefPilotID (const std::string &inPilotID) { return snprintf (buf_simbrief_pilot_id, sizeof (buf_simbrief_pilot_id) - 1, "%s", inPilotID.c_str ()); }


    // v3.305.3 collapse headers in a better controlled way on the scroll location. See implementation in draw_setup_layer().
    int                                      headerIndex{ 0 }; // v25.03.3. Used only with mapSetupHeaders
    std::unordered_map<int, mx_header_state> mapSetupHeaders = { { headerIndex, mx_header_state ("General Settings", false) }, { ++headerIndex, mx_header_state ("Simbrief & flightplandatabase.com setup", false) }, { ++headerIndex, mx_header_state ("APT data optimization", false) }, { ++headerIndex, mx_header_state ("TOOLS", false) }, { ++headerIndex, mx_header_state ("Normalize Mission Sound Volume", false) }, { ++headerIndex, mx_header_state ("OVERPASS Setup", false) }, { ++headerIndex, mx_header_state ("Medevac Setup", false) }, { ++headerIndex, mx_header_state ("External Flight Plan Setup", false) }, { ++headerIndex, mx_header_state ("Default Scoring", false) }, { ++headerIndex, mx_header_state ("Linux: Troubleshoot", false) }, { ++headerIndex, mx_header_state ("Designer: Unsaved Options", false) } };

  } ;




}


#endif //MISSIONX_SHARED_UI_TYPES_H
