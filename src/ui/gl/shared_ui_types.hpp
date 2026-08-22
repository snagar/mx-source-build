//
// Created by saar.nagar on 4/15/2026.
//
// We must use the "inline" for shared static variables/structs in this file, so it will be correctly shared between all classes.
// So instead of "static" use "inline", or each class will create their own private memory for each "static" variable.
//

#ifndef MISSIONX_SHARED_UI_TYPES_H
#define MISSIONX_SHARED_UI_TYPES_H

#include "../../core/xx_mission_constants.hpp"
#include "../../core/data_manager.h"
#include "../../core/dataref_manager.h"
#include "../../../libs/imgui4xp/imgui/imgui_internal.h"
//#include "../../../libs/imgui4xp/fa-solid-900.inc" // FontAwesome5
// #include "../../../libs/imgui4xp/IconsFontAwesome5.h"

namespace missionx
{
// ---------------------- GLOBAL PARAMETERS ----------
inline constexpr int DEFAULT_MESSAGE_TIME_I = 8;
inline constexpr int MIN_RAD_UI_VALUE_MT = 5;
inline constexpr int MAX_RAD_UI_VALUE_MT = 50000;


inline const std::string LBL_START_MISSION              = ">> Start Mission <<";
inline const std::string LBL_LOAD_WARNINGS              = "!! Show Warnings !!"; // v26.1.1
inline const std::string LBL_ABORT_THREAD_LABEL         = "!! Abort !!";
inline const std::string FPLN_MORE_DETAILS              = "More Flight Plan Details";
inline const std::string GENERATE_QUESTION              = "Generate Mission From Flight Plan";
inline const std::string GENERATE_ILS_QUESTION          = "Generate Mission From ILS data"; // v3.0.253.6
inline const std::string GENERATE_TEMPLATE_QUESTION     = "Generate Mission From Template data"; // v25.06.1
inline const std::string POPUP_FLIGHT_LEG_SETTINGS      = "Leg Detail Popup"; // v3.0.301
inline const std::string POPUP_BRIEFER_SETTINGS         = "Briefer Settings Popup"; // v3.0.301
inline const std::string POPUP_TRIGGER_SETTINGS         = "Trigger Settings Popup"; // v3.0.301
inline const std::string POPUP_DATAREF_SETTINGS         = "Dataref Settings Popup"; // v3.0.301
inline const std::string POPUP_GLOBAL_SETTINGS          = "GlobalSettings Popup"; // v3.305.1
inline const std::string POPUP_USER_LAT_LON             = "User Lat/Lon"; // v3.0.301
inline const std::string POPUP_TRIG_OUTCOME             = "Outcome Trigger Element"; // v3.0.301
inline const std::string POPUP_PICK_GLOBAL_SETTING_NODE = "Pick GlobalSettings to store Popup"; // v3.305.1
inline const std::string POPUP_ONLINE_SCRIPT_EDIT       = "Script Online Edit"; // v3.305.3
inline const std::string POPUP_ONLINE_GLOBALS_EDIT      = "Globals Online Edit"; // v3.305.3
inline const std::string POPUP_FPLN_EXTRA_DATA          = "FPLN Extra Data"; // v25.06.1
inline const std::string POPUP_LOAD_WARNINGS            = "LOAD Warnings"; // v26.01.1


