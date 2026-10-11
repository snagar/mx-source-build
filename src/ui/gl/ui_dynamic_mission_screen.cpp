//
// Created by saar.nagar on 2026-10-08.
//

#include "ui_dynamic_mission_screen.h"

#include "IconsFontAwesome5.h"

namespace missionx {


 ui_dynamic_mission_screen::~ui_dynamic_mission_screen(){}


// -------------------------------------

void ui_dynamic_mission_screen::add_ui_semi_act_phase_1_pick()
{
  constexpr static auto TABLE_COLUMNS        = 4;
  static constexpr auto VEC2_BTN_SIZE        = ImVec2 (160.0f, 120.0f);

  if (ImGui::BeginTable ("Semi Actions Table Based On Plane Type", TABLE_COLUMNS))
  {
    int column_index_i       = 0;

    // loop over list. every time mod TABLE_COLUMN == 0 we start a new column.
    for (const auto & btn_info : list_semi_auto_activities)
    {
      // decide if we need another row
      if (column_index_i%TABLE_COLUMNS == 0)
      {
        ImGui::TableNextRow();
      }

      ImGui::TableNextColumn();
      {
        ImGui::BeginGroup();
        {
          // Display big ICON image
          // if (ui::imgui::ImageButton (btn_info.imgName.c_str(), data_manager::mapCachedPluginTextures[btn_info.imgName].gTexture, VEC2_BTN_SIZE))
          if ( missionx::ui::wrap_imgui::ImageButton (btn_info.imgName.c_str(), data_manager::mapCachedPluginTextures[btn_info.imgName].gTexture, VEC2_BTN_SIZE))
          {
            missionx::strct_user_create_layer.user_semi_act_picked = btn_info;
            missionx::strct_user_create_layer.act_phase_enum = mx_act_phase_enum::phase_accept;

            // v26.09.1 Gather shared activity data, this includes "activity type" to the map_llm_requests_messages[].
            gather_semi_act_phase1_data_based_on_picked_activity(missionx::strct_user_create_layer, true);

          }
          if (!btn_info.tip.empty ())
            this->mx_add_tooltip(missionx::color::color_vec4_yellow, btn_info.tip);

          mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_MED ()); // set font size
          ImGui::TextColored(missionx::color::color_vec4_springgreen, "%s", btn_info.label.c_str());
          mxUiReleaseLastFont();

          ImGui::Separator();
          ImGui::Spacing();

        } // end group
        ImGui::EndGroup();
      }
      // End table column

      // increase column counter
      ++column_index_i;
    }
    ImGui::EndTable ();
  } // end table



}

// -------------------------------------

