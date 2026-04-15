//
// Created by saar.nagar on 4/15/2026.
//

#ifndef MISSIONX_UI_CONV_SCREEN_H
#define MISSIONX_UI_CONV_SCREEN_H

#include "shared_ui_types.hpp"
#include <ImgWindow/ImgWindow.h> // inside libs/imgui4xp

namespace missionx {

typedef struct trig_table_strct_
{
  static const int MAX_ARRAY = 10;

  bool flag_first_point_is_center_cbox{ false };

  int indx{ -1 };
  int trig_type_indx{ 0 }; // type // 0 = rad, 1 = script, 2 = polygonal
  int trig_plane_pos_combo_indx{ 0 }; // 0=ignore 1=on ground 2=airborn
  int trig_ui_elev_type_combo_indx{ -1 }; // { "min/max elev", "lower than..", "above than..", "max elev above ground", "min elev above ground" }

  int  iCurrentBuf{ 0 };
  char buffArray[MAX_ARRAY][512] = { { '\0' } }; // holds all trigger arrays. [0]=name(reserved)

  std::string trig_type_s{ "" }; // type string
  std::string trig_onGround_s{ "" }; // on ground string
  std::string trig_name_s{ "" };

  IXMLNode node_ptr{ IXMLNode::emptyIXMLNode };
  IXMLNode copyOfNode_ptr{ IXMLNode::emptyIXMLNode };

  missionx::Point pos; // used with poly type trigger
  void            init ()
  {
    indx                         = -1;
    trig_type_indx               = 0; // type // 0 = rad, 1 = polygonal, 2 = script
    trig_plane_pos_combo_indx    = 0; // 0=ignore 1=on ground 2=airborne
    trig_ui_elev_type_combo_indx = -1;

    trig_type_s.clear (); // type string
    trig_onGround_s.clear (); // on ground string
    trig_name_s.clear ();

    for (size_t i = 0; i < 10; ++i)
    {
      buffArray[i][0] = '\0'; // holds all trigger arrays
      memset (buffArray[i], '\0', sizeof (buffArray[i]));
    }

    node_ptr = IXMLNode::emptyIXMLNode;
  }

  // reset buff
  void resetBuff (int indx)
  {
    assert (indx < MAX_ARRAY && "Tried to reset a cell not in array.");

    memset (buffArray[indx], '\0', sizeof (buffArray[indx]));
  }

  // get buff
  std::string getBuff (int i)
  {
    assert (i < MAX_ARRAY && "Tried to access cell not in array.");

    return std::string (buffArray[i]);
  }
  // set buff array
  void setBuff (int indx, std::string inVal_s)
  {
    if (indx < MAX_ARRAY)
    {
      resetBuff (indx);
      #ifdef IBM
      memcpy_s (buffArray[indx], sizeof (buffArray[indx]), inVal_s.c_str (), (inVal_s.length () > sizeof (buffArray[indx]) ? sizeof (buffArray[indx]) : inVal_s.length ())); // we copy the memory based on which buffer do not exceeds the buffer.
      #else
      memcpy (buffArray[indx], inVal_s.c_str (), inVal_s.length ());
      #endif

    } // end if in boundaries

  } // end set buff


} mx_trig_strct_;

enum class mxTrig_ui_mode_enm : uint8_t // v3.0.301
{
  naTrigger   = 0,
  editTrigger = 1, // edit mode
  newTrigger  = 2 // add new trigger
};


// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
// -------- CLASS ------------
class ui_conv_screen : public ImgWindow {
protected:
  void buildInterface() override {};

private:
  // I am window number...
  const int myWinNum;

  mx_setup_layer* strct_setup_layer;
  mx_popup_adv_settings_strct* adv_settings_strct;

public:
// --------- Constructors ----------

  ui_conv_screen(const int left, const int top, const int right, const int bot, const XPLMWindowDecoration decoration, const XPLMWindowLayer layer
                , const int &in_win_num
                , mx_setup_layer* inout_strct_setup_layer
                , mx_popup_adv_settings_strct* inout_adv_settings_strct)
                : ImgWindow (left, top, right, bot, decoration, layer)
                  , myWinNum(in_win_num)
                  , strct_setup_layer(inout_strct_setup_layer)
                  , adv_settings_strct(inout_adv_settings_strct) {};


// ----- virtual functions ----------------
  void flc() override {};

// ----- Pointers to parent functions -----