  inline const char *clockHours_arr[24]   = { "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23" }; // v3.303.8
  inline const char *clockMinutes_arr[12] = { "00", "05", "10", "15", "20", "25", "30", "35", "40", "45", "50", "55" }; // v3.303.8
  inline const char *clockDayOfYear_arr[365]{ "0 - Jan", "1",   "2",   "3",   "4",   "5",   "6",   "7",   "8",   "9",   "10",  "11",  "12",  "13",  "14",  "15",  "16",  "17",  "18",  "19",  "20",  "21",  "22",  "23",  "24",  "25",  "26",  "27",  "28",  "29",        "30",  "31 - Feb", "32",  "33",  "34",  "35",  "36",  "37",  "38",  "39",  "40",  "41",  "42",  "43",  "44",  "45",  "46",  "47",  "48",  "49",  "50",  "51",  "52",  "53",  "54",  "55",  "56",  "57",  "58",  "59 - Mar", "60",        "61",  "62",  "63",  "64",  "65",  "66",  "67",  "68",  "69",  "70",  "71",  "72",  "73",  "74",  "75",  "76",  "77",  "78",  "79",  "80",  "81",  "82",  "83",  "84",  "85",  "86",  "87",  "88",  "89",  "90 - Apr",  "91",  "92",  "93",  "94",  "95",  "96",  "97",  "98",  "99",  "100", "101", "102", "103", "104", "105", "106", "107", "108", "109", "110", "111", "112", "113", "114", "115", "116", "117", "118", "119", "120 - May", "121",       "122", "123", "124", "125", "126", "127", "128", "129", "130", "131", "132", "133", "134", "135", "136", "137", "138", "139", "140", "141", "142", "143", "144", "145", "146", "147", "148", "149", "150", "151 - Jun", "152", "153", "154", "155", "156", "157", "158", "159", "160", "161", "162", "163", "164", "165", "166", "167", "168", "169", "170", "171", "172", "173", "174", "175", "176", "177", "178", "179", "180", "181 - Jul", "182"
                                              , "183", "184", "185", "186", "187", "188", "189", "190", "191", "192", "193", "194", "195", "196", "197", "198", "199", "200", "201", "202", "203", "204", "205", "206", "207", "208", "209", "210", "211", "212 - Aug", "213", "214",      "215", "216", "217", "218", "219", "220", "221", "222", "223", "224", "225", "226", "227", "228", "229", "230", "231", "232", "233", "234", "235", "236", "237", "238", "239", "240", "241", "242",      "243 - Sep", "244", "245", "246", "247", "248", "249", "250", "251", "252", "253", "254", "255", "256", "257", "258", "259", "260", "261", "262", "263", "264", "265", "266", "267", "268", "269", "270", "271", "272", "273 - Oct", "274", "275", "276", "277", "278", "279", "280", "281", "282", "283", "284", "285", "286", "287", "288", "289", "290", "291", "292", "293", "294", "295", "296", "297", "298", "299", "300", "301", "302", "303",       "304 - Nov", "305", "306", "30",  "308", "309", "310", "311", "312", "313", "314", "315", "316", "317", "318", "319", "320", "321", "322", "323", "324", "325", "326", "327", "328", "329", "330", "331", "332", "333", "334 - Dec", "335", "336", "337", "338", "339", "340", "341", "342", "343", "344", "345", "346", "347", "348", "349", "350", "351", "352", "353", "354", "355", "356", "357", "358", "359", "360", "361", "362", "363", "364" };

  inline static std::map<mx_ils_type_enum, std::string> mapILS_types = {
    { mx_ils_type_enum::LOC, "LOC" },
    { mx_ils_type_enum::ILS_cat_I, "ILS-cat-I" },
    { mx_ils_type_enum::ILS_cat_II, "ILS-cat-II" },
    { mx_ils_type_enum::ILS_cat_III, "ILS-cat-III" },
    { mx_ils_type_enum::IGS, "IGS" },
    { mx_ils_type_enum::LDA, "LDA" },
    { mx_ils_type_enum::SDF, "SDF" },
    { mx_ils_type_enum::GLS, "GLS" }, // v3.305.4
    { mx_ils_type_enum::LP, "LP" }, // v3.305.4
    { mx_ils_type_enum::LPV, "LPV" } // v3.305.4
  };

// ---------------------- ENUMS ----------------------

enum class mx_btn_coordinate_state_enum
  : uint8_t
{
  none   = 0,
  plane  = 1,
  camera = 2
} ;

// v26.04.1
enum class mx_act_phase_enum : uint8_t
{
  phase_pick = 0,
  phase_accept
};

// ---------------------- STRUCTS --------------------




// v3.0.253.11
struct mx_cross_layer_property
{
  bool flag_start_from_plane_position{false};
  bool flag_generate_gps_waypoints{true};
  bool flag_add_route_waypoints{false}; // v25.04.2
  bool flag_auto_load_route_to_gps_or_fms{false}; // v25.04.2

} ;


struct radio_plane_type
{
  missionx::mx_plane_types_enum type {missionx::mx_plane_types_enum::plane_type_any};
  std::string              label;

  float from_slider_min{ (float)mxconst::SLIDER_MIN_RND_DIST };
  float to_slider_min{ (float)mxconst::SLIDER_MAX_RND_DIST };
  float from_slider_max{ (float)(mxconst::SLIDER_MIN_RND_DIST * 1.2) };
  float to_slider_max{ (float)(mxconst::SLIDER_MAX_RND_DIST * 1.2) };

  radio_plane_type () = default;

  radio_plane_type (const missionx::mx_plane_types_enum inType, const std::string& inLabel, const float in_lower_slider_min, const float in_upper_slider_min, const float inMultiplyLowerMin = 1.2f, const float inMultiplyUpperMin = 1.2f)
  {
    type            = inType;
    label           = inLabel;
    from_slider_min = in_lower_slider_min;
    to_slider_min   = in_upper_slider_min;
    from_slider_max = static_cast<float>(static_cast<int>(from_slider_min * inMultiplyLowerMin));
    to_slider_max   = static_cast<float>(static_cast<int>(to_slider_min * inMultiplyUpperMin));
  }

} ;



// v26.03.1
struct mx_header_state
{
  bool        bState{ false };
  std::string title{ "n/a" };