void ui_dynamic_mission_screen::add_ui_semi_act_phase_2_detail()
{
  static constexpr auto VEC2_IMAGE_BTN_SIZE = ImVec2(160.0f, 120.0f);
  static constexpr auto VEC2_MOD_CHILD      = ImVec2(0.0f, 150.0f);
  static constexpr auto VEC2_BACK_BTN_SIZE  = ImVec2(32.0f, 32.0f);

  const auto win_width_f = mxUiGetContentWidth();

  // Initialize basic flags
  const bool b_plane_is_helos                = strct_user_create_layer.user_semi_act_picked.activity < missionx::enums::mx_semi_activities_enum::act_props;
  const bool b_plane_is_airline_type         = strct_user_create_layer.user_semi_act_picked.activity > missionx::enums::mx_semi_activities_enum::act_jets;
  const bool bPickedMedevacMission           = (strct_user_create_layer.user_semi_act_picked.activity == missionx::enums::mx_semi_activities_enum::act_helos_medevac_accident || strct_user_create_layer.user_semi_act_picked.activity == missionx::enums::mx_semi_activities_enum::act_helos_medevac_surprise_me);
  const bool bPickedOilRigMission            = (strct_user_create_layer.user_semi_act_picked.activity == missionx::enums::mx_semi_activities_enum::act_helos_cargo_oilrig);
  const bool bPickedGAMission                = (strct_user_create_layer.user_semi_act_picked.activity == missionx::enums::mx_semi_activities_enum::act_props || strct_user_create_layer.user_semi_act_picked.activity == missionx::enums::mx_semi_activities_enum::act_props_float);
  const bool bPickedMedevacSurpriseMeMission = (strct_user_create_layer.user_semi_act_picked.activity == missionx::enums::mx_semi_activities_enum::act_helos_medevac_surprise_me);

  ImGui::BeginGroup();
  {
    // display back button
    mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
    if (mx_img_window::ButtonTooltip(mxUtils::from_u8string(ICON_FA_REPLY).append("##BackButtonInSetup").c_str(), "Back", IM_COL32(255, 255, 0, 255), IM_COL32(0, 0, 0, 255), VEC2_BACK_BTN_SIZE)) //
    {
      missionx::strct_user_create_layer.act_phase_enum = mx_act_phase_enum::phase_pick;
      missionx::strct_user_create_layer.user_semi_act_picked.reset();
    }
    mxUiReleaseLastFont();

    // title
    ImGui::SameLine(0.0f, VEC2_IMAGE_BTN_SIZE.x - VEC2_BACK_BTN_SIZE.x + 10.0f);
    mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_MED());
    ImGui::TextColored(missionx::color::color_vec4_yellow, "%s", "Description");
    mxUiReleaseLastFont();

    const auto activity_does_not_have_description = (!bPickedOilRigMission && !bPickedGAMission && !bPickedMedevacSurpriseMeMission);
    constexpr auto table_flags                    = ImGuiTableFlags_SizingStretchProp;

    ImGui::BeginTable("mission_description#table_phase2_detail", 2, table_flags);
    {
      ImGui::TableNextRow();
      // Show image
      ImGui::TableNextColumn();
      {
        //  Display button image
        ui::wrap_imgui::Image(data_manager::mapCachedPluginTextures[missionx::strct_user_create_layer.user_semi_act_picked.imgName].gTexture, VEC2_IMAGE_BTN_SIZE);
      }
      ImGui::TableNextColumn();
      {
        mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG()); // set font size
          ImGui::Text("%s", missionx::strct_user_create_layer.user_semi_act_picked.desc.c_str());
        mxUiReleaseLastFont();

        // Display special message for SurpriseMe missions
        if (bPickedMedevacSurpriseMeMission)
        {
          ImGui::Spacing();
          this->add_ui_medevac_surprise_me_warning();
        }
        else
          this->add_ui_mission_description(missionx::strct_user_create_layer.user_semi_act_picked.random_description);

      }
      ImGui::EndTable();
    } // endTable block
  } // end group
  ImGui::EndGroup();
  ImGui::Separator();

  // ------------------------
  // Modification options
  // ------------------------
  mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_REG()); // set font size
  ImGui::BeginGroup();
  {
    ImGui::BeginChild("Modify Semi-Automation Options", VEC2_MOD_CHILD, ImGuiChildFlags_None);
    {
      ImGui::Spacing();
      // ------------------------
      // Show AI options
      // ------------------------
      auto  llm_enum_options = enums::llm_ui_options_enum::llm_background_story;
      if (!bPickedOilRigMission && !bPickedMedevacMission)
      {
        if (!b_plane_is_airline_type)
          llm_enum_options = llm_enum_options | enums::llm_ui_options_enum::llm_suggest_payloads;

        llm_enum_options = llm_enum_options | enums::llm_ui_options_enum::llm_suggest_targets;
      }
      if (missionx::strct_setup_layer.b_use_ai)
      {
        add_ui_llm_options(llm_enum_options);
      }

      // - v26.08.1 -------------
      // Show briefer description collapsing header
      // ------------------------
      add_briefer_description_post_mission_creation(!(missionx::data_manager::get_is_random_engine_running())
                                                    && missionx::data_manager::missionState < mx_mission_state_enum::mission_is_running);

      ImGui::Spacing();

      // Oil rig search area
      if (bPickedOilRigMission)
      {
        ImGui::Spacing();
        add_ui_oilrig_search_area_buttons();
        ImGui::Spacing();
        ImGui::Separator();
      }
      else if (!bPickedOilRigMission && !bPickedMedevacSurpriseMeMission)
      {
        ImGui::Checkbox ("Start from plane position", &missionx::strct_cross_layer_properties.flag_start_from_plane_position);

        if ( strct_user_create_layer.user_semi_act_picked.activity >= enums::mx_semi_activities_enum::act_props
             && strct_user_create_layer.user_semi_act_picked.activity <= enums::mx_semi_activities_enum::act_turboprops )
        {
          ImGui::SameLine(0.0f, 10.0f);
          add_ui_is_amphibian();
          ImGui::SameLine(0.0f, 10.0f);
          add_ui_is_ga_cross_country_checkbox();
        }

        ImGui::Spacing ();
      }

      missionx::strct_user_create_layer.mapSemiOptionsHeaders[missionx::strct_user_create_layer.headerIndex].setState((ImGui::CollapsingHeader(missionx::strct_user_create_layer.mapSemiOptionsHeaders[missionx::strct_user_create_layer.headerIndex].title.c_str())));
      if (missionx::strct_user_create_layer.mapSemiOptionsHeaders[missionx::strct_user_create_layer.headerIndex].bState)
      {
        if (!bPickedOilRigMission && !bPickedMedevacSurpriseMeMission)
        {
          // ------------------------
          // -- Restrict max distance
          // ------------------------
          // sliders
          // We will use the "strct_user_create_layer.user_semi_act_picked.max_distance_slider" to hold the max distance. This parameter is shared with "option a"
          ImGui::TextColored ( missionx::color::color_vec4_yellow, "%s", "Pick Maximum Flight Leg Distance: ");
          if (ImGui::SliderFloat ("##tweak_max_distance", &strct_user_create_layer.user_semi_act_picked.max_distance_slider_f, missionx::strct_user_create_layer.user_semi_act_picked.distance_min_max.lowest_max, missionx::strct_user_create_layer.user_semi_act_picked.distance_min_max.max, "%.2f nm"))
          {
            // refresh description
            strct_user_create_layer.user_semi_act_picked.prepare_the_semi_activity_description_for_phase2(strct_user_create_layer.iNumberOfFlighLegs);
          }
        } // end oilrig and distance restriction

        // Customize the number of flight legs to generate.
        const bool bDisableRadioLegs = mx_img_window::mxStartUiDisableState(strct_user_create_layer.flag_cross_country);
        if (add_ui_pick_how_many_legs(strct_user_create_layer.iNumberOfFlighLegs, "How Many Flight Legs ? ", missionx::strct_user_create_layer.user_semi_act_picked.legs_min_max.min, missionx::strct_user_create_layer.user_semi_act_picked.legs_min_max.max) )
        {
          strct_user_create_layer.user_semi_act_picked.final_legs_no_to_generate = strct_user_create_layer.iNumberOfFlighLegs;
          strct_user_create_layer.user_semi_act_picked.prepare_the_semi_activity_description_for_phase2(strct_user_create_layer.iNumberOfFlighLegs);
        }
        mx_img_window::mxEndUiDisableState(bDisableRadioLegs);

      }

      ImGui::Spacing();

      // ------------------------------
      // Add other settings: header
      // ------------------------------
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, missionx::color::color_vec4_azure);
      ImGui::PushStyleColor (ImGuiCol_Header, missionx::color::color_vec4_coral);
      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_navy); // Text
      add_other_settings_header(b_plane_is_helos, bPickedMedevacMission, bPickedOilRigMission);
      ImGui::PopStyleColor(3);

    }
    ImGui::EndChild();
  }
  ImGui::EndGroup();
  mxUiReleaseLastFont();

  ImGui::Separator();
  ImGui::Spacing();


  // --------------------------
  // Add bottom buttons
  // --------------------------
  ImGui::BeginGroup();
  {
    // Add Advance Configuration Button
    ImGui::NewLine();
    ImGui::SameLine(win_width_f - 230.0f);
    // v26.08.1
    this->add_ui_advance_settings_random_date_time_weather_and_weight_button (mxconst::get_TEXT_TYPE_TITLE_REG ());

    // Add [generate] button
    ImGui::SameLine(10.0f);
    mxUiSetFont(mxconst::get_TEXT_TYPE_TITLE_REG());
    {

      if (missionx::data_manager::get_is_random_engine_running() && data_manager::strct_ui_share_data.user_message_line1.empty())
      {
        this->set_bottom_message_line1("Random Engine is running, please wait...", 10);
      }
      else if (missionx::data_manager::flag_apt_dat_optimization_is_running && data_manager::strct_ui_share_data.user_message_line1.empty())
      {
        this->set_bottom_message_line1("Can't Generate mission, apt data optimization is currently running. Please wait for it to finish first !!!", 5);
      }
      else if (!(missionx::data_manager::get_is_random_engine_running() || missionx::data_manager::flag_apt_dat_optimization_is_running))
      {
        // Regenerate Random Date Time // v3.303.10
        static bool bRerunRandomDateTime{false};
        bRerunRandomDateTime = add_ui_checkbox_rerun_random_date_and_time();
        ImGui::SameLine();

        missionx::flag_generatedRandomFile_success = false;

        // -----------------------
        // DISPLAY GENERATE BUTTON
        // -----------------------

        if (add_ui_generate_button())
        {
          // generate random date/time ?
          if (bRerunRandomDateTime) // v3.303.10
            this->execAction(mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);

          // set template
          missionx::strct_generate_template_layer.selectedTemplateKey = mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI();

          // ---------------------
          // Prepare layer data to be compatible with the -
          // "action_prepare_dynamic_mission_properties_and_call_generate_action()" function properties
          // ---------------------

          // type of mission
          if (mxUtils::mx_between<missionx::enums::mx_semi_activities_enum>(missionx::strct_user_create_layer.user_semi_act_picked.activity, missionx::enums::mx_semi_activities_enum::act_none, missionx::enums::mx_semi_activities_enum::act_helos_medevac_oilrig, missionx::enums::mx_between_types::gt_min_eqles_max))
            strct_user_create_layer.iRadioMissionTypePicked = 2; // oilrig
          else if (missionx::strct_user_create_layer.user_semi_act_picked.activity < missionx::enums::mx_semi_activities_enum::act_props)
            strct_user_create_layer.iRadioMissionTypePicked = 0; // medevac
          else
            strct_user_create_layer.iRadioMissionTypePicked = 1; // cargo

          // subcategory
          switch (missionx::strct_user_create_layer.user_semi_act_picked.activity)
          {
          case missionx::enums::mx_semi_activities_enum::act_helos_medevac_accident:
            strct_user_create_layer.iMissionSubCategoryPicked = 1; // 0: any location, 1: accident, 2: surprise me
            break;
          case missionx::enums::mx_semi_activities_enum::act_helos_medevac_surprise_me:
            strct_user_create_layer.iMissionSubCategoryPicked = 2; // 0: any location, 1: accident, 2: surprise me
            break;
          case missionx::enums::mx_semi_activities_enum::act_helos_medevac_oilrig:
            strct_user_create_layer.iMissionSubCategoryPicked = 0; // 0: cargo, 1: medevac
            break;
          default:
            strct_user_create_layer.iMissionSubCategoryPicked = 0; // 0: cargo  <= planes, oilrig cargo mission
            break;

          } // end switch subcategory

          // v26.09.1 Reminder that in "add_ui_semi_act_phase_1_pick() we add the activity type to the map_llm_requests_messages[]
          // v26.09.2 LLM add background Story text
          if (data_manager::strct_ui_share_data.flag_llm_use_llm_to_generate_a_mission && data_manager::strct_ui_share_data.flag_llm_add_background_story)
          {
            switch (missionx::strct_user_create_layer.user_semi_act_picked.activity)
            {
            case missionx::enums::mx_semi_activities_enum::act_helos_medevac_accident:
              data_manager::strct_ui_share_data.map_llm_requests_messages[missionx::llm_category::background_story.data()] = "Write a realistic, short background story about an accident that requires us to fly our helicopter to conduct a medevac evacuation.";
              break;
            case missionx::enums::mx_semi_activities_enum::act_helos_medevac_surprise_me:
              data_manager::strct_ui_share_data.map_llm_requests_messages[missionx::llm_category::background_story.data()] = "Write a realistic but generic, short background story about a medevac evacuation that needs helicopter evacuation. Unfortunately we don't know if it is car or wilderness evacuation";
              break;
            case missionx::enums::mx_semi_activities_enum::act_helos_medevac_oilrig:
              data_manager::strct_ui_share_data.map_llm_requests_messages[missionx::llm_category::background_story.data()] = "Write a realistic, short background story about Oilrig evacuation. Do not exaggerate in details, make it simple.";
              break;
            case missionx::enums::mx_semi_activities_enum::act_helos_cargo_oilrig:
              data_manager::strct_ui_share_data.map_llm_requests_messages[missionx::llm_category::background_story.data()] = "Write a realistic, short background story about Oilrig cargo mission. Do not exaggerate details, make it simple.";
              break;
            case missionx::enums::mx_semi_activities_enum::act_jets:
              data_manager::strct_ui_share_data.map_llm_requests_messages[missionx::llm_category::background_story.data()] = "Write a realistic and concise pre-flight background story for the upcoming jet flight based on the waypoints and type of plane.";
              break;
            case missionx::enums::mx_semi_activities_enum::act_props:
            case missionx::enums::mx_semi_activities_enum::act_props_float:
              data_manager::strct_ui_share_data.map_llm_requests_messages[missionx::llm_category::background_story.data()] = "Write a realistic and concise pre-flight for general aviation flight. Remember that they are mainly related to recreational activities, work or family activity. The background story should be light"
              "\nIf the number of waypoints is more than one or the distances are short (less than 100 nautical miles), then treat it like recreational or short hop flight."
              "\nIf there is only one waypoint and the distance is larger than 100 nautical miles, treat it as a cross country flight, so it could be a work, family, maintenance or even a trip flight. Something along those lines."
              ;
              break;
            default:
              data_manager::strct_ui_share_data.map_llm_requests_messages[missionx::llm_category::background_story.data()] = "Do not write a background story for this kind of flight. Keep it realistic and professional. Concentrate on the pre-flight activities and not on a story.";
              break;
            } // end switch subcategory

            data_manager::strct_ui_share_data.map_llm_requests_messages[missionx::llm_category::background_story.data()] +=
                    "\nTry to explain the reason for the flight and give the pilot a believable operational context, such as transporting passengers, medical evacuation, delivering cargo, positioning the aircraft, or completing a charter flight."
                    "\nKeep the story grounded in realistic aviation operations. Do not invent flight-plan details, procedures, weather, aircraft malfunctions, or other information that has not been provided."
                    "\nWrite 2-4 sentences in a professional but engaging tone. The result should feel like a real-world flight assignment rather than an adventure story."
            ;

          }


          // set plane type
          strct_user_create_layer.iRadioPlaneType = missionx::strct_user_create_layer.user_semi_act_picked.plane_type;

          // set min/max distances
          const auto [min, low_max, max] = missionx::strct_user_create_layer.user_semi_act_picked.get_mission_area();
          this->action_prepare_dynamic_mission_properties_and_call_generate_action(min, max, missionx::strct_cross_layer_properties.flag_start_from_plane_position);

        } // end [generate button]

      }
      else if (missionx::data_manager::random_thread_state.flagIsActive)
      {
        // -----------------------
        // DISPLAY [ABORT]
        // -----------------------
        mx_img_window::HelpMarker("Abort the background process.");
        ImGui::SameLine(0.0f, 5.0f);
        this->add_ui_abort_mission_creation_button(mx_window_actions::ACTION_ABORT_RANDOM_ENGINE_RUN); // Add Abort Random Engine
      }

      if (mxUtils::mx_between(data_manager::missionState, mx_mission_state_enum::mission_loaded_from_the_original_file, mx_mission_state_enum::mission_is_running, enums::mx_between_types::eqgt_min_less_max)
        && !missionx::data_manager::get_is_random_engine_running())
      {
        // -----------------------
        // DISPLAY [START] & [WARNING]
        // -----------------------
        ImGui::SameLine((mxUiGetContentWidth() * 0.40f) - (ImGui::CalcTextSize(missionx::LBL_START_MISSION.c_str()).x * 0.5f));
        this->add_ui_start_mission_button(mx_window_actions::ACTION_START_RANDOM_MISSION);

        this->add_ui_warning_messages_button(); // v26.1.1. Also change the "start" button position to clear space for the warning button.
      }

    }
    mxUiReleaseLastFont();

  } // end footer button group
  ImGui::EndGroup();

}

