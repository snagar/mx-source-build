//
// Created by saar.nagar on 2026-10-07.
//

#include "ui_external_dbflightplan_screen.h"

#include "IconsFontAwesome5.h"
#include "../../io/system_actions.h"

namespace missionx {




 ui_external_dbflightplan_screen::~ui_external_dbflightplan_screen()
 {
 }

void ui_external_dbflightplan_screen::draw_child_ext_fpln_db_site_screen() {
  const auto                   win_size_vec2  = mxUiGetWindowContentWxH ();
  constexpr const static float ga_range_begin = 0.0f, ga_range_end = 9000.0f, ga_steps = 20.0f;
  constexpr const static char *limit_items[] = { "10", "20", "30", "40", "60", "80", "100" };
  constexpr const static char *sort_items[]  = { "created", "updated", "popularity", "distance" };

  // first time
  if (missionx::strct_ext_layer.flag_first_time)
  {
    missionx::strct_ext_layer.flag_first_time ^= 1;
    if (missionx::strct_ext_layer.flag_remove_duplicate_airport_names)
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_REMOVE_DUPLICATE_ICAO_ROWS (), missionx::strct_ext_layer.flag_remove_duplicate_airport_names);

    // v24.06.1
    if (mxUtils::trim (Utils::getNodeText_type_6 (missionx::system_actions::pluginSetupOptions.node, mxconst::get_SETUP_AUTHORIZATION_KEY (), "")).empty ())
      missionx::strct_ext_layer.flag_flightplandatabase_auth_exists = false;
    else
      missionx::strct_ext_layer.flag_flightplandatabase_auth_exists = true;
  }

  // // ImGui::SetWindowFontScale (missionx::strct_setup_layer.fPreferredFontScale);

  mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.305.1

  mx_img_window::HelpMarker ("This screen is dependent on \"flightplandatabase.com\".\n Fill in the information to query from the external site.");
  ImGui::SameLine ();
  ImGui::TextUnformatted ("Flight plans are fetched from \"flightplandatabase.com\". It is only for Flight Simulation.");