  mx_header_state () = default;
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
      std::string propValue_s;

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
      std::string propValue_s;

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
    static constexpr int OSM_BUFF_SIZE_I = 499;
    bool                 bDisplayTargetMarkers{ true };
    bool                 bOverrideExpectedTargetDistance{ false };
    bool                 bPauseIn2D{ false };
    bool                 bPauseInVR{ false }; // v25.06.1 should always be false
    bool                 bCycleLogFiles{ false };
    bool                 bAddCountdown{ false };
    bool                 bGPSImmediateExposure{ false };
    bool                 bForceNormalizedVolume{ false }; // v3.0.303.6
    bool                 bSuppressDistanceMessages{false}; // v25.02.1
    bool                 bDisableInventoryImageLoad{false}; // v26.04.4

    int iNormalizedVolume_val{ mxconst::DEFAULT_SETUP_MISSION_VOLUME_I }; // v3.0.303.6
    int iMinDistanceSlider{ static_cast<int> (mxconst::SLIDER_MIN_RND_DIST) }; // init with 5
    #ifdef LIN
    int                            iLinuxFlavor_val{0}; // v3.303.8.1 - will hold the linux flavor to deal with
    const std::vector<const char*> vecLinuxComboCodes_s{"Debian / Ubuntu based distros like Mint, Pop!OS and the likes", "Arch based distros like Manjaro, Garuda, Endeavour and the likes", "other - not in the list"};
    #endif

    float fPreferredFontPixelSize{ mxconst::FONT_PIXEL_13 };
    float fFontMaxPixelSize{ mxconst::DEFAULT_MAX_FONT_PIXEL_SIZE };
    float fFontMinPixelSize{ mxconst::DEFAULT_MIN_FONT_PIXEL_SIZE };

    float fPreferredFontScale{ 1.0f };
    float fFontMaxScaleSize{ mxconst::DEFAULT_MAX_FONT_SCALE }; // 1.4f as of this writing
    float fFontMinScaleSize{ mxconst::DEFAULT_MIN_FONT_SCALE }; // 0.8f as of this writing


    bool bPlaceMarkersAwayFromTarget{ false };
    bool bOverideCustomExternalFPLN_folders{ false };

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

    bool            bPressedPlane{false};
    bool            bPressedCamera{false};
    missionx::Point coord;

    mx_btn_coordinate_state_enum btn_coord_state = { mx_btn_coordinate_state_enum::none };


    int setPilotName (const std::string &inPilotName) { return snprintf (buf_pilotName, sizeof (buf_pilotName) - 1, "%s", inPilotName.c_str ()); }

    int setSimbriefPilotID (const std::string &inPilotID) { return snprintf (buf_simbrief_pilot_id, sizeof (buf_simbrief_pilot_id) - 1, "%s", inPilotID.c_str ()); }


    // v3.305.3 collapse headers in a better controlled way on the scroll location. See implementation in draw_setup_layer().
    int                                      headerIndex{ 0 }; // v25.03.3. Used only with mapSetupHeaders
    std::unordered_map<int, mx_header_state> mapSetupHeaders = {
                          { headerIndex, mx_header_state ("General Settings", false) }
                        , { ++headerIndex, mx_header_state ("Simbrief, flightplandatabase.com & LLM setup", false) }
                        , { ++headerIndex, mx_header_state ("APT data optimization", false) }
                        , { ++headerIndex, mx_header_state ("TOOLS", false) }
                        , { ++headerIndex, mx_header_state ("Normalize Mission Sound Volume", false) }
                        , { ++headerIndex, mx_header_state ("OVERPASS Setup", false) }
                        , { ++headerIndex, mx_header_state ("Medevac Setup", false) }
                        , { ++headerIndex, mx_header_state ("External Flight Plan Setup", false) }
                        , { ++headerIndex, mx_header_state ("Default Scoring", false) }
                        , { ++headerIndex, mx_header_state ("Troubleshoot", false) }
                        , { ++headerIndex, mx_header_state ("Designer: Unsaved Options", false) }
    };

  bool         b_use_ai{false}; // v26.08.1
  std::string  ai_url; // v26.08.1
  std::string  ai_auth_key; // v26.08.1
  char         buf_ai_url[512]{ "" }; // v26.08.1
  char         buf_ai_auth_key[512]{ "" }; // v26.08.1
  // Array of string options to display in the dropdown
  constexpr static std::array<const char*, 2> llm_server_timeout_items = { "60", "120" };
  int llm_timeout_selected_index {0};

  int set_ai_url_buf (const std::string &in_value) { return snprintf (buf_ai_url, sizeof (buf_ai_url) - 1, "%s", in_value.c_str ()); }
  int set_ai_auth_key_buf (const std::string &in_value) { return snprintf (buf_ai_auth_key, sizeof (buf_ai_auth_key) - 1, "%s", in_value.c_str ()); }

  }; // end setup struct