// -------------------------------------

void ui_dynamic_mission_screen::draw_dynamic_mission_creation_screen_child_1()
{
  const auto win_size_vec2 = mxUiGetWindowContentWxH ();

  if (missionx::strct_user_create_layer.layer_state == missionx::mx_layer_state_enum::success_can_draw)
  {
    static constexpr float img_ps_f = 0.5f;
    float                  pos_x    = data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_LAB_24X18 ()].sImageData.getW_f () * img_ps_f;

    mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());

    ImGui::BeginChild ("draw_user_create_mission_layer_01", ImVec2 (0.0f, win_size_vec2.y * 0.77f), ImGuiChildFlags_Borders);
    {
      ///////// Image ////////
      ImGui::BeginGroup ();
      ui::wrap_imgui::Image (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_LAB_24X18 ()].gTexture, ImVec2 (data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_LAB_24X18 ()].sImageData.getW_f () * img_ps_f, data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_LAB_24X18 ()].sImageData.getH_f () * img_ps_f));
      ImGui::EndGroup ();

      pos_x += 50.0f;
      ImGui::SameLine (pos_x);
      ImGui::BeginGroup ();



      // v26.09.2 --------------------------------------
      //                  Topic 0 - Briefer description
      // Show briefer description collapsing header
      // ------------------------
      add_briefer_description_post_mission_creation(!(missionx::data_manager::get_is_random_engine_running()) && missionx::data_manager::missionState < mx_mission_state_enum::mission_is_running);


      //------------------------------------------------
      //                  Topic 1 - Header
      //------------------------------------------------

      // label
      // v3.0.253.9 explanation
      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellowgreen); //
      HelpMarker ("For best results - 'run APT.dat optimization' at least once or after XP update or a new scenery is added.\nYou can find it in the SETUP screen.");
      ImGui::PopStyleColor (1);
      ImGui::SameLine (0.0f, 2.0f);

      ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", " Pick Type of Mission:");

      ImGui::SameLine (0.0f, 50.0f);
      this->add_ui_advance_settings_random_date_time_weather_and_weight_button (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.10 convert the random dateTime button to a self contain function

      if (ImGui::RadioButton ("Medevac", missionx::strct_user_create_layer.iRadioMissionTypePicked == static_cast<int> (missionx::mx_ui_mission_type::medevac)))
      {
        missionx::strct_user_create_layer.iRadioMissionTypePicked   = static_cast<int> (missionx::mx_ui_mission_type::medevac);
        missionx::strct_user_create_layer.iMissionSubCategoryPicked = 0; // reset sub category combo
        missionx::strct_user_create_layer.flag_use_web_osm          = true;
        missionx::strct_user_create_layer.iRadioPlaneType           = missionx::mx_plane_types_enum::plane_type_helos;

        this->refresh_slider_data_based_on_plane_type (missionx::strct_user_create_layer.iRadioPlaneType); // v3.303.14
      }
      ImGui::SameLine ();
      if (ImGui::RadioButton ("Cargo/Passengers", missionx::strct_user_create_layer.iRadioMissionTypePicked == static_cast<int> (missionx::mx_ui_mission_type::cargo)))
      {
        missionx::strct_user_create_layer.iRadioMissionTypePicked   = static_cast<int> (missionx::mx_ui_mission_type::cargo);
        missionx::strct_user_create_layer.iMissionSubCategoryPicked = 0; // reset sub category combo
        missionx::strct_user_create_layer.flag_use_web_osm          = false;
      }

      ImGui::SameLine ();
      if (ImGui::RadioButton ("Oil Rig", missionx::strct_user_create_layer.iRadioMissionTypePicked == static_cast<int> (missionx::mx_ui_mission_type::oil_rig)))
      {
        missionx::strct_user_create_layer.iRadioMissionTypePicked   = static_cast<int> (missionx::mx_ui_mission_type::oil_rig);
        missionx::strct_user_create_layer.iMissionSubCategoryPicked = 0; // reset sub category combo
        missionx::strct_user_create_layer.flag_use_web_osm          = false;
        missionx::strct_user_create_layer.iRadioPlaneType           = missionx::mx_plane_types_enum::plane_type_helos;

        this->refresh_slider_data_based_on_plane_type (missionx::strct_user_create_layer.iRadioPlaneType); // v3.303.14
      }
      ImGui::SameLine ();
      mx_img_window::HelpMarker ("The plugin will place you randomly in a location near an Oil Rig.\nThe Oil Rig data is highly dependent on the Oil Rig information found in the apt.dat files, and not on what you see in your maps.");
      // sub categories
      {
        const auto vecToDisplay = missionx::mapMissionCategories[missionx::strct_user_create_layer.iRadioMissionTypePicked];
        this->add_ui_pick_subcategories (vecToDisplay); // v25.04.1
      }
      const bool bPickedOilRigMission            = missionx::strct_user_create_layer.iRadioMissionTypePicked == static_cast<int> (missionx::mx_ui_mission_type::oil_rig);
      const bool bPickedMedevacMission           = missionx::strct_user_create_layer.iRadioMissionTypePicked == static_cast<int> (mx_ui_mission_type::medevac);
      const bool bPickedMedevacSurpriseMeMission = (bPickedMedevacMission && missionx::strct_user_create_layer.iMissionSubCategoryPicked == 2);


      if (bPickedMedevacSurpriseMeMission)
      {
        add_ui_medevac_surprise_me_warning();
      }


      ImGui::Separator ();
      ImGui::NewLine ();

      const auto lmbda_get_title = [] (int &inMissionType)
      {
        switch (static_cast<missionx::mx_ui_mission_type> (inMissionType))
        {
          case missionx::mx_ui_mission_type::medevac:
            return "Medevac Settings";
            break;
          case missionx::mx_ui_mission_type::cargo:
            return "Cargo Settings";
            break;
          case missionx::mx_ui_mission_type::oil_rig:
            return "Oil Rig Settings";
            break;
          default:
            break;
        }

        return "";
      };
      const std::string Topic2_title = lmbda_get_title (missionx::strct_user_create_layer.iRadioMissionTypePicked);

      //------------------------------------------------
      //                  Topic 2 - plane type and area filter
      //------------------------------------------------

      // ImGui::PushStyleColor(ImGuiCol_Header, missionx::color::color_vec4_beige);
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, missionx::color::color_vec4_azure);
      ImGui::PushStyleColor (ImGuiCol_Header, missionx::color::color_vec4_coral);
      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_navy); // Text

      if (ImGui::CollapsingHeader (Topic2_title.c_str (), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // dynamic title
      {
        // ImGui::GetStateStorage()->SetInt(ImGui::GetID("Other Settings"), 0);     // collapse "Other Settings"

        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_white); // internal color

        // ------------------------
        // -- Skewed 3D Markers
        // ------------------------
        if (bPickedMedevacMission) // v3.0.253.6
        {
          add_ui_skewed_marker_checkbox ();
          ImGui::NewLine ();
        }

        // ------------------------
        // -- Preferred Plane
        // ------------------------

        mxStartUiDisableState (bPickedMedevacSurpriseMeMission); // v25.06.1 start disable/enable

        HelpMarker ("The aircraft you select will help filter the route distance and the available ramps at the destination airport.\n\nExample:\n\nJets: Small to medium Jets (B,C)\nAirline/Cargo: E190, B737, A320 (C,D)\nHeavy Airline/Cargo: B777, A350 (C,D,E,F)");
        ImGui::SameLine (0.0f, 2.0f);
        ImGui::TextColored  ( missionx::color::color_vec4_yellow, "Pick The Plane Type You Will Fly:");

        int radio_btn_counter = 0;
        for (const auto &planeTypeLabel : missionx::mapListPlaneRadioLabel | std::views::values) // v24.12.1
        {
          // Filter out all plane types that are not Helos for Medevac and Oil Rig missions
          if (missionx::strct_user_create_layer.iRadioMissionTypePicked != static_cast<int> (missionx::mx_ui_mission_type::cargo) && planeTypeLabel.type != mx_plane_types_enum::plane_type_helos)
            continue;

          if (ImGui::RadioButton (planeTypeLabel.label.c_str (), missionx::strct_user_create_layer.iRadioPlaneType == planeTypeLabel.type))
          {
            missionx::strct_user_create_layer.iRadioPlaneType = planeTypeLabel.type;
            this->refresh_slider_data_based_on_plane_type (missionx::strct_user_create_layer.iRadioPlaneType); // v24.12.1 deprecated
          }
          radio_btn_counter++;
          if (radio_btn_counter%5 == 0)
            ImGui::NewLine();

          ImGui::SameLine (0.0f);
        }

        //// v3.0.253.11 added the PROP_START_FROM_PLANE_POSITION checkbox

        // ------------------------
        // -- Helos only option to narrow ramp type
        // ------------------------
        if (missionx::strct_user_create_layer.iRadioPlaneType == missionx::mx_plane_types_enum::plane_type_helos)
        {
          ImGui::NewLine ();
          ImGui::Checkbox ("Narrow helos ramp locations", &strct_user_create_layer.flag_narrow_helos_filtering);
          ImGui::NewLine ();
        }
        else
        {
          // ------------------------
          // -- Pick Runway Type
          // ------------------------
          auto count_filters_picked = static_cast<size_t> (0);
          // Runway filter
          ImGui::NewLine ();

          // Filter out type of runway for heavies type of plane.
          if (missionx::strct_user_create_layer.iRadioPlaneType < missionx::mx_plane_types_enum::plane_type_airline )
          {
            ImGui::Checkbox ("Pick Any Runway##filterRunways", &missionx::strct_user_create_layer.flag_pick_any_rw);
            // display the filter tree if changed to false
            if (missionx::strct_user_create_layer.flag_pick_any_rw == false)
            {
              ImGui::Text ("Include airports that have:");

              for (auto &[rw_lbl, b_picked] : missionx::strct_user_create_layer.map_filter_runways)
              {
                ImGui::SameLine (0.0f, 10.0f); //
                ImGui::Checkbox (rw_lbl.c_str (), &b_picked);
                if (b_picked) // if picked
                  count_filters_picked++;
              }

              if (count_filters_picked == missionx::strct_user_create_layer.map_filter_runways.size ())
                missionx::strct_user_create_layer.reset_filter_runways_flags ();
            }

          } // end runway types

          // v26.04.4
          if (missionx::strct_user_create_layer.iRadioPlaneType <= missionx::mx_plane_types_enum::plane_type_turboprops )
          {
            // ImGui::Checkbox ("Is Amphibian##filterRamps", &missionx::strct_user_create_layer.flag_plane_is_amphibian);
            add_ui_is_amphibian();
          }

          ImGui::NewLine ();
        }


        mxEndUiDisableState ( bPickedMedevacSurpriseMeMission ); // v25.06.1 end disable/enable
        if (bPickedMedevacSurpriseMeMission)
          ImGui::TextColored (missionx::color::color_vec4_yellow, "Inventory will be XP11 compatible - No stations.");

        //------------------------------------------------
        //           Row 3 - sliders
        //------------------------------------------------
        const static std::string popupOverpassWindowName = "overpass filter popup";


        if (bPickedOilRigMission)
        {
          add_ui_oilrig_search_area_buttons(); // v26.04.1
        }

        if (!bPickedOilRigMission && !bPickedMedevacSurpriseMeMission ) // Display for missions that are NOT Oil Rig nor "Surprise Me type".
        {
          ImGui::Columns (2);
          {
            ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
            ImGui::Text ("Pick Minimum Flight Leg Distance: ");
            ImGui::PopStyleColor (1);
            if (ImGui::SliderFloat (strct_user_create_layer.dyn_slider1_lbl.c_str (), &strct_user_create_layer.dyn_sliderVal1, missionx::mapListPlaneRadioLabel[missionx::strct_user_create_layer.iRadioPlaneType].from_slider_min, missionx::mapListPlaneRadioLabel[missionx::strct_user_create_layer.iRadioPlaneType].from_slider_max, "%.2f nm"))
            {
              if (strct_user_create_layer.dyn_sliderVal2 <= strct_user_create_layer.dyn_sliderVal1)
                strct_user_create_layer.dyn_sliderVal2 = strct_user_create_layer.dyn_sliderVal1 + 100.0f;

              this->validate_sliders_values (missionx::strct_user_create_layer.iRadioPlaneType);
            }

            ImGui::NextColumn ();
            {
              ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
              ImGui::Text ("Pick Maximum Flight Leg Distance: ");
              ImGui::PopStyleColor (1);
              if (ImGui::SliderFloat (strct_user_create_layer.dyn_slider2_lbl.c_str (), &strct_user_create_layer.dyn_sliderVal2, missionx::mapListPlaneRadioLabel[missionx::strct_user_create_layer.iRadioPlaneType].to_slider_min, missionx::mapListPlaneRadioLabel[missionx::strct_user_create_layer.iRadioPlaneType].to_slider_max, "%.2f nm"))
              {
                if (strct_user_create_layer.dyn_sliderVal1 > strct_user_create_layer.dyn_sliderVal2)
                  strct_user_create_layer.dyn_sliderVal1 = strct_user_create_layer.dyn_sliderVal2 - 100.0f;

                this->validate_sliders_values (missionx::strct_user_create_layer.iRadioPlaneType);
              }
            }
          }
          ImGui::Columns ();


          //if (strct_user_create_layer.iRadioMissionTypePicked == static_cast<int> (mx_mission_type::medevac) && (missionx::strct_user_create_layer.iMissionSubCategoryPicked > 0)) // > 0 = med_any_location
          if (bPickedMedevacMission && (missionx::strct_user_create_layer.iMissionSubCategoryPicked > 0)) // > 0 = med_any_location
          {
            // display OSM WEB and OSM DB only if it is not cargo type

            ImGui::NewLine ();

            strct_user_create_layer.flag_use_web_osm = true;
            ImGui::PushItemFlag (ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar (ImGuiStyleVar_Alpha, ImGui::GetStyle ().Alpha * 0.5f);
            ImGui::Checkbox ("Use web OSM", &strct_user_create_layer.flag_use_web_osm);
            ImGui::PopItemFlag ();
            ImGui::PopStyleVar ();

            this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Prefer search from the web over local OSM database search.");
            if (strct_user_create_layer.flag_use_web_osm)
            {
              if (missionx::data_manager::get_is_random_engine_running()) // v3.0.255.4 fix bug where user exit the popup window and it crashes x-plane during mission creation in the background
              {
                ImGui::PushItemFlag (ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar (ImGuiStyleVar_Alpha, ImGui::GetStyle ().Alpha * 0.5f);
              }

              ImGui::SameLine (0.0f, 10.0f);
              if (ImGui::Button ("filter ..."))
              {
                ImGui::OpenPopup (popupOverpassWindowName.c_str ());
              }
              this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Advance options to filter the data fetched");

              if (missionx::data_manager::get_is_random_engine_running()) // v3.0.255.4 fix bug where user exit the popup window and it crashes x-plane during mission creation in the background
              {
                ImGui::PopItemFlag ();
                ImGui::PopStyleVar ();
              }
            }

            ImGui::SameLine (0.0f, 150.0f);

            ImGui::Checkbox ("Use local OSM db file", &strct_user_create_layer.flag_use_osm);
            this->mx_add_tooltip (missionx::color::color_vec4_yellow, "If OSM database is available, then try to get data from it before the Web OSM.");

            //// OSM Web filter popup
            // auto vec2WindowSize = mxUiGetWindowContentWxH();
            ImGui::SetNextWindowSize (ImVec2 (590.0f, 230.0f));
            ImGui::PushStyleColor (ImGuiCol_ChildBg, missionx::color::color_vec4_black);
            {
              if (ImGui::BeginPopupModal (popupOverpassWindowName.c_str (), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
              {
                constexpr static const int OSM_BUFF_SIZE_I           = 2047;
                static bool                b_first_time              = true;
                static char                buf1[OSM_BUFF_SIZE_I + 1] = "";

                if (b_first_time) // reset main overpass filter
                {
                  // std::string err;
                  std::string stored_filter = Utils::getNodeText_type_6 (missionx::system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_FILTER (), mxconst::get_DEFAULT_OVERPASS_WAYS_FILTER ());
                  if (stored_filter.empty ())
                  {
                    stored_filter = missionx::strct_user_create_layer.overpass_original_filter;
                    Utils::xml_search_and_set_node_text (system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_FILTER (), stored_filter, this->mxcode.STRING, true); // "6" = string type
                    this->execAction (missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);
                  }

                  mxUtils::copy_string_to_buffer(stored_filter, buf1[0], sizeof(buf1));


                  missionx::strct_user_create_layer.overpass_main_filter = stored_filter;
                  b_first_time                                       = false;
                }

                ImGui::BeginGroup ();
                ImGui::BeginChild ("popupOverpassWindow##PopupOverpassWindowName");
                {
                  int iStyle = 0;
                  ImGui::BeginChild ("internal##PopupOverpassWindowName");

                  ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
                  iStyle++;
                  ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_Button, missionx::color::color_vec4_dodgerblue);
                  iStyle++;
                  ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_Text, missionx::color::color_vec4_white);
                  iStyle++;
                  ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_TextSelectedBg, missionx::color::color_vec4_black);
                  iStyle++;

                  ImGui::PushItemWidth (500.0f);
                  ImGui::TextColored (missionx::color::color_vec4_yellow, "Define informational filters to impact requested data from overpass site.\nFormat must be \"way[key=value]({{bbox}});\"  Fill in the key/value.\nFor space use \"%%20\" or \"+\": \";out%%20center;\"\n");
                  ImGui::PopItemWidth ();

                  ImGui::NewLine ();

                  /////////////////////
                  // main filter
                  ////////////////////
                  ImGui::TextColored (missionx::color::color_vec4_yellow, "Ways filter length:");
                  ImGui::SameLine ();
                  ImGui::TextColored (missionx::color::color_vec4_beige, "%i chars", static_cast<int> (std::string (buf1).length ()));
                  ImGui::SameLine (this->mxUiGetContentWidth () - 40.0f, 0.0f);
                  mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.14
                  if (ImGui::Button ("reset##resetButton1"))
                  {
                    mxUtils::copy_string_to_buffer(missionx::strct_user_create_layer.overpass_original_filter, buf1[0], sizeof(buf1)); // v26.04.3
                    missionx::strct_user_create_layer.overpass_pre_apply_filter_s = missionx::strct_user_create_layer.overpass_original_filter;
                  }
                  mxUiReleaseLastFont ();

                  ImGui::BeginChild ("overpassFilter##PopupOverpassWindowName", ImVec2 (0.0f, 55.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysHorizontalScrollbar); // InputTextWithHint

                  ImGui::PushItemWidth (static_cast<float> (OSM_BUFF_SIZE_I) + 1.0f);
                  ImGui::PushID ("##overpassCustomFilter");
                  if (ImGui::InputTextWithHint ("", "way['key'='value']({{bbox}});out;", buf1, OSM_BUFF_SIZE_I)) // , ImGuiInputTextFlags_CharsNoBlank)) // , ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal);
                  {
                    missionx::strct_user_create_layer.overpass_pre_apply_filter_s = std::string (buf1);
                  }
                  ImGui::PopID ();
                  ImGui::PopItemWidth ();

                  ImGui::EndChild ();


                  mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.14
                  ImGui::Separator ();

                  ImGui::SameLine (0.0f, 20.0f);
                  ImGui::SetItemDefaultFocus ();
                  if (ImGui::Button ("Apply", ImVec2 (150, 0)))
                  {
                    missionx::strct_user_create_layer.overpass_pre_apply_filter_s = std::string (buf1);
                    missionx::strct_user_create_layer.overpass_main_filter        = missionx::strct_user_create_layer.overpass_pre_apply_filter_s;
                    Utils::xml_search_and_set_node_text (system_actions::pluginSetupOptions.node, mxconst::get_OPT_OVERPASS_FILTER (), missionx::strct_user_create_layer.overpass_main_filter, this->mxcode.STRING, true); // "6" = string type


                    this->execAction (missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS);

                    ImGui::CloseCurrentPopup ();
                  }
                  ImGui::SameLine (0.0f, 20.0f);
                  ImGui::SetItemDefaultFocus ();
                  if (ImGui::Button ("Cancel", ImVec2 (90, 0)))
                  {
                    mxUtils::copy_string_to_buffer(missionx::strct_user_create_layer.overpass_main_filter, buf1[0], sizeof(buf1)); // v26.04.3

                    missionx::strct_user_create_layer.overpass_pre_apply_filter_s = missionx::strct_user_create_layer.overpass_main_filter;
                    ImGui::CloseCurrentPopup ();
                  }

                  mxUiReleaseLastFont ();

                  ImGui::PopStyleColor (iStyle); // pop out before EndPopup

                  ImGui::EndChild ();
                }

                ImGui::EndChild ();
                ImGui::EndGroup ();

                ImGui::EndPopup ();
              } // end quit popup
            }
            ImGui::PopStyleColor (); // child background
          } // end OSM checkboxes
          else
          {
            strct_user_create_layer.flag_use_web_osm = false;
          }
        } // end if not Oil Rig

        ImGui::PopStyleColor (1); // disable internal text color

        ImGui::NewLine ();

        // ------------------------
        // -- How Many Flight Legs ?
        // ------------------------
        int iMinLegsVal = 1;
        int iMaxLegsVal = 4;
        // calculate default number of flight legs per mission and plane type
        if ((strct_user_create_layer.iRadioPlaneType == missionx::mx_plane_types_enum::plane_type_helos && bPickedMedevacMission) || (bPickedOilRigMission))
        {
          iMinLegsVal                                = 2;
          iMaxLegsVal                                = 2;
          strct_user_create_layer.iNumberOfFlighLegs = iMinLegsVal; // we're automatically resetting the starting/picked legs to 2 and not 1
        }
        // v24.12.1 jets and heavies have only "one" flight leg.
        else if (strct_user_create_layer.iRadioPlaneType > missionx::mx_plane_types_enum::plane_type_turboprops && strct_user_create_layer.iRadioMissionTypePicked == static_cast<int> (missionx::mx_ui_mission_type::cargo))
        {
          iMinLegsVal                                = 1;
          iMaxLegsVal                                = 1;
          strct_user_create_layer.iNumberOfFlighLegs = iMinLegsVal; // we're automatically resetting the starting/picked legs to only one, for jets and heavies.
        }

        //if (missionx::strct_user_create_layer.iRadioMissionTypePicked != static_cast<int> (missionx::mx_mission_type::medevac) && missionx::strct_user_create_layer.iRadioMissionTypePicked != static_cast<int> (missionx::mx_mission_type::oil_rig) && strct_user_create_layer.iRadioPlaneType < missionx::mx_plane_types::plane_type_jets)
        if (!bPickedMedevacMission && !bPickedOilRigMission && !bPickedMedevacSurpriseMeMission && strct_user_create_layer.iRadioPlaneType < missionx::mx_plane_types_enum::plane_type_jets)
        {
          add_ui_pick_how_many_legs (strct_user_create_layer.iNumberOfFlighLegs, "How Many Flight Legs ? ", iMinLegsVal, iMaxLegsVal);

          ImGui::NewLine ();
        }

      } // End Collapse Topic 2 & 3

      //------------------------------------------------
      //     Topic 4 (basically the columns have few rows, but we count them as 1)
      //------------------------------------------------

      // v26.04.1
      const bool b_plane_is_helos = strct_user_create_layer.iRadioPlaneType == missionx::mx_plane_types_enum::plane_type_helos;
      add_other_settings_header(b_plane_is_helos, bPickedMedevacMission, bPickedOilRigMission);


      ImGui::PopStyleColor (3); // end Collapse Header special color - button hovered,button,text,text selected bg

      ImGui::NewLine ();
      ImGui::NewLine ();

      ImGui::EndGroup ();
    }
    ImGui::EndChild (); // end main dynamic creation area child

    mxUiReleaseLastFont ();

    // ------------------------
    // ------------------------
    // ------------------------
    //     Footer Buttons
    // ------------------------


    mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
    ImGui::BeginGroup ();
    {

      if (missionx::data_manager::get_is_random_engine_running() && data_manager::strct_ui_share_data.user_message_line1.empty())
      {
        this->set_bottom_message_line1 ("Random Engine is running, please wait...", 20);
      }
      else if (missionx::data_manager::flag_apt_dat_optimization_is_running && data_manager::strct_ui_share_data.user_message_line1.empty())
      {
        this->set_bottom_message_line1 ("Can't Generate mission, apt data optimization is currently running. Please wait for it to finish first !!!", 20);
      }
      else if (!(missionx::data_manager::get_is_random_engine_running() || missionx::data_manager::flag_apt_dat_optimization_is_running))
      {
        // Regenerate Random Date Time // v3.303.10
        static bool bRerunRandomDateTime{ false };
        bRerunRandomDateTime = add_ui_checkbox_rerun_random_date_and_time ();

        ImGui::SameLine ();

        missionx::flag_generatedRandomFile_success = false;

        // -----------------------
        // DISPLAY GENERATE BUTTON
        // -----------------------
        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
        ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_orange);
        ImGui::PushStyleColor (ImGuiCol_ButtonActive, missionx::color::color_vec4_azure);
        if (ImGui::Button (">> Generate Mission <<"))
        {
          if (bRerunRandomDateTime) // v3.303.10
            this->execAction (missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);

          missionx::strct_generate_template_layer.selectedTemplateKey = mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI ();

          this->action_prepare_dynamic_mission_properties_and_call_generate_action(strct_user_create_layer.dyn_sliderVal1, strct_user_create_layer.dyn_sliderVal2, true);

        }

        ImGui::PopStyleColor (3);
      }
      else if (missionx::data_manager::random_thread_state.flagIsActive)
      {

        mx_img_window::HelpMarker ("Abort the background process.");
        ImGui::SameLine (25.0f, 0.0f);
        this->add_ui_abort_mission_creation_button (mx_window_actions::ACTION_ABORT_RANDOM_ENGINE_RUN); // Add Abort Random Engine
      }

      //if (!data_manager::random_thread_state.flagIsActive || !missionx::data_manager::get_is_random_engine_running())
      if ( mxUtils::mx_between( data_manager::missionState, missionx::mx_mission_state_enum::mission_loaded_from_the_original_file, missionx::mx_mission_state_enum::mission_is_running, enums::mx_between_types::eqgt_min_less_max)
          && !missionx::data_manager::get_is_random_engine_running())
      {
        ImGui::SameLine ((mxUiGetContentWidth () * 0.40f) - (ImGui::CalcTextSize (missionx::LBL_START_MISSION.c_str ()).x * 0.5f));
        this->add_ui_start_mission_button (missionx::mx_window_actions::ACTION_START_RANDOM_MISSION);

        this->add_ui_warning_messages_button (); // v26.1.1. Also change the "start" button position to clear space for the warning button.
      }

      // // ImGui::SetWindowFontScale (mxconst::DEFAULT_BASE_FONT_SCALE);
    }
    ImGui::EndGroup ();
    mxUiReleaseLastFont ();


  } // end if (missionx::strct_user_create_layer.layer_state == missionx::mx_layer_state_enum::success_can_draw)
  else if (missionx::strct_user_create_layer.layer_state == missionx::mx_layer_state_enum::failed_data_is_not_present || missionx::strct_user_create_layer.layer_state == missionx::mx_layer_state_enum::fatal_database_is_not_initializing_correctly)
  {
    /// TODO: consider removing this part of the code, since we always force the creation of the SQLite DB in order to use the plugin features.
    ImGui::NewLine ();

    if (missionx::strct_user_create_layer.layer_state == missionx::mx_layer_state_enum::failed_data_is_not_present)
    {
      this->display_shared_message_when_optimized_data_is_not_present (missionx::mx_layer_state_enum::failed_data_is_not_present);
    }
    else
    {
      this->display_shared_message_when_optimized_data_is_not_present (missionx::mx_layer_state_enum::fatal_database_is_not_initializing_correctly);
    }
  }
  else // display wait
  {
    ImGui::NewLine ();

    mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_BIG ());
    ImGui::TextColored (missionx::color::color_vec4_magenta, "Please wait while testing validity of the data.... ");
    mxUiReleaseLastFont (); // v3.303.14 release the last 2 fonts we pushed


    if (missionx::strct_user_create_layer.layer_state < missionx::mx_layer_state_enum::validating_data)
    {
      missionx::strct_user_create_layer.layer_state = missionx::mx_layer_state_enum::validating_data;
    }
  }


} // draw_dynamic_mission_creation_screen