  //
  std::function<void(std::string, int)> setMessage;
  std::function<void()> add_designer_mode_checkbox;
  std::function<bool()> add_ui_checkbox_rerun_random_date_and_time;
  // void add_ui_advance_settings_random_date_time_weather_and_weight_button (int &out_iClockDayOfYearPicked, int &out_iClockHourPicked, int &out_iClockMinutesPicked, const std::string &inTEXT_TYPE = mxconst::get_TEXT_TYPE_TITLE_REG ());
  std::function<void(int&, int&, int&, const std::string&)> add_ui_advance_settings_random_date_time_weather_and_weight_button;
  // execAction (mx_window_actions actionCommand)
  std::function<void(missionx::mx_window_actions)> execAction;
  // void  add_message_text ();
  std::function<void()> add_message_text;



// ----- convert FPLN layer -----
  // v3.0.301
  typedef enum class conv_sub_ui_enum
    : uint8_t
  {
    conv_pick_fpln   = 0,
    conv_design_fpln = 5
  } mx_conv_sub_ui;

  typedef struct _conv_layer
  {
    bool flag_first_time{ true };
    bool flag_foundBriefer_index0{ false };
    bool flag_refresh_table_from_file{ false };

    bool flag_store_state{ false }; // v3.0.303.4
    bool flag_use_loaded_globalSetting_from_conversion_file{ false }; // v3.305.1
    bool flag_load_conversion_file{ false }; // v3.0.303.4

    int file_picked_i{ -1 };
    int way_row_picked_i{ -1 };

    mx_conv_sub_ui conv_sub_ui{ conv_sub_ui_enum::conv_pick_fpln };

    IXMLNode               xConvMainNode{ IXMLNode () }; // { IXMLNode::createXMLTopNode("xml", TRUE) };
    IXMLNode               xSavedGlobalSettingsNode{ IXMLNode () }; // Will hold the <global_settings> stored at the <CONVERSION> element. We should have a flag to keep the Saved globalSettings or use the "advanced settings" (construct a new one)
    IXMLNode               xConvDummy{ IXMLNode () }; // empty <DUMMY> node
    IXMLNode               xConvInfo{ IXMLNode () }; // empty <mission_info> node
    IXMLNode               xTriggers_global{ IXMLNode () }; // empty <triggers> node will be used as the global triggers node that will hold mainly custom user events
    IXMLNode               xXPlaneDataRef_global{ IXMLNode () }; // empty <xpdata> node will be used as the global datarefs
    const std::string_view MISSION_SKELATON_ELEMENT = "<MISSION> </MISSION>";
    const std::string_view DUMMY_SKELATON_ELEMENT   = "<DUMMY> </DUMMY>";

    char buff_dataref[4096]{ '\0' }; // v3.0.303.4 store dataref string
    char buff_globalSettings[4096]{ '\0' }; // v3.305.1 stores the GlobalSettings string

    std::map<std::string, std::string> mapFileList;
    std::vector<const char *>          vecFileList_char; // convert to

    void set_conv_map_files (const std::map<std::string, std::string> inMapFileList)
    {
      mapFileList.clear ();
      mapFileList = inMapFileList;
      convert_map_files_to_const_char_vector ();
    }
    void convert_map_files_to_const_char_vector ()
    {
      vecFileList_char.clear ();
      for (auto &[f, p] : mapFileList)
      {
        vecFileList_char.emplace_back (f.c_str ());
      }
    }

    // reset briefer buffs

    // Triggers
    int  trig_picked_i{ -1 };
    int  trig_seq{ 1 }; // used when creating new triggers only
    bool flag_refreshTriggerListFrom_xNode{ true };

    mxTrig_ui_mode_enm trig_ui_mode{ mxTrig_ui_mode_enm ::naTrigger };

    const std::vector<const char *> vecTrigType_list           = { "Radius", "Script", "Box", "Camera" }; //{ "Radius","Polygonal","Script" };
    const std::vector<const char *> vecTrigType_list_trans     = { "rad", "script", "poly", "camera" }; //{ "rad","script","poly" };
    const std::vector<const char *> vecTrigOnGround_list       = { "Ignore", "On Ground", "Airborne" };
    const std::vector<const char *> vecTrigOnGround_list_trans = { "", "true", "false" };
    const int                       vecTrigTypeCounter_i       = 3;
    const int                       vecTrigOnGroundCount_i     = 3;

    std::map<int, missionx::mx_trig_strct_> mapOfGlobalTriggers;
    std::vector<std::string>                vecGlobalTriggers_names;

    mx_trig_strct_ trigger;

    void set_global_settings_into_buffer (IXMLNode &in_xGlobalSettings) // v3.305.1
    {
      if (!in_xGlobalSettings.isEmpty ())
      {
        std::string data_4096_s;
        for (int i1 = 0; i1 < in_xGlobalSettings.nChildNode (); ++i1) // read all sub elements
        {
          IXMLRenderer render;
          data_4096_s += render.getString (in_xGlobalSettings.getChildNode (i1));
        }

        #ifdef IBM
        memcpy_s (buff_globalSettings, sizeof (buff_globalSettings), data_4096_s.c_str (), sizeof (buff_globalSettings) - 1);
        #else
        memcpy (buff_globalSettings, data_4096_s.c_str (), sizeof (buff_globalSettings) - 1);
        #endif

        xSavedGlobalSettingsNode = in_xGlobalSettings.deepCopy ();
      }
    }

  } mx_conv_layer;
  mx_conv_layer strct_conv_layer;