  // ----- ILS Layer -----
  //// user mission creation variables
  struct mx_ils_layer
  {
    bool flagNavigatedFromOtherLayer{ false }; // v24025
    bool flagForceNavDataTab{ false }; // v24.03.1
    bool flagIgnoreDistanceFilter{ false }; // v24.03.1

    bool isIFR {true};
    bool isVFR {false};


    mx_layer_state_enum               layer_state{ missionx::mx_layer_state_enum::not_initialized };
    missionx::enums::mx_treeNodeState enum_elevSliderOpenState{ missionx::enums::mx_treeNodeState::closed }; // v24.03.1 converted the flag to enum

    int sort_indx{ 0 }; // starts in 0

    // from ILS icao parameters
    char                 buf1[10] = { "" };
    char                 buf2[10] = { "" };
    bool                 bFirstTime{ false };
    std::string          from_icao; // we will fetch the closest icao location to plane location
    std::string          to_icao; // v3.0.253.3 search specific destination
    missionx::NavAidInfo navaid;

    int            iRadioTemplate_Med_or_Cargo{ 1 }; // template type should only be cargo
    mx_plane_types_enum iRadioPlaneType{ missionx::mx_plane_types_enum::plane_type_props };
    // sliders
    float       ils_or_vfr_min_slider_value = mxconst::SLIDER_ILS_MIN_SEARCH_RADIUS; // v26.04.4 dynamic min distance
    float       ils_or_vfr_max_slider_value = mxconst::SLIDER_ILS_MAX_SEARCH_RADIUS; // v26.04.4 dynamic min distance
    float       ils_sliderVal1              = mxconst::SLIDER_ILS_MIN_SEARCH_RADIUS; // min distance
    float       ils_sliderVal2              = mxconst::SLIDER_ILS_MIN_SEARCH_RADIUS * 1.2f; // lowest ILS search radius. 250nm in v3.0.253.6
    std::string ils_slider2_lbl             = "[" + Utils::formatNumber<float> (ils_sliderVal1, 0) + ".." + Utils::formatNumber<float> (ils_sliderVal2, 0) + "]";

    // ILS types
    std::string                                ils_types_tree_label; // empty means "any"
    std::map<missionx::mx_ils_type_enum, bool> mapCheck_ILS_types;

    // RW Length Slider
    int         slider_min_rw_length_i{ mxconst::SLIDER_ILS_STARTING_RW_LENGTH_VALUE };
    std::string min_rw_length_label_s{ Utils::formatNumber<int> (slider_min_rw_length_i) };

    // Minimal RW Width Slider
    int slider_min_rw_width_i{ mxconst::SLIDER_ILS_STARTING_RW_WIDTH_VALUE };

    // Minimal Airport elevation Slider
    int slider_min_airport_elev_ft_i{ mxconst::SLIDER_ILS_STARTING_AIRPORT_ELEV_VALUE_FT };

    int                          limit_indx{ 0 }; // v24.03.1 limit ILS rows fetched from DB
    constexpr const static char *limit_items[] = { "250", "500", "750", "1000", "1250", "1500", "2000" };


    // search ILS from database - progress
    std::string filter_query_s; // v24.03.1
    missionx::mxFetchState_enum fetch_ils_state{ missionx::mxFetchState_enum::fetch_not_started };
    missionx::mxFetchState_enum fetch_nav_state{ missionx::mxFetchState_enum::fetch_not_started };
    missionx::mxFetchState_enum fetch_metar_state{ missionx::mxFetchState_enum::fetch_not_started };

    std::string asyncFetchMsg_s;
    std::string asyncNavFetchMsg_s;
    std::string asyncMetarFetchMsg_s;

    // Plane location
    missionx::Point planePos;

    // Nav Data // v24025
    std::string                                sNavICAO;
    std::unordered_map<int, mx_nav_data_strct> mapNavaidData; // airport data

    // ---------------- Members -----------------
    mx_ils_layer () = default;
    // {
    //   init_mapChecks_ils_types ();
    // }

    void init_mapChecks_ils_types ()
    {
      mapCheck_ILS_types.clear ();
      for (const auto &keyType : missionx::mapILS_types | std::views::keys)
        mapCheck_ILS_types[keyType] = false; // reset to false
    }

    std::string get_ils_types_picked () const
    // will help to construct the types tree title and the SQL statement
    {
      std::string result_s;
      bool        foundAtLeastOne = false;
      auto        counter         = 0; // (imapSize > 0) ? 1 : imapSize; // counter follows which iterator we test. If we reached last one, we won't add "," to result_s
      auto        foundCounter_i  = 0;
      for (auto &v : mapCheck_ILS_types)
      {
        counter++;
        if (v.second)
        {
          foundCounter_i++;
          if (foundAtLeastOne) // add "," only if we have at least 1 value
            result_s += ",";
          // add label
          result_s += "'" + missionx::mapILS_types[v.first] + "'";

          foundAtLeastOne = true;
        }
      }

      if (foundCounter_i == counter)
        result_s.clear ();

      return mxUtils::stringToLower (result_s);
    } // get_ils_types_picked

  };