// -------------------------------------

void ui_dynamic_mission_screen::draw_dynamic_mission_creation_screen_child_2()
{
   // loop over all listMainBtn and display in buttons
   static constexpr float win_padding = 30.0f;

   ///////// Main Layer Buttons
   ImGui::BeginChild ("draw_semi_auto_screen", ImVec2 (0.0f, ImGui::GetWindowHeight () - missionx::fBottomToolbarPadding_f - missionx::fTopToolbarPadding_f - win_padding));
   {
     switch (missionx::strct_user_create_layer.act_phase_enum)
     {
     case mx_act_phase_enum::phase_pick:
     {
       this->add_ui_semi_act_phase_1_pick();
     }
     break;
     case mx_act_phase_enum::phase_accept:
     {
       this->add_ui_semi_act_phase_2_detail();
     }
     break;
     }

   }
   ImGui::EndChild(); // end draw_semi_auto_screen
}
// -------------------------------------
void ui_dynamic_mission_screen::draw_dynamic_mission_creation_screen()
{
   switch (missionx::strct_user_create_layer.child_screen)
   {
   case mx_user_create_mission_layer::mx_dynamic_fpln_screen::ext_option_a:
   {
     // reset the "semi-automated" mission creation, since we share parameters (like dynSlider)
     missionx::strct_user_create_layer.act_phase_enum = mx_act_phase_enum::phase_pick;
     missionx::strct_user_create_layer.user_semi_act_picked.reset();
     draw_dynamic_mission_creation_screen_child_1 ();
   }
   break;
   case mx_user_create_mission_layer::mx_dynamic_fpln_screen::ext_option_b:
   {
     draw_dynamic_mission_creation_screen_child_2();
   }
   break;
   default: // mx_ext_fpln_screen::ext_home
     ImGui::BeginChild ("User Creation Screen", ImVec2 (0.0f, -50.0f), ImGuiChildFlags_None);
     draw_dynamic_mission_creation_screen_home ();
     ImGui::EndChild ();
   }
}