  //// END CONVERTION struct

  // convert FPLN members
  std::map<std::string, std::string> read_fpln_files (); // imp
  void                               draw_conv_popup_datarefs (IXMLNode &inXpData); // imp
  void                               draw_conv_popup_globalSettings (IXMLNode &inOutGlobalSettingsNode); // imp
  void                               draw_conv_popup_flight_leg_detail (missionx::mx_local_fpln_strct &inLegData); // imp
  void                               draw_conv_popup_briefer (missionx::mx_local_fpln_strct &inLegData); // imp
  void                               subDraw_popup_user_lat_lon (mx_trig_strct_ &inout_trig);
  void                               subDraw_popup_outcome (mx_trig_strct_ &inout_trig, IXMLNode &inMessageTemplates);

  void subDraw_fpln_table (IXMLNode &inMainNode, std::map<int, missionx::mx_local_fpln_strct> &in_map_tableOfParsedFpln);
  bool validate_conversion_table (IXMLNode &inMainNode, std::map<int, missionx::mx_local_fpln_strct> in_map_tableOfParsedFpln);

  // The function should receive the parent of all <trigger> elements. The information that will be displayed will be added to it.
  void subDraw_ui_xTrigger_main (missionx::mx_local_fpln_strct &inLegData, bool &in_out_needRefresh_b, int inLegIndex, std::map<int, missionx::mx_trig_strct_> &inMapOfGlobalTriggers, std::vector<std::string> &inVecGlobalTriggers_names);
  void subDraw_ui_xTrigger_detail (mx_trig_strct_ &inTrig_ptr, bool &in_out_needRefresh_b, std::string &suggested_name, missionx::mx_local_fpln_strct &inLegData);


  void subDraw_ui_xRadius (IXMLNode &node, int pad_x_i = 0); // display the radius widget. Used for triggers based rad
  void subDraw_ui_xPolyBox (IXMLNode &pNode, mx_trig_strct_ &inTrig_ptr); // display the poligonal box widget. Used for triggers based box/poly. We have 2 types, bottom left and center based
  void subDraw_ui_xTrigger_elev (mx_trig_strct_ &inTrig_ptr, IXMLNode &node, bool inResetPick = false); // display the elevation options
  // display a multiline input text for <scriptlet> element. Add it to the pNode once clicking the [apply] button. Returns the name of the scriptlet.
  void subDraw_ui_xScriptlet (IXMLNode &pNode, mxTrig_ui_mode_enm &inMode, mx_trig_strct_ *inTrig_ptr, const std::string inScriptInputLabel = "", missionx::mx_local_fpln_strct *inLegData = nullptr, char *inOutBuff = nullptr);

  // ----- END convert FPLN layer -----

  // --------- MAIN Functions ---------
  void draw_conv_main_fpln_to_mission_window (); // v3.0.301


  std::map<int, missionx::mx_local_fpln_strct> read_and_parse_saved_state (const std::string inPathAndFile); // v3.0.303.4 Read stored conversion state

  void draw_conv_popup_which_global_settings_to_save (std::string_view inPopupWindowName);
};

} // missionx

#endif //MISSIONX_UI_CONV_SCREEN_H