  // ----- Custom Template Layer -----
  //typedef struct _generate_template_layer
  struct mx_generate_template_layer
  {
    bool bFinished_loading_templates{false};

    mx_layer_state_enum layer_state{missionx::mx_layer_state_enum::not_initialized}; // v3.0.253.9

    int    user_pick_from_replaceOptions_combo_i{mxconst::INT_UNDEFINED};
    ImVec2 vec2_replace_options_size{150.f, 20.0f}; // v3.0.255.4.1

    std::vector<const char*> vecReplaceOptions_char{}; // v3.0.255.4 will store pointers to the <replace_options> element from the TemplateInfo

    std::string last_picked_template_key;
    std::string selectedTemplateKey; // v25.09.2
  } ;


  // Activity button struct, for the "semi-automation" creation mission
  struct activity_btn_info_strct
  {
    // list parameters (needed for the semi-automation activity screen)
    struct st_distance
    {
      float min {5.0f};
      float lowest_max{20.0f};
      float max {50.0f};
    };
    struct st_targets
    {
      int min {1};
      int max {1};
    };

    int          id{ -1 };
    int          final_legs_no_to_generate {1}; // Will hold the final number of legs to generate.

    st_distance  distance_min_max {.min = 5.0f, .lowest_max = 20.0f, .max = 50.0f};
    st_targets   legs_min_max {.min = 1, .max = 4}; // store the number of targets per "action type picked". Example: Medevac will only have 2 legs.

    missionx::enums::mx_semi_activities_enum activity{missionx::enums::mx_semi_activities_enum::act_none};
    missionx::mx_plane_types_enum            plane_type{missionx::mx_plane_types_enum::plane_type_any};

    std::string  imgName;
    std::string  label;
    std::string  tip;

    // Non-List parameters
    std::string desc {""}; // we need initializer, so ignore the CLion suggestion
    std::string random_description{""}; // will hold a short description based on plane type. Might hold a funny description to make it more enjoyable for the user.
    float max_distance_slider_f {0.0f};

    void reset()
    {
      id                        = -1;
      final_legs_no_to_generate = 1;
      distance_min_max          = {5.0, 20.0f, 50.0};
      legs_min_max              = {1, 4};
      activity                  = missionx::enums::mx_semi_activities_enum::act_none;
      plane_type                = missionx::mx_plane_types_enum::plane_type_any;
      imgName.clear();
      label.clear();
      tip.clear();

      // temporary data
      desc.clear();
      random_description.clear();
      max_distance_slider_f = 0.0f;
    }


    [[nodiscard]] st_distance get_mission_area() const
    {
      st_distance mission_area;

      if (this->activity < missionx::enums::mx_semi_activities_enum::act_turboprops && (this->max_distance_slider_f - this->distance_min_max.min < 10.0f))
        mission_area.min = distance_min_max.min;
      else
        mission_area.min = max_distance_slider_f - ( 0.5f * (max_distance_slider_f - distance_min_max.min ) );

      mission_area.max = max_distance_slider_f;
      mission_area.lowest_max = distance_min_max.lowest_max;

      return mission_area;
    }

    // void randomize_max_distance()
    // {
    //   max_distance_slider_f = std::roundf(  static_cast<float>( Utils::getRandomRealNumber(distance_min_max.lowest_max, distance_min_max.max) ) );
    // }
    void randomize_max_distance(const float &in_min, const float &in_max)
    {
      max_distance_slider_f = std::roundf(  static_cast<float>( Utils::getRandomRealNumber(in_min, in_max) ) );
    }

    int randomize_no_of_legs()
    {
       final_legs_no_to_generate = legs_min_max.min;

      if (legs_min_max.min < legs_min_max.max)
      {
        // randomize pick num of legs
        final_legs_no_to_generate = Utils::getRandomIntNumber(legs_min_max.min, legs_min_max.max);
      }

      return final_legs_no_to_generate;

    } // end randomize_no_of_legs