// -------------------------------------
void ui_dynamic_mission_screen::draw_dynamic_mission_creation_screen_home()
{
  constexpr static auto btn_size_vec2 = ImVec2 (190.0f, 190.0f);
  const auto                  win_size_vec2 = mxUiGetWindowContentWxH ();

  ImGui::Spacing ();
  mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_BIG ());
  ImGui::TextWrapped ("%s", "Pick one of the options below.");
  ImGui::Separator ();
  mxUiReleaseLastFont ();


  mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
  ImGui::Spacing ();
  ImGui::Spacing ();
  ImGui::Spacing ();
  if (ImGui::BeginTable ("Dynamic Creation Options", 2))
  {
    ImGui::TableNextColumn ();
    {
      if (ui::wrap_imgui::ImageButton ("Full Control", data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_FULL_CONTROL ()].gTexture, btn_size_vec2))
      {
        // disable find target using llm
        data_manager::strct_ui_share_data.reset_llm();

        // v26.09.2 reset the semi activity screen
        missionx::strct_user_create_layer.act_phase_enum = mx_act_phase_enum::phase_accept;

        // flag we want to display Option A Screen
        missionx::strct_user_create_layer.child_screen = mx_user_create_mission_layer::mx_dynamic_fpln_screen::ext_option_a;
        this->refresh_slider_data_based_on_plane_type(missionx::strct_user_create_layer.iRadioPlaneType);
      } // end imageButton - flightplandb
      this->mx_add_tooltip (missionx::color::color_vec4_white, "Configure a random mission.");
      // bottom title
      ImGui::TextColored (missionx::color::color_vec4_yellowgreen, "%s", "Configure every aspect of the generated flight." );

    } // end full configuration button

    ImGui::TableNextColumn ();
    {
      if (ui::wrap_imgui::ImageButton ("Some Control", data_manager::mapCachedPluginTextures[mxconst::get_BITMAP_BTN_SOME_CONTROL()].gTexture, btn_size_vec2))
      {
        // flag we want to display Option B Screen
        missionx::strct_user_create_layer.child_screen = mx_user_create_mission_layer::mx_dynamic_fpln_screen::ext_option_b;
      }
      this->mx_add_tooltip (missionx::color::color_vec4_white, "Pick the type to generate.");
      // bottom title
      ImGui::TextColored (missionx::color::color_vec4_yellowgreen, "%s", "Pick the type of flight you would like to fly,\nwith minimal setup.");

    } // end Some Control
    ImGui::EndTable ();
  } // end table
  mxUiReleaseLastFont ();
}

// -------------------------------------

} // missionx