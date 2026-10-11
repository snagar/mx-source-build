//
// Created by saar.nagar on 2026-10-08.
//

#include "ui_setup_screen.h"
#include "../../libs/imgui4xp/colors/mx_colors.hpp"
#include "../../io/OptimizeAptDat.h"

namespace missionx {

// -------------------------------------------
// ------------ draw setup layer
// -------------------------------------------

ui_setup_screen::~ui_setup_screen() {}

void ui_setup_screen::draw_setup_layer()
{
  const static std::string IMAGE          = mxconst::get_BITMAP_BTN_SETUP_24X18 ();
  static constexpr float   img_ps_f       = 0.5f;
  float                    pos_x          = data_manager::mapCachedPluginTextures[IMAGE].sImageData.getW_f () * img_ps_f;
  const static std::string slider_label_s = "[" + Utils::formatNumber<double> (mxconst::SLIDER_MIN_RND_DIST, 0) + ".." + Utils::formatNumber<double> (mxconst::SLIDER_MAX_RND_DIST, 0) + "]";

  auto         win_size_vec2 = ImGui::GetWindowSize ();
  const auto   modal_w       = mxUiGetContentWidth ();
  const auto   modal_h       = ImGui::GetWindowHeight ();
  const ImVec2 modal_center (modal_w * 0.5f, modal_h * 0.5f);
  const auto   vec2_multiLine_dim = ImVec2 (win_size_vec2.x - 25.0f, 120.0f);


  //------------------------------------------------
  //                  First Time
  //------------------------------------------------
  if (missionx::strct_setup_layer.is_first_time)
  {
    std::string stored_overpass_url = Utils::getNodeText_type_6 (missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_URL (), mxconst::get_DEFAULT_OVERPASS_URL ());
    mxUtils::copy_string_to_buffer (stored_overpass_url, missionx::strct_setup_layer.overpass_url_buf[0], sizeof(missionx::strct_setup_layer.overpass_url_buf));

    // v3.303.9.1
    if (!missionx::data_manager::xMissionxProperties_node.getChildNode (mxconst::get_ELEMENT_SCORING ().c_str ()).isEmpty ())
    {
      IXMLNode    node          = missionx::data_manager::xMissionxProperties_node.getChildNode (mxconst::get_ELEMENT_SCORING ().c_str ());
      std::string nodeContent_s = Utils::xml_get_node_content_as_text (node);
      mxUtils::copy_string_to_buffer (nodeContent_s, missionx::strct_setup_layer.default_scoring_buf[0], sizeof (missionx::strct_setup_layer.default_scoring_buf) );

    }

    missionx::strct_setup_layer.flag_lock_overpass_url = Utils::getNodeText_type_1_5<bool> (missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_LOCK_OVERPASS_URL_TO_USER_PICK (), false);

    // initialize the combo option from the preference file. Search for same URL string
    for (auto i = static_cast<size_t> (0); i < missionx::strct_setup_layer.vecOverpassUrls_char.size (); ++i)
    {
      if (stored_overpass_url == missionx::strct_setup_layer.vecOverpassUrls_char.at (i))
      {
        missionx::data_manager::overpass_user_picked_combo_i = static_cast<int> (i);
        break;
      }
    }


    #ifdef LIN
    missionx::strct_setup_layer.iLinuxFlavor_val = Utils::getNodeText_type_1_5<int> (missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_LINUX_FLAVOR_CODE_I (), 0); // v3.303.8.1
    #endif


    missionx::strct_setup_layer.is_first_time = false;
  } // end first time



  //------------------------------------------------
  //                  Draw Setup Layer
  //------------------------------------------------
  ImGui::BeginChild ("draw_setup_layer_01", ImVec2 (0.0f, static_cast<float> (missionx::WINDOWS_MAX_HEIGHT) - FOOTER_REGION_CLEARENCE_PX));
  {
    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
    {
      ///////// Image ////////
      ImGui::BeginGroup ();
      ui::wrap_imgui::Image (data_manager::mapCachedPluginTextures[IMAGE].gTexture, ImVec2 (data_manager::mapCachedPluginTextures[IMAGE].sImageData.getW_f () * img_ps_f, data_manager::mapCachedPluginTextures[IMAGE].sImageData.getH_f () * img_ps_f));
      ImGui::EndGroup ();
    }
    ImGui::PopStyleColor (1);
  }



  //------------------------------------------------
  //          START CollapsingHeader SETUP NODES
  //------------------------------------------------


  pos_x += 50.0f;
  ImGui::SameLine (pos_x);
  ImGui::BeginGroup ();
  {
    //------------------------------------------------
    //                  Pilot Name
    //------------------------------------------------

    // v3.305.1 added Pilot name for story mode in <message> element.
    mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
    mx_img_window::HelpMarker ("> Stores name in preference file only when pressing [Enter]\n> Only used in Messages that use the \"story mode\".\n> Might be extended in future builds to all missions.");
    ImGui::SameLine ();
    ImGui::TextColored (missionx::color::color_vec4_turquoise, "Your pilot nick name:");
    ImGui::SameLine ();
    this->mx_add_tooltip (missionx::color::color_vec4_white, "Press [Enter] to store value.");
    ImGui::SameLine ();
    ImGui::SetNextItemWidth (120.0f);
    if (ImGui::InputTextWithHint ("##PilotNickName", "Pilot Nickname", missionx::strct_setup_layer.buf_pilotName, 10, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue))
    {
      Utils::xml_search_and_set_node_text (system_actions::pluginSetupOptions.node, mxconst::SETUP_PILOT_NAME, std::string (missionx::strct_setup_layer.buf_pilotName), this->mxcode.STRING, true);
      this->execAction (mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
    }
    ImGui::SameLine ();
    ImGui::TextColored (missionx::color::color_vec4_grey, "Press [enter] to register the name."); // v25.08.1

    ImGui::Spacing (); // v3.305.3 moved


    this->add_ui_pause_in_2d_mode ();
    ImGui::Separator ();

    bool bDesignerMode = Utils::readNodeNumericAttrib<int> (missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_ENABLE_DESIGNER_MODE (), false); // 0 = false
    if (ImGui::Checkbox ("Toggle Designer Mode", &bDesignerMode))
    {
      missionx::data_manager::queFlcActions.push_back (mx_flc_pre_command::toggle_designer_mode);
    }

    ImGui::SameLine (0.0f, 15.0f);
    bool bCueInfo = Utils::readNodeNumericAttrib<int> (missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_DISPLAY_VISUAL_CUES (), false); // 0 = false
    if (ImGui::Checkbox ("Toggle Cue Info", &bCueInfo))
    {
      missionx::data_manager::queFlcActions.push_back (mx_flc_pre_command::toggle_cue_info_mode);
    }

    // v26.02.1
    bool bDisplayClock = missionx::system_actions::pluginSetupOptions.getNodeText_type_1_5<bool>(mxconst::get_OPT_DISPLAY_UI_CLOCKS(), true);
      ImGui::SameLine (0.0f, 15.0f);
    if (ImGui::Checkbox ("Toggle Clocks", &bDisplayClock))
    {
      missionx::data_manager::queFlcActions.push_back (mx_flc_pre_command::toggle_ui_clocks);
    }

    bool bDisplayFPS = missionx::system_actions::pluginSetupOptions.getNodeText_type_1_5<bool>(mxconst::get_OPT_DISPLAY_UI_FPS(), true);
      ImGui::SameLine (0.0f, 15.0f);
    if (ImGui::Checkbox ("Toggle FPS", &bDisplayFPS))
    {
      missionx::data_manager::queFlcActions.push_back (mx_flc_pre_command::toggle_ui_fps);
    }


    mxUiReleaseLastFont ();
    ImGui::Separator ();
    ImGui::Spacing ();



    int index = 0;
    //------------------------------------------------
    //                  General Settings Group
    //------------------------------------------------
    // // ImGui::SetWindowFontScale (mxconst::DEFAULT_BASE_FONT_SCALE);


    mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());

    missionx::strct_setup_layer.mapSetupHeaders[index].setState ((ImGui::CollapsingHeader (missionx::strct_setup_layer.mapSetupHeaders[index].title.c_str ())));
    if (missionx::strct_setup_layer.mapSetupHeaders[index].bState)
    {
      mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
      ImGui::TextColored (missionx::color::color_vec4_yellow, "GPS Waypoints & Markers:");

      // GPS Exposure
      if (data_manager::missionState >= mx_mission_state_enum::mission_is_running)
      {
        ImGui::PushItemFlag (ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar (ImGuiStyleVar_Alpha, ImGui::GetStyle ().Alpha * 0.5f);
      }

      this->add_ui_expose_all_gps_waypoints ();


      if (data_manager::missionState >= mx_mission_state_enum::mission_is_running)
      {
        ImGui::PopItemFlag ();
        ImGui::PopStyleVar ();
      }

      // target markers
      if (ImGui::Checkbox ("Display Target Markers", &missionx::strct_setup_layer.bDisplayTargetMarkers))
      {
        // ADD set option value
        missionx::system_actions::pluginSetupOptions.set_node_text_type_1_5<bool> (mxconst::get_SETUP_DISPLAY_TARGET_MARKERS (), missionx::strct_setup_layer.bDisplayTargetMarkers);
        this->execAction (mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
      }

      ImGui::Separator ();

      // Inventory X11 compatibility mode // 24.12.2
      if (missionx::data_manager::xplane_ver_i > missionx::XP12_VERSION_NO)
      {
        // disable while running
        const bool bDisable = mxStartUiDisableState (missionx::data_manager::missionState >= mx_mission_state_enum::mission_is_running);
        {
          // v26.09.3
          ImGui::TextColored (missionx::color::color_vec4_yellow, "Inventory Setup:");
          mx_img_window::HelpMarker ("Do not generate \"External Inventories\" in each waypoint.");
          ImGui::SameLine();
          add_ui_no_inventories_checkbox(missionx::color::color_vec4_white);

          this->add_ui_xp11_comp_checkbox (true); // v24.12.2
        }
        mxEndUiDisableState (bDisable);
      }

      ImGui::Separator (); // v3.305.1
      // v26.09.3
      ImGui::TextColored (missionx::color::color_vec4_yellow, "Messages:");
      this->add_ui_suppress_distance_messages_checkbox_ui (missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS); // v25.02.1

      ImGui::Separator (); // v3.305.1

      // Cycle Mission-X Log File
      ImGui::TextColored (missionx::color::color_vec4_yellow, "Cycle plugin log files:");
      mx_img_window::HelpMarker ("Cycle Mission-X log file in every new session.\nThe plugin will keep few versions of the logfile for debug or as a reference.\n\nYou can find the files in the plugin folder.");
      ImGui::SameLine ();
      if (ImGui::Checkbox ("Cycle log files", &missionx::strct_setup_layer.bCycleLogFiles))
      {
        // ADD set option value
        missionx::system_actions::pluginSetupOptions.set_node_text_type_1_5<bool> (mxconst::get_OPT_CYCLE_LOG_FILES (), missionx::strct_setup_layer.bCycleLogFiles);
        this->execAction (mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
      }

      ImGui::Separator ();
      ImGui::NewLine ();

      mxUiReleaseLastFont ();

    } // end collapse


    //------------------------------------------------
    //      Simbrief + flightplandatabase.com
    //------------------------------------------------
    ++index; // 1

    missionx::strct_setup_layer.mapSetupHeaders[index].setState ((ImGui::CollapsingHeader (missionx::strct_setup_layer.mapSetupHeaders[index].title.c_str ())));
    if (missionx::strct_setup_layer.mapSetupHeaders[index].bState)
    {
      mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
      {
        ImGui::Spacing ();
        ImGui::TextColored (missionx::color::color_vec4_greenyellow, "Simbrief:");

        this->add_ui_simbrief_pilot_id ();
        ImGui::Spacing ();
        ImGui::Separator ();

        ImGui::TextColored(missionx::color::color_vec4_greenyellow, "flightplandatabase:");
        this->add_ui_flightplandb_key (false);

        // ------------------------
        // -- AI info
        // ------------------------
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(missionx::color::color_vec4_greenyellow, "LLM Server (AI):");

        // v26.08.1
        this->add_ui_ai_use_ai_checkbox ();
        this->add_ui_ai_server_url();

      }
      mxUiReleaseLastFont ();

      ImGui::Spacing ();
      ImGui::Separator ();
    }
    //------------------------------------------------
    //                  APT Dat optimizations
    //------------------------------------------------
    ++index; // 2

    ImGui::PushStyleColor (ImGuiCol_Header, missionx::color::color_vec4_grey);

    missionx::strct_setup_layer.mapSetupHeaders[index].setState ((ImGui::CollapsingHeader (missionx::strct_setup_layer.mapSetupHeaders[index].title.c_str ())));
    if (missionx::strct_setup_layer.mapSetupHeaders[index].bState)
    {
      mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.305.1

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text ("Execute the apt data optimization scripts:");
      ImGui::PopStyleColor (1); // back to white

      // // ImGui::SetWindowFontScale (mxconst::DEFAULT_BASE_FONT_SCALE);

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
      {
        if (data_manager::missionState >= mx_mission_state_enum::mission_is_running)
        {
          ImGui::PushItemFlag (ImGuiItemFlags_Disabled, true);
          ImGui::PushStyleVar (ImGuiStyleVar_Alpha, ImGui::GetStyle ().Alpha * 0.5f);
        }
        if (ImGui::Button ("Run Apt Data Optimization", ImVec2 (200.0f, 30.0f)))
        {
          if (missionx::data_manager::flag_apt_dat_optimization_is_running || OptimizeAptDat::aptState.flagIsActive)
            this->set_bottom_message_line1 ("Apt DATA is running.... please wait.", 8);
          else
            missionx::data_manager::queFlcActions.push_back (mx_flc_pre_command::exec_apt_dat_optimization); // v3.0.253.6
        }
        if (data_manager::missionState >= mx_mission_state_enum::mission_is_running)
        {
          ImGui::PopItemFlag ();
          ImGui::PopStyleVar ();
        }
      }
      ImGui::PopStyleColor (3);

      ImGui::Separator (); // v3.305.1
      ImGui::NewLine (); // v3.305.1


      mxUiReleaseLastFont (); // v3.305.1
    }

    ImGui::PopStyleColor (); // header color

    //------------------------------------------------
    //                  Tools Group
    //------------------------------------------------

    ++index; // 3
    // // ImGui::SetWindowFontScale (mxconst::DEFAULT_BASE_FONT_SCALE);
    // if (ImGui::CollapsingHeader("TOOLS"))
    missionx::strct_setup_layer.mapSetupHeaders[index].setState ((ImGui::CollapsingHeader (missionx::strct_setup_layer.mapSetupHeaders[index].title.c_str ())));
    if (missionx::strct_setup_layer.mapSetupHeaders[index].bState)
    {
      constexpr const static float BTN_WIDTH_F = 300.0f; // v3.305.1

      mx_img_window::HelpMarker ("Create external flight plan files based on the \"fpln_folders.ini\" settings and the current GPS flight plan.");
      ImGui::SameLine ();

      mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.305.1

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text ("Create external FPLN based of GPS");
      ImGui::PopStyleColor (1); // back to white

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
      if (ImGui::Button ("Write GPS to External FPLN", ImVec2 (BTN_WIDTH_F, 25.0f)))
      {
        missionx::data_manager::queFlcActions.push_back (mx_flc_pre_command::write_fpln_to_external_folder); //
      }
      ImGui::PopStyleColor (3); // back to white

      ImGui::NewLine ();


      // v3.303.14 Write Plane Coordinates
      mx_img_window::HelpMarker ("Write Plane coordinates into the missionx.log file.");
      ImGui::SameLine ();

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text ("Dump Plane coordinates into missionx.log file");
      ImGui::PopStyleColor (1); // back to white

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
      if (ImGui::Button ("Dump Plane coordinates", ImVec2 (BTN_WIDTH_F, 25.0f)))
      {
        missionx::data_manager::write_plane_position_to_log_file ();
        this->set_bottom_message_line1 ("Wrote Plane coordinates to missionx.log file.", 5); // v3.305.3
      }
      ImGui::PopStyleColor (3); // back to white


      ImGui::NewLine ();

      // v3.303.14 Write Camera Coordinates
      mx_img_window::HelpMarker ("Write Camera coordinates into the missionx.log file.");
      ImGui::SameLine ();

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text ("Dump Camera coordinates into missionx.log file");
      ImGui::PopStyleColor (1); // back to white

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
      if (ImGui::Button ("Dump Camera coordinates", ImVec2 (BTN_WIDTH_F, 25.0f)))
      {
        missionx::data_manager::write_camera_position_to_log_file ();
        this->set_bottom_message_line1 ("Wrote Camera coordinates to missionx.log file.", 5); // v3.305.3
      }
      ImGui::PopStyleColor (3); // back to white


      ImGui::NewLine ();

      // v3.303.13 Write weather data
      mx_img_window::HelpMarker ("Write Weather datarefs into the missionx.log file so you could use that in a scriptlet.\nXP12 designers are encourage to double check the \"change_mode, variability, turbulence, cloud_height_msl\" datarefs and tweak them for better experience.");
      ImGui::SameLine ();

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text ("Dump weather related datarefs into missionx.log file");
      ImGui::PopStyleColor (1); // back to white

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
      if (ImGui::Button ("Dump Weather Datarefs", ImVec2 (BTN_WIDTH_F, 25.0f)))
      {
        missionx::data_manager::write_weather_state_to_log_file ();
        this->set_bottom_message_line1 ("Wrote Weather info into missionx.log file.", 5); // v3.305.3
      }
      ImGui::PopStyleColor (3); // back to white


      ImGui::NewLine ();

      mx_img_window::HelpMarker ("!!!This is a template designer feature!!!\n\nReads the file '{XP}/Custom Scenery/missionx/random/points.xml' and probe <point> locations to test if it is water or land.\nThe information will be used by the Random Engine (Read the designer guide, check the Appendix)");
      ImGui::SameLine ();

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text ("Probe <point> elements, best used if you are in the same DSF area.");
      ImGui::PopStyleColor (1); // back to white

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
      if (ImGui::Button ("Update <point> elements in points.xml file", ImVec2 (BTN_WIDTH_F, 25.0f)))
      {
        missionx::data_manager::queFlcActions.push_back (mx_flc_pre_command::update_point_in_file_with_template_based_on_probe); //
      }
      ImGui::PopStyleColor (3); // back to white


      ImGui::Separator ();
      ImGui::NewLine ();


      mxUiReleaseLastFont (); // v3.305.1

    } // TOOLS


    //------------------------------------------------
    //                  Normalized Volume Group
    //------------------------------------------------
    ++index; // 4

    // v3.0.303.6
    // if (ImGui::CollapsingHeader("Normalize Mission Sound Volume"))
    missionx::strct_setup_layer.mapSetupHeaders[index].setState ((ImGui::CollapsingHeader (missionx::strct_setup_layer.mapSetupHeaders[index].title.c_str ())));
    if (missionx::strct_setup_layer.mapSetupHeaders[index].bState)
    {
      mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.305.1

      // // ImGui::SetWindowFontScale (mxconst::DEFAULT_BASE_FONT_SCALE);
      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      {
        ImGui::Text ("Force Normalized Mission Volume: ");

        mx_img_window::HelpMarker ("Normalized mission volume means that we force a max volume over the mission volume definition (NOT X-Plane sound volume).\nThe effect takes place when the sound file starts to play and not during play.");
        ImGui::SameLine ();
        ImGui::Text ("Force Volume ?: ");
        ImGui::SameLine ();
      }
      ImGui::PopStyleColor (1); // back to white

      if (ImGui::Checkbox ("##normalizedCheckbox", &missionx::strct_setup_layer.bForceNormalizedVolume))
      {
        missionx::system_actions::pluginSetupOptions.set_node_text_type_1_5<bool> (mxconst::get_SETUP_NORMALIZE_VOLUME_B (), missionx::strct_setup_layer.bForceNormalizedVolume);
        this->execAction (mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
      }
      if (missionx::strct_setup_layer.bForceNormalizedVolume)
      {
        ImGui::SameLine (0.0f, 10.0f);
        ImGui::SetNextItemWidth (100.0f);
        if (ImGui::InputInt ("(0..100)##volumeValue", &missionx::strct_setup_layer.iNormalizedVolume_val, 1, 2))
        {
          if (missionx::strct_setup_layer.iNormalizedVolume_val < static_cast<int> (mxconst::MIN_SOUND_VOLUME_F))
            missionx::strct_setup_layer.iNormalizedVolume_val = static_cast<int> (mxconst::MIN_SOUND_VOLUME_F);
          else if (missionx::strct_setup_layer.iNormalizedVolume_val > static_cast<int> (mxconst::MAX_SOUND_VOLUME_F))
            missionx::strct_setup_layer.iNormalizedVolume_val = static_cast<int> (mxconst::MAX_SOUND_VOLUME_F);

          missionx::system_actions::pluginSetupOptions.set_node_text_type_1_5<int> (mxconst::get_SETUP_NORMALIZED_VOLUME (), missionx::strct_setup_layer.iNormalizedVolume_val);
          this->execAction (mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
        }
      }

      ImGui::Separator ();
      ImGui::NewLine ();

      mxUiReleaseLastFont (); // v3.305.1
    }



    //------------------------------------------------
    //                  OVERPASS
    //------------------------------------------------
    index++; // 5 Overpass
    // if (ImGui::CollapsingHeader("OVERPASS Setup"))
    missionx::strct_setup_layer.mapSetupHeaders[index].setState ((ImGui::CollapsingHeader (missionx::strct_setup_layer.mapSetupHeaders[index].title.c_str ())));
    if (missionx::strct_setup_layer.mapSetupHeaders[index].bState)
    {
      mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.305.1
      ImGui::PushItemWidth (450.0f);
      {

        // // ImGui::SetWindowFontScale (mxconst::DEFAULT_BASE_FONT_SCALE);
        ImGui::TextColored (missionx::color::color_vec4_yellow, "Preferred Overpass URL:");
        ImGui::PushID ("##overpassCustomUrl");
        if (ImGui::Combo ("##OverPassURLs", &missionx::data_manager::overpass_user_picked_combo_i, missionx::strct_setup_layer.vecOverpassUrls_char.data (), static_cast<int> (missionx::strct_setup_layer.vecOverpassUrls_char.size ())))
        {
          #ifndef RELEASE
          const std::string stored_overpass_url = missionx::strct_setup_layer.vecOverpassUrls_char.at(missionx::data_manager::overpass_user_picked_combo_i); // for debug purposes only
          #endif // !RELEASE
          missionx::data_manager::overpass_last_url_indx_used_i = missionx::data_manager::overpass_user_picked_combo_i; // v3.0.255.4.1 store user picked in last run so we will start from it during next run

          Utils::xml_search_and_set_node_text (system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_URL (), std::string (missionx::strct_setup_layer.vecOverpassUrls_char.at (missionx::data_manager::overpass_user_picked_combo_i)), this->mxcode.STRING, true);

          this->execAction (mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
        }
        ImGui::PopID ();
        ImGui::PopItemWidth ();
        // Lock URL - checkbox
        ImGui::SameLine ();
        if (ImGui::Checkbox ("Lock URL", &missionx::strct_setup_layer.flag_lock_overpass_url))
        {
          missionx::system_actions::pluginSetupOptions.set_node_text_type_1_5<bool> (mxconst::get_SETUP_LOCK_OVERPASS_URL_TO_USER_PICK (), ((missionx::strct_setup_layer.flag_lock_overpass_url) ? true : false));
          this->execAction (mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
        }


        // --- Overpass list to choose from. Not stored in preferences file.
        ImGui::NewLine();
        mx_img_window::HelpMarker("At least one URL must be selected from the list.\nYou can modify your preferences during mission creation.\nCheck the error message line during generation (scroll down if needed).");
        ImGui::SameLine();
        ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", "Limit available Overpass URLs (this setting is not saved in preferences):");
        for (int url_index = 0; const auto& url: missionx::data_manager::vecOverpassUrls)
        {
          ImGui::PushID(url_index);
          if (ImGui::Checkbox("##urlCheckboxes", &data_manager::strct_ui_share_data.map_ui_user_picks_overpass_urls[url_index]))
          {
            // validate at least one of the items in the container is "true"
            const bool at_least_one_is_picked = std::ranges::any_of(data_manager::strct_ui_share_data.map_ui_user_picks_overpass_urls, [](const auto v_pair) { return v_pair.second; });
            if (!at_least_one_is_picked)
            {
              data_manager::strct_ui_share_data.map_ui_user_picks_overpass_urls[url_index] = true; // force at least one item to be true
            }
          }
          ImGui::PopID();
          ImGui::SameLine();
          const auto ui_text_color = (data_manager::strct_ui_share_data.map_ui_user_picks_overpass_urls[url_index]) ? missionx::color::color_vec4_beige : missionx::color::color_vec4_darkgray;
          ImGui::TextColored(ui_text_color, "%s", url.c_str());
          ++url_index;
        }

        ImGui::Separator (); // v3.305.1
        ImGui::NewLine (); // v3.305.1
      }

      mxUiReleaseLastFont (); // v3.305.1
    }
    //------------------------------------------------
    //                  Medevac Group
    //------------------------------------------------
    index++; // 6 Medevac
    // if (ImGui::CollapsingHeader("Medevac Setup"))
    missionx::strct_setup_layer.mapSetupHeaders[index].setState ((ImGui::CollapsingHeader (missionx::strct_setup_layer.mapSetupHeaders[index].title.c_str ())));
    if (missionx::strct_setup_layer.mapSetupHeaders[index].bState)
    {
      mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.305.1

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text ("Settings Will be applied only to helos missions");
      ImGui::Text ("and it depends on template setup");
      ImGui::PopStyleColor (1); // back to white

      ImGui::NewLine ();

      add_ui_skewed_marker_checkbox (); // v3.0.253.6

      ImGui::Separator (); // v3.305.1
      ImGui::NewLine (); // v3.305.1


      mxUiReleaseLastFont (); // v3.305.1
    }

    //------------------------------------------------
    //                  External Flight Plan
    //------------------------------------------------
    // if (ImGui::CollapsingHeader("External Flight Plan Setup"))
    index++; // 7 External Flight Plan
    missionx::strct_setup_layer.mapSetupHeaders[index].setState ((ImGui::CollapsingHeader (missionx::strct_setup_layer.mapSetupHeaders[index].title.c_str ())));
    if (missionx::strct_setup_layer.mapSetupHeaders[index].bState)
    {
      mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.305.1

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text ("Override external flight plan location and write to X-Plane's 'FMS plans' folder");
      ImGui::Text ("(ignores fpln_folders.ini custom locations)");
      ImGui::PopStyleColor (1); // back to white
      if (ImGui::Checkbox ("Ignore custom FPLN folders and write to X-Plane 'FMS Plans' instead.", &missionx::strct_setup_layer.bOverideCustomExternalFPLN_folders))
      {
        // ADD set option value
        missionx::system_actions::pluginSetupOptions.set_node_text_type_1_5<bool> (mxconst::get_OPT_WRITE_CONVERTED_FPLN_TO_XPLANE_FOLDER (), missionx::strct_setup_layer.bOverideCustomExternalFPLN_folders);
        this->execAction (mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
      }

      ImGui::Separator (); // v3.305.1
      ImGui::NewLine (); // v3.305.1

      mxUiReleaseLastFont (); // v3.305.1
    }

    //------------------------------------------------
    //                  Default Scoring
    //------------------------------------------------
    // if (ImGui::CollapsingHeader("Default Scoring"))
    index++; // 8 Default Scoring
    missionx::strct_setup_layer.mapSetupHeaders[index].setState ((ImGui::CollapsingHeader (missionx::strct_setup_layer.mapSetupHeaders[index].title.c_str ())));
    if (missionx::strct_setup_layer.mapSetupHeaders[index].bState)
    {
      mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.305.1

      // Description Text
      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
      ImGui::Text ("Modify the default scoring XML node in your preferences file.");
      ImGui::Text ("It will be effective only if mission file has no <scoring> element defined.");
      ImGui::Text ("!!! Make changes at your own risk. Only modify the attribute values !!!");
      ImGui::PopStyleColor (1); // back to white


      //////////////// Buttons ////////////////
      {
        // Parse and store button
        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
        ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
        ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
        if (ImGui::Button ("Parse and Store"))
        {
          std::string scoringNode_s (missionx::strct_setup_layer.default_scoring_buf);

          IXMLDomParser iDomTemplate;
          IXMLResults   parse_result_strct;
          IXMLNode      xScoring_node = iDomTemplate.parseString (scoringNode_s.c_str (), mxconst::get_ELEMENT_SCORING ().c_str (), &parse_result_strct).deepCopy (); // parse xml into ITCXMLNode
          if (parse_result_strct.errorCode == IXMLError_None)
          {
            if (!missionx::data_manager::xMissionxProperties_node.isEmpty ())
            {
              if (!missionx::data_manager::xMissionxProperties_node.getChildNode (mxconst::get_ELEMENT_SCORING ().c_str ()).isEmpty ())
                missionx::data_manager::xMissionxProperties_node.getChildNode (mxconst::get_ELEMENT_SCORING ().c_str ()).deleteNodeContent ();

              missionx::data_manager::xMissionxProperties_node.addChild (xScoring_node.deepCopy ());
              missionx::system_actions::store_plugin_options ();
              this->set_bottom_message_line1 ("Default <scoring> information was stored into preference file.", 20);
            }

            #ifndef RELEASE
            Log::logMsg("Valid <scoring>:\n" + scoringNode_s);
            #endif // !RELEASE
          }
          else
          {
            std::string err_s = std::string(IXMLPullParser::getErrorMessage(parse_result_strct.errorCode)) + " [line/col:" + mxUtils::formatNumber<long long>(parse_result_strct.nLine) + "/" + mxUtils::formatNumber<int>(parse_result_strct.nColumn) + "]";
            this->set_bottom_message_line1(err_s, 20);
            #ifndef RELEASE
            Log::logMsg(err_s);
            #endif // !RELEASE

            Log::logMsg ("Not valid <scoring>:\n" + scoringNode_s);
          }


        } // end [Parse and Store]
        ImGui::PopStyleColor (3);
        this->mx_add_tooltip (missionx::color::color_vec4_yellow, "The XML string you entered must be valid or it won't be stored");

        // Reset button
        ImGui::SameLine (0.0f, 50.0f);
        if (ImGui::Button ("reset data", ImVec2 (100.0f, 0.0f)))
        {
          if (!missionx::data_manager::xMissionxProperties_node.getChildNode (mxconst::get_ELEMENT_SCORING ().c_str ()).isEmpty ())
          {
            IXMLNode    node          = missionx::data_manager::xMissionxProperties_node.getChildNode (mxconst::get_ELEMENT_SCORING ().c_str ());
            std::string nodeContent_s = Utils::xml_get_node_content_as_text (node);
            //std::memcpy (missionx::strct_setup_layer.default_scoring_buf, nodeContent_s.c_str (), sizeof (missionx::strct_setup_layer.default_scoring_buf) - 1);
            mxUtils::copy_string_to_buffer(nodeContent_s, missionx::strct_setup_layer.default_scoring_buf[0], sizeof(missionx::strct_setup_layer.default_scoring_buf));
            this->set_bottom_message_line1 ("Reset Data", 5);
          }
          else
            this->set_bottom_message_line1 ("Nothing to reset.", 5);
        }
      } // end buttons


      // Input Text
      ImGui::InputTextMultiline ("##scoringMultiLine", missionx::strct_setup_layer.default_scoring_buf, sizeof (missionx::strct_setup_layer.default_scoring_buf), vec2_multiLine_dim);
      ImGui::TextColored (missionx::color::color_vec4_beige, "%zu of %zu", std::string (missionx::strct_setup_layer.default_scoring_buf).length (), sizeof (missionx::strct_setup_layer.default_scoring_buf));

      ImGui::Separator (); // v3.305.1
      ImGui::NewLine (); // v3.305.1

      mxUiReleaseLastFont (); // v3.305.1
    }


    //------------------------------------------------
    //                  Troubleshoot
    //------------------------------------------------
    index++; // 9 troubleshoot // v24.03.2 moved outside #if directive
    missionx::strct_setup_layer.mapSetupHeaders[index].setState ((ImGui::CollapsingHeader (missionx::strct_setup_layer.mapSetupHeaders[index].title.c_str ())));
    if (missionx::strct_setup_layer.mapSetupHeaders[index].bState)
    {
      mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v26.04.4
      {
        ImGui::TextColored (missionx::color::color_vec4_aqua, "Disable Inventory image load, if Inventory screen CTD X-Plane.");
        if (ImGui::Checkbox ("Disable Inventory Image Load ?", &missionx::strct_setup_layer.bDisableInventoryImageLoad))
        {
          // ADD set option value
          missionx::system_actions::pluginSetupOptions.set_node_text_type_1_5<bool> (mxconst::get_OPT_DISABLE_INVENTORY_IMAGE_LOAD (), missionx::strct_setup_layer.bDisableInventoryImageLoad);
          this->execAction (mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
        }

        #ifdef LIN
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(missionx::color::color_vec4_orange, "%s", "--- Linux Only ---");
        ImGui::TextColored (missionx::color::color_vec4_aqua, "only if X-Plane 12 crashes during exit or mission quit.");

        ImGui::TextColored (missionx::color::color_vec4_yellow, "During X-Plane 12 tests, I found out that the FMOD library has issues in specific function cases.\nThe symptom was: X-Plane became unresponsive. ");
        ImGui::TextColored (missionx::color::color_vec4_yellow, "If that happens, please pick the Linux Distro version you are using and\nthe plugin will try to use a workaround for it.");
        if (ImGui::Combo ("Linux Flavor##LinuxFlavorCombo", &missionx::strct_setup_layer.iLinuxFlavor_val, missionx::strct_setup_layer.vecLinuxComboCodes_s.data (), static_cast<int> (missionx::strct_setup_layer.vecLinuxComboCodes_s.size ())))
        {
          missionx::system_actions::pluginSetupOptions.set_node_text_type_1_5<int> (mxconst::get_SETUP_LINUX_FLAVOR_CODE_I (), missionx::strct_setup_layer.iLinuxFlavor_val);
          this->execAction (mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
        }
        #endif

        ImGui::Separator ();
        ImGui::NewLine ();

      }
      mxUiReleaseLastFont ();
    }



    //------------------------------------------------
    //                  Advanced: unsaved options
    //------------------------------------------------


    // if (ImGui::CollapsingHeader("Designer: Unsaved Options"))
    index++; // 10 Unsaved Options
    missionx::strct_setup_layer.mapSetupHeaders[index].setState ((ImGui::CollapsingHeader (missionx::strct_setup_layer.mapSetupHeaders[index].title.c_str ())));
    if (missionx::strct_setup_layer.mapSetupHeaders[index].bState)
    {
      mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.305.1

      ImGui::TextColored (missionx::color::color_vec4_yellow, "The following options won't be stored in the Mission-X preference file.");
      ImGui::TextColored (missionx::color::color_vec4_yellow, "You will have to set them every new X-Plane session.");

      // v24.03.2 plane/camera location
      ImGui::Separator ();
      {
        ImGui::SetNextItemWidth (100.0f);
        ImGui::InputDouble ("##Latitude", &missionx::strct_setup_layer.coord.lat, 0.0, 0.0, "%.8f", ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine ();
        ImGui::TextColored (missionx::color::color_vec4_greenyellow, "%s", "/");
        ImGui::SameLine ();
        ImGui::SetNextItemWidth (100.0f);
        ImGui::InputDouble ("##Longitude", &missionx::strct_setup_layer.coord.lon, 0.0, 0.0, "%.8f", ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine ();
        ImGui::TextColored (missionx::color::color_vec4_greenyellow, "%s", "Elv.");
        ImGui::SameLine ();
        ImGui::SetNextItemWidth (50.0f);
        ImGui::InputDouble ("##Elevation", &missionx::strct_setup_layer.coord.elevation_ft, 0.0, 0.0, "%.0f", ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine ();
        ImGui::TextColored (missionx::color::color_vec4_greenyellow, "%s", "psi");
        ImGui::SameLine ();
        ImGui::SetNextItemWidth (50.0f);
        ImGui::InputFloat ("##Heading", &missionx::strct_setup_layer.coord.heading, 0.0, 0.0, "%.2f", ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine ();

        // Plane/Camera buttons switch
        if (missionx::strct_setup_layer.bPressedPlane)
          ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_greenyellow);
        if (ImGui::Button ("Plane"))
        {
          missionx::strct_setup_layer.coord           = missionx::data_manager::write_plane_position_to_log_file ();
          missionx::strct_setup_layer.btn_coord_state = mx_btn_coordinate_state_enum::plane;
        }
        if (missionx::strct_setup_layer.bPressedPlane)
          ImGui::PopStyleColor ();

        ImGui::SameLine ();

        if (missionx::strct_setup_layer.bPressedCamera)
          ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_greenyellow);
        if (ImGui::Button ("Camera"))
        {
          missionx::strct_setup_layer.coord           = missionx::data_manager::write_camera_position_to_log_file ();
          missionx::strct_setup_layer.btn_coord_state = mx_btn_coordinate_state_enum::camera;
        }
        if (missionx::strct_setup_layer.bPressedCamera)
          ImGui::PopStyleColor ();


        switch (missionx::strct_setup_layer.btn_coord_state)
        {
          case mx_btn_coordinate_state_enum::plane:
          {
            missionx::strct_setup_layer.bPressedPlane  = true;
            missionx::strct_setup_layer.bPressedCamera = false;
          }
          break;
          case mx_btn_coordinate_state_enum::camera:
          {
            missionx::strct_setup_layer.bPressedPlane  = false;
            missionx::strct_setup_layer.bPressedCamera = true;
          }
          break;
          default:
          {
            missionx::strct_setup_layer.bPressedPlane  = false;
            missionx::strct_setup_layer.bPressedCamera = false;
          }
        } // end switch
      }
      ImGui::Separator ();


      // v24.03.2 Designer mode flag
      add_ui_designer_mode_checkbox ();

      // v3.305.2 Debug tab during flight
      mx_img_window::HelpMarker ("Display the \"debug\" tab during mission flight.\nBest used with unpausing in 2D mode, so info will be updated in real time.");
      ImGui::SameLine ();
      ImGui::Checkbox ("Display \"debug\" tab.", &missionx::data_manager::flag_setupShowDebugTabDuringMission);

      // Display Message debug tab
      ImGui::NewLine ();
      ImGui::SameLine (0.0f, 28.0f);

      mx_img_window::HelpMarker (
        R"(Opt-In and display the message debug tab.
ATTENTION !!! It allows you to execute the message even if it is not the correct flight leg. !!!
1. Use to debug messages in story mode and testing background sounds.
2. Use when you want to test a specific message while bypasses leg restriction (be careful).

Use at your own risk!!!
)");
      bool bDisable = mxStartUiDisableState (!missionx::data_manager::flag_setupShowDebugTabDuringMission);
      {
        ImGui::SameLine ();
        ImGui::Checkbox ("Display message debug tab.", &missionx::data_manager::flag_setupShowDebugMessageTab);
      }
      mxEndUiDisableState (bDisable);


      // v3.305.3 skip abort script on script error
      add_ui_skip_abort_setup_checkbox ();

      // Display/Hide Auto skip option in the story mode screen
      mx_img_window::HelpMarker ("Display \"force skip\" option in story mode to make messages complete faster.");
      ImGui::SameLine ();
      ImGui::Checkbox ("Display \"force skip\" option in story mode.", &missionx::data_manager::flag_setupDisplayAutoSkipInStoryMessage);

      // Disable / Enable "auto skip" if and only if the option is checked.
      ImGui::NewLine ();
      ImGui::SameLine (0.0f, 28.0f);
      mx_img_window::HelpMarker ("When in story mode, force skip. This will actually make the story mode message display immediately");

      bDisable = mxStartUiDisableState (!missionx::data_manager::flag_setupDisplayAutoSkipInStoryMessage);
      // Toggle Auto Skip
      ImGui::SameLine ();
      ImGui::Checkbox ("Force Auto Skip, in \"story\" based messages.", &missionx::data_manager::flag_setupAutoSkipStoryMessage);
      mxEndUiDisableState (bDisable);
      // end Disable / Enable "auto skip action"


      // Force Heading
      mx_img_window::HelpMarker ("Force plane heading change even if we are close to the starting position.");
      ImGui::SameLine ();
      ImGui::Checkbox ("When positioning a plane, enforce heading even if start location is in 20m", &missionx::data_manager::flag_setupChangeHeadingEvenIfPlaneIn_20meter_radius);

      // Force plane position
      mx_img_window::HelpMarker ("Force the positioning of the plane even if it is near or at the starting location - will call XPLMPlaceUserAtLocation() function.");
      ImGui::SameLine ();
      ImGui::Checkbox ("Force plane positioning at mission start", &missionx::data_manager::flag_setupForcePlanePositioningAtMissionStart);

      ImGui::Separator (); // v3.305.1
      // v24.03.2
      mxUiHelpMarker (missionx::color::color_vec4_yellow, "Write weather datarefs to both missionx.log file and to the screen.");
      ImGui::SameLine ();
      if (ImGui::TreeNode ("Get Weather Information"))
      {
        static std::string sLastWeatherInformation;
        if (ImGui::Button ("Get Current Weather Information"))
        {
          sLastWeatherInformation = missionx::data_manager::write_weather_state_to_log_file ();
        }
        ImGui::InputTextMultiline ("##WeatherInformation", sLastWeatherInformation.data (), sLastWeatherInformation.length (), ImVec2 (550.0f, 70.0f), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
        ImGui::TreePop ();
      } // end showing weather info on demand

      ImGui::Separator ();


      // Dedicated buttons to print the parts of the mission
      mx_img_window::HelpMarker ("Dump key parts of the loaded XML mission file into the log file.");
      ImGui::SameLine ();
      ImGui::TextColored (missionx::color::color_vec4_aqua, "%s", "The following buttons will dump the active mission XML parts into the Log file.\nYou can achieve the same by saving your work and checking the saved file.\nThat way you can understand what the plugin sees.");

      if (ImGui::Button ("Global Settings"))
      {
        Log::printHeaderToLog ("In Memory: Global Settings");
        Utils::xml_print_node (missionx::data_manager::mx_global_settings.node);
      }
      ImGui::SameLine (0.0f, 10.0f);
      if (ImGui::Button ("Briefer"))
      {
        Log::printHeaderToLog ("In Memory: Briefer");
        Utils::xml_print_node (missionx::data_manager::briefer.node);
      }
      ImGui::SameLine (0.0f, 10.0f);
      if (ImGui::Button ("Datarefs"))
      {
        Log::printHeaderToLog ("In Memory: Datarefs");
        std::for_each (missionx::data_manager::mapDref.begin (),
                       missionx::data_manager::mapDref.end (),
                       [] (std::pair<const std::string, missionx::dataref_param> &pair)
                       {
                         pair.second.readDatarefValue_into_missionx_plugin ();
                         Log::logMsg (fmt::format ("Name: {:.<30}, Key: {:.<60}, Current Value: {}. ", pair.first, pair.second.key, pair.second.get_dataref_scalar_value_as_string ()));
                       });
      }
      ImGui::SameLine (0.0f, 10.0f);
      if (ImGui::Button ("End Element"))
      {
        Log::printHeaderToLog ("In Memory: End Element");
        Utils::xml_print_node (missionx::data_manager::endMissionElement.node);
      }

      ImGui::Separator ();
      if (ImGui::Button ("Flight Legs"))
      {
        Log::printHeaderToLog ("In Memory: Flight Legs");
        std::for_each (missionx::data_manager::mapFlightLegs.begin (), missionx::data_manager::mapFlightLegs.end (), [] (std::pair<const std::string, missionx::Waypoint> &pair) { Log::logMsg (fmt::format ("Flight Leg Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text (pair.second.node))); });
      }
      ImGui::SameLine (0.0f, 10.0f);
      if (ImGui::Button ("Objectives"))
      {
        Log::printHeaderToLog ("In Memory: Objectives");
        std::for_each (missionx::data_manager::mapObjectives.begin (), missionx::data_manager::mapObjectives.end (), [] (std::pair<const std::string, missionx::Objective> &pair) { Log::logMsg (fmt::format ("Objective Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text (pair.second.node))); });
      }
      ImGui::SameLine (0.0f, 10.0f);
      if (ImGui::Button ("Triggers"))
      {
        Log::printHeaderToLog ("In Memory: Triggers");
        std::for_each (missionx::data_manager::mapTriggers.begin (), missionx::data_manager::mapTriggers.end (), [] (std::pair<const std::string, missionx::Trigger> &pair) { Log::logMsg (fmt::format ("Trigger Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text (pair.second.node))); });
      }

      ImGui::Separator ();
      if (ImGui::Button ("Regular Messages"))
      {
        Log::printHeaderToLog ("In Memory: Regular Messages");
        std::for_each (missionx::data_manager::mapMessages.begin (),
                       missionx::data_manager::mapMessages.end (),
                       [] (std::pair<const std::string, missionx::Message> &pair)
                       {
                         if (pair.second.mode != mx_msg_mode::mode_story)
                           Log::logMsg (fmt::format ("Message Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text (pair.second.node)));
                       });
      }
      ImGui::SameLine (0.0f, 10.0f);
      if (ImGui::Button ("Story Messages"))
      {
        Log::printHeaderToLog ("In Memory: Story Messages");
        std::for_each (missionx::data_manager::mapMessages.begin (),
                       missionx::data_manager::mapMessages.end (),
                       [] (std::pair<const std::string, missionx::Message> &pair)
                       {
                         if (pair.second.mode == mx_msg_mode::mode_story)
                           Log::logMsg (fmt::format ("Message Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text (pair.second.node)));
                       });
      }
      ImGui::SameLine (0.0f, 10.0f);
      if (ImGui::Button ("Choices"))
      {
        Log::printHeaderToLog ("In Memory: Choices");
        Utils::xml_print_node (missionx::data_manager::xmlChoices);
      }

      ImGui::Separator ();
      if (ImGui::Button ("Inventories"))
      {
        Log::printHeaderToLog ("In Memory: Inventories");
        std::for_each (missionx::data_manager::mapInventories.begin (), missionx::data_manager::mapInventories.end (), [] (std::pair<const std::string, missionx::Inventory> &pair) { Log::logMsg (fmt::format ("Inventory Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text (pair.second.node))); });
      }
      ImGui::SameLine (0.0f, 10.0f);
      if (ImGui::Button ("3D Objects Templates"))
      {
        Log::printHeaderToLog ("In Memory: 3D Object Templates");
        std::for_each (missionx::data_manager::st_map3d_obj.begin (), missionx::data_manager::st_map3d_obj.end (), [] (std::pair<const std::string, missionx::obj3d> &pair) { Log::logMsg (fmt::format ("3D Object Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text (pair.second.node))); });
      }
      ImGui::SameLine (0.0f, 10.0f);
      if (ImGui::Button ("Instanced 3D Objects"))
      {
        Log::printHeaderToLog ("In Memory: Instanced 3D Objects");
        std::for_each (missionx::data_manager::map3dInstances.begin (), missionx::data_manager::map3dInstances.end (), [] (std::pair<const std::string, missionx::obj3d> &pair) { Log::logMsg (fmt::format ("3D Instance Name: \"{}\"\n{}", pair.first, Utils::xml_get_node_content_as_text (pair.second.node))); });
      }

      mxUiReleaseLastFont (); // v3.305.1

      ImGui::Separator (); // v3.305.1
      ImGui::NewLine (); // v3.305.1

      // // v25.10.1
      mx_img_window::HelpMarker ("Force displaying 2D Map Cues in release mode.");
      ImGui::SameLine ();
      #ifndef RELEASE
      missionx::data_manager::flag_setupUseDraw2DMapInReleaseMode = true; // force value
      bool b_disable_force_2d_map_cues = mxStartUiDisableState (true);
      #endif
      ImGui::Checkbox ("Force 2D map cues", &missionx::data_manager::flag_setupUseDraw2DMapInReleaseMode);
      #ifndef RELEASE
      mxEndUiDisableState (b_disable_force_2d_map_cues);
      #endif

    }

    mxUiReleaseLastFont (); // v3.305.1
  }
  ImGui::EndGroup (); // end setting UI groups

  ImGui::EndChild ();

}

// ------------------------------
} // missionx