    // prepare desc
    void prepare_the_semi_activity_description(const int& in_number_of_legs)
    {
      // Type of plane
      const std::string plane_type_desc = (id < 5)? "You will fly a helos mission" : "You will fly a plane mission";

      // Flight area description
       const auto  [min_area, low_max, max_area] = get_mission_area();
      //const auto min_area = calc_min_area_distance();


      std::string flight_area_desc = fmt::format("{:.0f} to {:.0f}", min_area, max_area);
      if (activity == enums::mx_semi_activities_enum::act_helos_cargo_oilrig || activity == enums::mx_semi_activities_enum::act_helos_medevac_oilrig)
        flight_area_desc            = fmt::format("determined based on the oil rig search area");
      else if (activity == enums::mx_semi_activities_enum::act_helos_medevac_surprise_me)
        flight_area_desc            = fmt::format("determined by the plugin");

      const std::string area_desc  = fmt::format("Flight area: {}", flight_area_desc);


      // no. of legs
      // const std::string no_of_legs_desc = fmt::format("You will have: {}", (final_legs_no_to_generate < 2)? " one landing location" : fmt::format(" up to {}, landing locations", final_legs_no_to_generate));
      const std::string no_of_legs_desc = fmt::format("You will have: {}", (in_number_of_legs < 2)? " one landing location" : fmt::format(" up to {}, landing locations", final_legs_no_to_generate));

      // construct description
      desc = fmt::format("{}.\n\n{}.\n{}.", plane_type_desc, area_desc, no_of_legs_desc);
    }
  };


// -------------------------------------------
  // -- STRUCT user mission creation variables
  // -------------------------------------------
  struct mx_user_create_mission_layer
  {
    enum class mx_dynamic_fpln_screen
  : uint8_t
    {
      ext_user_creation_home = 0,
      ext_option_a,
      ext_option_b
    };

    bool flag_first_time{ true };
    mx_dynamic_fpln_screen child_screen{ mx_dynamic_fpln_screen::ext_user_creation_home };

    mx_layer_state_enum layer_state{ missionx::mx_layer_state_enum::not_initialized }; // v3.0.253.9

    // v26.04.1 semi-automated mission creation
    mx_act_phase_enum act_phase_enum{ mx_act_phase_enum::phase_pick }; // v26.04.1
    activity_btn_info_strct user_semi_act_picked; // v26.04.1

    int                                      headerIndex{ 0 }; // v25.03.3. Used only with mapSetupHeaders
    std::unordered_map<int, mx_header_state> mapSemiOptionsHeaders = { { headerIndex, mx_header_state ("Custom Tweaks", true) } }; // header is open by default


    int            iRadioMissionTypePicked{ static_cast<int> (missionx::mx_ui_mission_type::medevac) }; // which type of template user picked ?
    int            iMissionSubCategoryPicked{ -1 }; // v3.303.14 // v25.06.1 init -1 which is not valid
    mx_plane_types_enum iRadioPlaneType{ missionx::mx_plane_types_enum::plane_type_helos };
    bool           flag_narrow_helos_filtering{ false };



    // sliders
    float dyn_sliderVal1 = (float)mxconst::SLIDER_MIN_RND_DIST; // min distance
    float dyn_sliderVal2 = dyn_sliderVal1 * 1.2f; // max distance

    std::string dyn_slider1_lbl = "Min distance [" + Utils::formatNumber<float> (mxconst::SLIDER_MIN_RND_DIST, 0) + "..." + Utils::formatNumber<float> (mxconst::SLIDER_MIN_RND_DIST * 1.2, 0) + "]";
    std::string dyn_slider2_lbl = "Max distance [" + Utils::formatNumber<float> (mxconst::SLIDER_MIN_RND_DIST * 1.2, 0) + "..." + Utils::formatNumber<float> (mxconst::SLIDER_MIN_RND_DIST * 5.0, 0) + "]";

    // OSM checkbox
    bool flag_use_osm{ false };
    bool flag_use_web_osm{ false };
    bool flag_cross_country = { false };

    // flight legs
    int iNumberOfFlighLegs{ 2 };

    // overpass filter // v3.0.253.4
    const std::string overpass_original_filter{ mxconst::get_DEFAULT_OVERPASS_WAYS_FILTER () }; // { "[highway=primary][highway=secondary][highway=tertiary][highway=residential][highway=service][highway=living_street][highway=track]" };
    std::string       overpass_main_filter{ overpass_original_filter };
    std::string       overpass_pre_apply_filter_s{ overpass_main_filter };

    // Oilrig
    //std::string oilrig_part_of_globe_label; // v25.08.1
    std::map <int, std::string> map_pick_oilrig_globe_part {
       { missionx::PICKED_GLOBE, " Globe " }
      ,{ missionx::PICKED_HALF_GLOBE, " Half Globe " }
      ,{ missionx::PICKED_QUARTER_GLOBE, " Quarter Globe " }
      ,{ missionx::PICKED_LOCAL_REGION_GLOBE, " Local Region " }
      ,{ missionx::PICKED_IN_MY_AREA, " In My Area " }
    };
    // Filter runway by type
    bool                                     flag_pick_any_rw{ true };
    bool                                     flag_plane_is_amphibian{ false };
    std::map<const std::string, bool>        map_filter_runways                      = { { "Grass##filterRunways", false }, { "Dirt/Gravel##filterRunways", false }, { "Concrete/Asphalt##filterRunways", false }, { "water##filterRunways", false } };
    std::map<const std::string, std::string> map_filter_runways_translate_to_numbers = { { "Grass##filterRunways", "3" }, { "Dirt/Gravel##filterRunways", "4, 5" }, { "Concrete/Asphalt##filterRunways", "1, 2" }, { "water##filterRunways", "13" } };