  ImGui::BeginGroup ();
  ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow);
  ImGui::TextUnformatted ("You must fill one of the fields in Yellow labels");
  ImGui::PopStyleColor ();

  mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.305.1
  ImGui::BeginChild ("draw_external_fpln_layer_01", ImVec2 (0.0f, win_size_vec2.y * 0.30f), ImGuiChildFlags_Borders); // consume 1/3 of screen
  {
    // -- v26.09.2 ------------
    // Briefer description
    // Show briefer description collapsing header
    // ------------------------
    add_briefer_description_post_mission_creation(!(missionx::data_manager::get_is_random_engine_running()) && missionx::data_manager::missionState < mx_mission_state_enum::mission_is_running);


    mx_img_window::HelpMarker ("FROM or TO fields needs to have a value");
    ImGui::SameLine ();
    {
      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow);
      ImGui::PushItemWidth (200.0f);
      ImGui::PushID ("#FromIcao");
      if (ImGui::InputText ("", missionx::strct_ext_layer.buf_from_icao, 8, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank)) // , ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal);
      {
        missionx::strct_ext_layer.from_icao = std::string (missionx::strct_ext_layer.buf_from_icao);
      }
      ImGui::PopID ();
      this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Enter departure airport ICAO code");

      ImGui::SameLine ();

      mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
      if (mx_img_window::ButtonTooltip (mxUtils::from_u8string (ICON_FA_TRASH_ALT).append ("##ClearFromICAO").c_str (), "Clear from ICAO"))
      {
        missionx::strct_ext_layer.from_icao.clear ();
        memset (missionx::strct_ext_layer.buf_from_icao, 0, sizeof missionx::strct_ext_layer.buf_from_icao);
      }
      mxUiReleaseLastFont ();

      ImGui::SameLine ();
      if (ImGui::Button ("From ICAO")) // first time initialization or manual ICAO fetch
      {
        NavAidInfo nNavAid = data_manager::get_plane_airport_or_nearest_icao ();
        if (!nNavAid.getID().empty())
        {
          mxUtils::copy_string_to_buffer(nNavAid.getID(), missionx::strct_ext_layer.buf_from_icao[0], sizeof(missionx::strct_ext_layer.buf_from_icao)); // v26.04.3
        }

        missionx::strct_ext_layer.from_icao = std::string (missionx::strct_ext_layer.buf_from_icao); // first initialization
      }
      this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Plugin will pick closest airport ICAO for departure");

      // v3.0.253.3 add TO search location
      ImGui::SameLine (0.0f, 10.0f);
      ImGui::PushID ("##ToICAO");
      if (ImGui::InputText ("", missionx::strct_ext_layer.buf_to_icao, 8, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank)) // , ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal);
      {
        missionx::strct_ext_layer.to_icao = std::string (missionx::strct_ext_layer.buf_to_icao);
      }
      ImGui::PopID ();
      this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Enter Arrival airport ICAO code");

      ImGui::SameLine ();
      mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
      if (mx_img_window::ButtonTooltip (mxUtils::from_u8string (ICON_FA_TRASH_ALT).append ("##ClearToICAO").c_str (), "Clear to ICAO"))
      {
        missionx::strct_ext_layer.to_icao.clear ();
        memset (missionx::strct_ext_layer.buf_to_icao, 0, sizeof missionx::strct_ext_layer.buf_to_icao);
      }
      mxUiReleaseLastFont ();

      ImGui::SameLine ();
      ImGui::LabelText ("##To ICAO Label", "To ICAO");

      ImGui::PopStyleColor ();
      ImGui::PopItemWidth ();

      // Display restart plugin button if https failed
      if (missionx::strct_ext_layer.bDisplayPluginsRestart && missionx::data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running)
      {
        const static std::string msg = "Restarting all plugins is a last resort and it might not have the expected effect.\nI suggest to do it:\n1. Only once per session.\n2.You should do it while using LR plane.\n3.If you find the "
                                       "conflicting plugin please notify developer.";
        ImGui::SameLine (0.0f, 20.0);
        mx_img_window::HelpMarker (msg.c_str ());

        int iStyles = 0;
        ImGui::SameLine ();
        ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_red);
        iStyles++;
        ImGui::PushStyleColor (ImGuiCol_ButtonActive, missionx::color::color_vec4_white);
        iStyles++;
        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
        iStyles++;
        if (ImGui::Button ("!!! Reload All Plugins !!!"))
        {
          missionx::strct_ext_layer.bDisplayPluginsRestart = false;
          missionx::data_manager::queFlcActions.push_back (missionx::mx_flc_pre_command::restart_all_plugins);
        }
        this->mx_add_tooltip (missionx::color::color_vec4_yellow, msg);
        ImGui::PopStyleColor (iStyles);
      }
    }

    ImGui::PushItemWidth (520.0f);
    {
      mx_img_window::HelpMarker ("Zero value means any range.\nMax manual range is 9000nm\nIf you receive Error 500, try to limit the range.");
      ImGui::SameLine ();
      ImGui::DragFloat ("", &missionx::strct_ext_layer.ga_range_max_slider_f, ga_steps, ga_range_begin, ga_range_end, "Max Range: %.0f nm");
      ImGui::SameLine ();
      ImGui::TextColored (missionx::color::color_vec4_aqua, "%.0f - %.0f", ((missionx::strct_ext_layer.ga_range_max_slider_f-300.0f <= 0.0f)? 0.0f: (missionx::strct_ext_layer.ga_range_max_slider_f - 300.0f)), missionx::strct_ext_layer.ga_range_max_slider_f );

      mx_img_window::HelpMarker ("How many rows to retrieve. Default 20, Max 100");
      ImGui::SameLine ();
      ImGui::Combo ("Limit rows", &missionx::strct_ext_layer.limit_indx, limit_items, IM_ARRAYSIZE (limit_items)); // default is 20 rows

      mx_img_window::HelpMarker ("Sort options. Data is initially sorted by the external site and not sorted locally.");
      ImGui::SameLine ();
      ImGui::Combo ("Sort By", &missionx::strct_ext_layer.sort_indx, sort_items, IM_ARRAYSIZE (sort_items)); // default is "created"   { "created", "updated", "popularity", "distance" }
    }
    ImGui::PopItemWidth ();

    // v3.303.8.3
    mx_img_window::HelpMarker ("Result should only contain one row per airport, remove duplications based on airport code and name.");
    ImGui::SameLine ();
    if (ImGui::Checkbox ("Remove duplicate airports", &missionx::strct_ext_layer.flag_remove_duplicate_airport_names))
    {
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_REMOVE_DUPLICATE_ICAO_ROWS (), missionx::strct_ext_layer.flag_remove_duplicate_airport_names);
    }
    if (missionx::strct_ext_layer.flag_remove_duplicate_airport_names)
    {
      ImGui::SameLine (0.0f, 30.0f);
      if (ImGui::Checkbox ("Group By Waypoints", &missionx::strct_ext_layer.flag_group_by_waypoints))
      {
        missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_GROUP_DUPLICATES_BY_WAYPOINTS (), missionx::strct_ext_layer.flag_group_by_waypoints);
      }
    }
  }
  ImGui::EndChild ();
  ImGui::EndGroup ();

  mxUiReleaseLastFont (2); // v3.305.1 title and reg text

  // ------------ Buttons -----------------------

  //  Display button only if we are not processing the fetch
  const bool are_we_processing_the_fetch = mxUtils::mx_between <int>(static_cast<int>( missionx::strct_ext_layer.fetch_state ), static_cast<int>(missionx::mxFetchState_enum::fetch_in_process), static_cast<int>( missionx::mxFetchState_enum::fetch_guess_wp), missionx::enums::mx_between_types::both_can_be_equal);

  // v26.06.3 disable button instead of hiding it.
  const auto bDisableFetchButton = mxStartUiDisableState(missionx::data_manager::get_is_random_engine_running() || are_we_processing_the_fetch || (missionx::strct_ext_layer.from_icao.empty() && missionx::strct_ext_layer.to_icao.empty()));
  {
    mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());

    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_orange);
    ImGui::PushStyleColor (ImGuiCol_ButtonActive, missionx::color::color_vec4_azure);
    if (ImGui::Button ("Fetch Flight Plans"))
    {
      // call fetch information data Try to use mutex and std::async
      IXMLNode node_ptr = missionx::data_manager::prop_userDefinedMission_ui.node;
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (mx_ui_mission_type::cargo));
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types_enum::plane_type_ga_floats));
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_NO_OF_LEGS (), 0); // Ignore, set by the external site
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double> (mxconst::get_PROP_MIN_DISTANCE_SLIDER (), (missionx::strct_ext_layer.ga_range_max_slider_f-300.0f < 0.0f)? 0.0f : missionx::strct_ext_layer.ga_range_max_slider_f - 300.0f);
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double> (mxconst::get_PROP_MAX_DISTANCE_SLIDER (), missionx::strct_ext_layer.ga_range_max_slider_f);
      missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_USE_OSM_CHECKBOX (), false); // not OSM but external site

      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_FROM_ICAO (), std::string (missionx::strct_ext_layer.buf_from_icao));
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_TO_ICAO (), missionx::strct_ext_layer.to_icao); // v3.0.253.3 // Ignore, set by the external site
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_LIMIT (), limit_items[missionx::strct_ext_layer.limit_indx]); // Ignore, set by the external site
      missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_SORT_FPLN_BY (), sort_items[missionx::strct_ext_layer.sort_indx]); // Ignore, set by the external site

      this->execAction (missionx::mx_window_actions::ACTION_FETCH_FPLN_FROM_EXTERNAL_SITE);
    }

    ImGui::PopStyleColor (3);

    mxUiReleaseLastFont ();

  }
  mxEndUiDisableState(bDisableFetchButton);


  // ------------ Flight plan Table -----------------------

  const bool bEnableState = mxStartUiDisableState (missionx::strct_ext_layer.fetch_state != missionx::mxFetchState_enum::fetch_not_started); // v24.03.1 disable table until fetch is ended


  mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.305.1

  ImGui::BeginGroup ();
  ImGui::BeginChild ("draw_external_fpln_layer_02", ImVec2 (0.0f, win_size_vec2.y * 0.36f), ImGuiChildFlags_Borders); // consume 1/3 of screen
  {
    // --- Flight plan TABLE
    {

      // Alternating rows slightly grayish
      ImGui::PushStyleColor (ImGuiCol_TableRowBgAlt, IM_COL32 (0x1a, 0x1a, 0x1a, 0xff));
      static constexpr int COLUMN_NUM = 6;

      if (ImGui::BeginTable ("Table", COLUMN_NUM, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit
                             // CONSIDER: ImGuiTableFlags_SizingStretchSame
                             // ImGuiTableFlags_ScrollFreezeTopRow |
                             // ImGuiTableFlags_ScrollFreezeLeftColumn
                             ))
      {
        ImGui::TableSetupScrollFreeze (0, 1); // Make top row always visible

        // Set up the columns of the table
        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow);
        {
          ImGui::TableSetupColumn ("TO", ImGuiTableColumnFlags_None, 260); // to ICAO + (keyName)
          ImGui::TableSetupColumn ("Distance", ImGuiTableColumnFlags_DefaultSort, 80);
          ImGui::TableSetupColumn ("WPT", ImGuiTableColumnFlags_None, 40);
          ImGui::TableSetupColumn ("Plan ID", ImGuiTableColumnFlags_None, 70);
          ImGui::TableSetupColumn ("Popularity", ImGuiTableColumnFlags_None, 90);
          ImGui::TableSetupColumn ("Generate", ImGuiTableColumnFlags_NoSort, 70); // will open modal window that will display lat/lon + notes
          ImGui::TableHeadersRow ();
        }
        ImGui::PopStyleColor ();


        if (missionx::strct_ext_layer.fetch_state == missionx::mxFetchState_enum::fetch_ended)
        {
          this->execAction (mx_window_actions::ACTION_GUESS_WAYPOINTS);
        }
        else if (!missionx::data_manager::tableExternalFPLN_vec.empty () && missionx::data_manager::s_thread_sync_mutex.locked_by_caller () == false) // v3.0.255.4.3 added special class missionx::mutex that can return a bool value if mutex is being locked
        {
          // Sort the data if and as needed
          ImGuiTableSortSpecs *sortSpecs = ImGui::TableGetSortSpecs ();
          if (sortSpecs && sortSpecs->Specs && sortSpecs->SpecsCount >= 1 && sortSpecs->SpecsDirty && data_manager::tableExternalFPLN_vec.size () > 1) // v24.06.1 added "SpecsDirty" test
          {
            // tableDataListTy
            // We sort only by one column, no multi-column sort yet
            const ImGuiTableColumnSortSpecs &colSpec = *(sortSpecs->Specs);

            // We directly sort the tableList: tableExternalFPLN_vec
            std::ranges::sort (data_manager::tableExternalFPLN_vec, // lambda function
                               [colSpec] (const _ext_internet_fpln_strct &a, const _ext_internet_fpln_strct &b)
                               {
                                 int cmp = // less than 0 if a < b
                                   colSpec.ColumnIndex == 0   ? a.toICAO_s.compare (b.toICAO_s)
                                   : colSpec.ColumnIndex == 1 ? static_cast<int> (a.distnace_d - b.distnace_d)
                                   : colSpec.ColumnIndex == 2 ? a.waypoints_i - b.waypoints_i
                                   : colSpec.ColumnIndex == 3 ? a.fpln_unique_id - b.fpln_unique_id
                                   : colSpec.ColumnIndex == 4 ? a.popularity_i - b.popularity_i
                                                              //: colSpec.ColumnIndex == 5 ? 0 // v24.03.1 info button
                                                              // : colSpec.ColumnIndex == 5 ? 0
                                                              : 0;

                                 return colSpec.SortDirection == ImGuiSortDirection_Ascending ? cmp < 0 : cmp > 0;
                               });

            sortSpecs->SpecsDirty ^= 1; // v24.06.1 set to false
          }

          // Add rows to the table
          static int picked_fpln_id_i = -1;
          for (auto rowData : data_manager::tableExternalFPLN_vec) // v24.03.1 Replaces loop with automated one
          {
            // auto& td = td;
            ImGui::TableNextRow ();
            ImGui::TableSetColumnIndex (0);
            ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_mx_bluish);
            if (ImGui::Button (fmt::format ("{} ({})", rowData.toICAO_s, rowData.toName_s).c_str ()))
            {
              // this->m_ui_nav_screen->callNavData(rowData.toICAO_s, true);
              this->call_nav_screen_callNavData(rowData.toICAO_s, true);
            }
            ImGui::PopStyleColor ();

            ImGui::TableSetColumnIndex (1);
            ImGui::Text ("%.2f", static_cast<float> (rowData.distnace_d));
            ImGui::TableSetColumnIndex (2);
            ImGui::Text ("%i", rowData.waypoints_i);
            ImGui::TableSetColumnIndex (3);
            ImGui::Text ("%i", rowData.fpln_unique_id);
            ImGui::TableSetColumnIndex (4);
            ImGui::Text ("%i", rowData.popularity_i);
            ImGui::TableSetColumnIndex (5);

            // ---- Actions
            if (ImGui::Button (fmt::format ("   ...   ###ButtonGen{}", rowData.internal_id).c_str ()))
            {
              // display modal window and rest of information
              picked_fpln_id_i = rowData.internal_id; // store picked FPLN id

              // Generate mission from this after showing "are you sure" modal window
              IXMLNode node_ptr = missionx::data_manager::prop_userDefinedMission_ui.node;
              missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (missionx::mx_ui_mission_type::cargo)); //, node_ptr, node_ptr.getName());
              missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (missionx::mx_plane_types_enum::plane_type_ga)); //, node_ptr, node_ptr.getName());
              missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_NO_OF_LEGS (), rowData.waypoints_i); //, node_ptr, node_ptr.getName());
              missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double> (mxconst::get_PROP_MIN_DISTANCE_SLIDER (), 0.0); //, node_ptr, node_ptr.getName());
              missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double> (mxconst::get_PROP_MAX_DISTANCE_SLIDER (), rowData.distnace_d); //, node_ptr, node_ptr.getName());
              missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_USE_OSM_CHECKBOX (), false); //, node_ptr, node_ptr.getName());

              ImGui::OpenPopup (GENERATE_EXTERNAL_FPLN_QUESTION.c_str ());
            }

            ImVec2 center (ImGui::GetIO ().DisplaySize.x * 0.5f, ImGui::GetIO ().DisplaySize.y * 0.5f);
            ImGui::SetNextWindowPos (center, ImGuiCond_Appearing, ImVec2 (0.5f, 0.5f));
            this->draw_popup_generate_mission_based_on_ext_fpln (GENERATE_EXTERNAL_FPLN_QUESTION, rowData, picked_fpln_id_i);
          } // end loop over all vector
        } // end should we display table or not

        // End of table
        ImGui::EndTable ();
      } // END ImGui::BeginTable

      ImGui::PopStyleColor ();
    } // end draw table if data_manager::tableExternalFPLN_vec is not empty
  }
  ImGui::EndChild ();
  ImGui::EndGroup ();

  mxUiReleaseLastFont ();


  mxEndUiDisableState (bEnableState); // v24.03.1 disable table until fetch is ended

  // ----------------------
  // -- Abort BUTTON
  // ----------------------
  if (are_we_processing_the_fetch || missionx::data_manager::get_is_random_engine_running())
  {
    const mx_window_actions action = (missionx::data_manager::get_is_random_engine_running())?mx_window_actions::ACTION_ABORT_RANDOM_ENGINE_RUN
                                                                                              : mx_window_actions::ACTION_ABORT_EXTERNAL_DBFPLN_FETCH;
    this->add_ui_abort_mission_creation_button(action);
  }

  // ----------------------
  // -- START BUTTON
  // ----------------------

  // if (data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running && missionx::strct_ext_layer.fetch_state == missionx::mxFetchState_enum::fetch_not_started && missionx::flag_generatedRandomFile_success && missionx::strct_generate_template_layer.selectedTemplateKey.empty () && !missionx::data_manager::get_is_random_engine_running() /* make sure that thread is not running */) //
  if (data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running
      && are_we_processing_the_fetch == false // v26.01.3
      && missionx::flag_generatedRandomFile_success
      && missionx::strct_generate_template_layer.selectedTemplateKey.empty ()
      && !missionx::data_manager::get_is_random_engine_running() /* make sure that thread is not running */) //
  {
    mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
    this->add_ui_start_mission_button (missionx::mx_window_actions::ACTION_START_RANDOM_MISSION);
    mxUiReleaseLastFont ();
  }


}


} // missionx