    void reset_filter_runways_flags ()
    {
      for (auto &rw_type : map_filter_runways)
        rw_type.second = false;

      flag_pick_any_rw = true;
    }

    // Struct Constructor
    mx_user_create_mission_layer() = default;
  }; // end mx_user_create_mission_layer struct





  // ---------------------- CONTAINERS ----------------------
  inline std::map<missionx::mx_plane_types_enum, radio_plane_type> mapListPlaneRadioLabel = {
    { mx_plane_types_enum::plane_type_helos, radio_plane_type (mx_plane_types_enum::plane_type_helos, "Helos", (float)mxconst::SLIDER_MIN_RND_DIST, static_cast<float>(static_cast<int>(mxconst::SLIDER_MAX_RND_DIST * 0.2)), 2.4f, 2.0f) },
    { mx_plane_types_enum::plane_type_props, radio_plane_type (mx_plane_types_enum::plane_type_props, "GA (props)", (float)mxconst::SLIDER_MIN_RND_DIST, (float)(mxconst::SLIDER_MAX_RND_DIST), 10.5f, 4.0f) },
    { mx_plane_types_enum::plane_type_ga_floats, radio_plane_type (mx_plane_types_enum::plane_type_ga_floats, "Floats", (float)mxconst::SLIDER_MIN_RND_DIST, (float)(mxconst::SLIDER_MAX_RND_DIST), 10.5f, 4.0f) },
    { mx_plane_types_enum::plane_type_turboprops, radio_plane_type (mx_plane_types_enum::plane_type_turboprops, "Turbo Props", 15.0f, static_cast<float>(static_cast<int>(mxconst::SLIDER_MAX_RND_DIST * 4.0)), 4.0f, 5.0f) },
    { mx_plane_types_enum::plane_type_jets, radio_plane_type (mx_plane_types_enum::plane_type_jets, "Jets", 80.0f, static_cast<float>(static_cast<int>(mxconst::SLIDER_MAX_RND_DIST * 4.0)), 40.0f, 40.0f) },
    { mx_plane_types_enum::plane_type_airline, radio_plane_type (mx_plane_types_enum::plane_type_airline, "Airline", 120.0f, static_cast<float>(static_cast<int>(mxconst::SLIDER_MAX_RND_DIST * 6.0)), 40.0f, 30.0f) },
    { mx_plane_types_enum::plane_type_heavy_airline, radio_plane_type (mx_plane_types_enum::plane_type_heavy_airline, "H.Airline", 120.0f, static_cast<float>(static_cast<int>(mxconst::SLIDER_MAX_RND_DIST * 10.0)), 40.0f, 30.0f) },
    { mx_plane_types_enum::plane_type_cargo, radio_plane_type (mx_plane_types_enum::plane_type_cargo, "Cargo", 120.0f, static_cast<float>(static_cast<int>(mxconst::SLIDER_MAX_RND_DIST * 6.0)), 40.0f, 30.0f) },
    { mx_plane_types_enum::plane_type_heavy_cargo, radio_plane_type (mx_plane_types_enum::plane_type_heavy_cargo, "H.Cargo", 120.0f, static_cast<float>(static_cast<int>(mxconst::SLIDER_MAX_RND_DIST * 10.0)), 40.0f, 30.0f) }
  };


  inline std::unordered_map<int, std::vector<const char *>> mapMissionCategories = {
  { static_cast<int> (missionx::mx_ui_mission_type::medevac), data_manager::strct_ui_share_data.medevac_arr },
  { static_cast<int> (missionx::mx_ui_mission_type::oil_rig), data_manager::strct_ui_share_data.oilrig_arr },
  { static_cast<int> (missionx::mx_ui_mission_type::cargo), data_manager::strct_ui_share_data.cargo_arr }
};




  // ----------------------------------------------
  // -- STATIC STRUCTS
  // ----------------------------------------------

  // Setup screen
  inline mx_setup_layer strct_setup_layer;

  // Advance settings Popup Window
  inline mx_popup_adv_settings_strct adv_settings_strct;

  inline mx_generate_template_layer strct_generate_template_layer;

  // ILS screen layer struct
  inline mx_ils_layer strct_ils_layer;

  // Shared layer struct data
  inline mx_cross_layer_property strct_cross_layer_properties;

  // define the user creation screen struct
  inline mx_user_create_mission_layer strct_user_create_layer;

// ----------------------------------------------
// -- STATIC WinImguiBriefer Parameters
// ----------------------------------------------

  inline bool flag_generatedRandomFile_success{false}; // we use this flag to distinguish when engine ran and finish generating a mission based on RandomEngin. We can then display the correct output in the UI

 // We have three ways to store messages to display to the user.
  // One directly in the missionx::user_message_line1,
  // the "data_manager::strct_ui_share_data.ongoing_status_message_line2"
  // and a third one the: data_manager::strct_ui_share_data.error_message_line3.
  //inline std::string     user_message_line1{ "> " };


// --------------------------------------------------------------------
// --------------------------------------------------------------------
// --------------------------------------------------------------------
// ----------- SHARED and SELF CONTAINED FUNCTIONS
// ----------- SHARED and SELF CONTAINED FUNCTIONS
// ----------- SHARED and SELF CONTAINED FUNCTIONS
// --------------------------------------------------------------------
// --------------------------------------------------------------------
// --------------------------------------------------------------------

inline void
addAdvancedSettingsPropertiesBeforeGeneratingRandomMission ()
{

  // v25.09.2 Make sure that once you hit the generate button, and you picked x-plane time, we re-read X-Plane clock, especially if the sim is not in pause state
  if (missionx::adv_settings_strct.iRadioRandomDateTime_pick == missionx::mx_ui_random_date_time_type::xplane_day_and_time)
  {
    missionx::adv_settings_strct.iClockDayOfYearPicked = dataref_manager::getLocalDateDays (); // strct_user_create_layer.iClockDayOfYearPicked
    missionx::adv_settings_strct.iClockHourPicked      = dataref_manager::getLocalHour (); // strct_user_create_layer.iClockHourPicked
    missionx::adv_settings_strct.iClockMinutesPicked   = dataref_manager::getLocalMinutes (); // How many minutes passed since the start of the hour
  }


  // v3.303.14 added advance weather/time settings
  missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_STARTING_DAY (), missionx::clockDayOfYear_arr[missionx::adv_settings_strct.iClockDayOfYearPicked]);
  missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_STARTING_HOUR (), mxUtils::stringToNumber<int> (missionx::clockHours_arr[missionx::adv_settings_strct.iClockHourPicked]));
  missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_STARTING_MINUTE (), missionx::adv_settings_strct.iClockMinutesPicked);

  // Added weather information for RandomEngine
  switch (missionx::adv_settings_strct.iWeatherType_user_picked)
  {
    case missionx::mx_ui_random_weather_options::pick_pre_defined:
    {
      // store values in the prop_userDefinedMission_ui. During Random mission generation the weather will be picked from the list
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_WEATHER_USER_PICKED (), missionx::adv_settings_strct.get_weather_picked_by_user ());
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_WEATHER_CHANGE_MODE_USER_PICKED (), missionx::adv_settings_strct.get_weather_change_mode_picked_by_user ()); // v3.303.13
    }
    break;
    case missionx::mx_ui_random_weather_options::use_xplane_weather_and_store:
    {
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_WEATHER_USER_PICKED (), mxconst::get_VALUE_STORE_CURRENT_WEATHER_DATAREFS ());
      missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute (mxconst::get_PROP_WEATHER_CHANGE_MODE_USER_PICKED ().c_str ()); // v3.303.13
    }
    break;
    default: // use_xplane_weather - does not store in random.xml file
    {
      missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute (mxconst::get_PROP_WEATHER_USER_PICKED ().c_str ());
      missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute (mxconst::get_PROP_WEATHER_CHANGE_MODE_USER_PICKED ().c_str ()); // v3.303.13
    }
    break;
  } // end switch on weather options

  // Add weight setting
  missionx::data_manager::prop_userDefinedMission_ui.setBoolProperty (mxconst::get_PROP_ADD_DEFAULT_WEIGHTS_TO_PLANE (), missionx::adv_settings_strct.flag_add_default_weight_settings);
  if (missionx::adv_settings_strct.flag_add_default_weight_settings)
  {
    missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_OPT_PILOT_BASE_WEIGHT (), missionx::adv_settings_strct.pilot_base_weight_i);
    // v25.02.1
    if (missionx::data_manager::flag_setupUseXP11InventoryUI)
    {
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_OPT_PASSENGERS_BASE_WEIGHT (), missionx::adv_settings_strct.passengers_base_weight_i);
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_OPT_STORAGE_BASE_WEIGHT (), missionx::adv_settings_strct.cargo_base_weight);
    }
    else
    {
      missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute (mxconst::get_OPT_PASSENGERS_BASE_WEIGHT ().c_str ());
      missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute (mxconst::get_OPT_STORAGE_BASE_WEIGHT ().c_str ());
    }
  }
  else // remove default weight attributes if the "add default wights is not flagged
  {
    missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute (mxconst::get_OPT_PILOT_BASE_WEIGHT ().c_str ());
    missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute (mxconst::get_OPT_PASSENGERS_BASE_WEIGHT ().c_str ());
    missionx::data_manager::prop_userDefinedMission_ui.node.deleteAttribute (mxconst::get_OPT_STORAGE_BASE_WEIGHT ().c_str ());
  }
}


}


#endif //MISSIONX_SHARED_UI_TYPES_H
