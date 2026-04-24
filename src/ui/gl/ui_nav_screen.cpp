//
// Created by saar.nagar on 4/16/2026.
//

#include "ui_nav_screen.h"

namespace missionx
{


// ----------------------------
// ----------------------------
// ----------------------------

void ui_nav_screen::draw_ils_screen()
{
  auto    win_size_vec2          = this->mxUiGetWindowContentWxH ();
  ImGuiID elevVerticalSlider_gid = 0;

  ImGui::SetWindowFontScale (missionx::strct_setup_layer.fPreferredFontScale);

  if (missionx::strct_ils_layer.layer_state == missionx::mx_layer_state_enum::success_can_draw) // display the success screen - main search screen
  {
    // auto win_size_vec2 = ImGui::GetWindowSize(); // v3.305.1 removed

    ImgWindow::HelpMarker ("The NAV information screen, allows you to search for airports with ILS/VFR approaches.\nYou can filter which types of airports you are looking for or\nLet the plugin randomize the filtering for you.\n\nIt will not "
                                           "generate your FMS nor fetch the ILS Plates for you, this will be up to you.\n");
    ImGui::SameLine ();
    ImGui::TextUnformatted ("NAV information depends on the data collected from X-Plane and Custom Sceneries.");

    this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.305.1
    ImGui::BeginGroup ();
    ImGui::BeginChild ("##NavDataMainTabChild", ImVec2 (-5.0f, -35.0f));
    {
      if (ImGui::BeginTabBar ("NavDataMainTab", ImGuiTabBarFlags_None))
      {
        if (ImGui::BeginTabItem ("ILS Search"))
        {
          this->child_draw_ils_search ();

          ImGui::EndTabItem ();
        }

        const ImGuiTabItemFlags tabFlags = (!missionx::strct_ils_layer.flagForceNavDataTab) ? ImGuiTabItemFlags_None : ImGuiTabItemFlags_SetSelected; // v24.03.1
        if (ImGui::BeginTabItem ("Nav Information", nullptr, tabFlags))
        {
          this->child_draw_nav_search ();

          missionx::strct_ils_layer.flagForceNavDataTab = false; // reset state

          ImGui::EndTabItem ();
        }

        ImGui::EndTabBar ();
      } // end Main Nav Tab

    } // End Child
    ImGui::EndChild ();
    ImGui::EndGroup ();
    this->mxUiReleaseLastFont (); // v3.305.1
  }
  // Display failure message
  else if (missionx::strct_ils_layer.layer_state == missionx::mx_layer_state_enum::failed_data_is_not_present || missionx::strct_ils_layer.layer_state == missionx::mx_layer_state_enum::fatal_database_is_not_initializing_correctly)
  {
    ImGui::NewLine ();

    if (missionx::strct_ils_layer.layer_state == missionx::mx_layer_state_enum::failed_data_is_not_present)
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

    // ImGui::SetWindowFontScale(2.0f);
    this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_BIG ());
    ImGui::TextColored (missionx::color::color_vec4_magenta, "Please wait while plugin tests the validity of the data.... ");
    this->mxUiReleaseLastFont ();
    ImGui::SetWindowFontScale (mxconst::DEFAULT_BASE_FONT_SCALE);

    if (missionx::strct_ils_layer.layer_state < missionx::mx_layer_state_enum::validating_data)
    {
      missionx::strct_ils_layer.layer_state = missionx::mx_layer_state_enum::validating_data;
    }
  }

  ImGui::SetWindowFontScale (missionx::strct_setup_layer.fPreferredFontScale);
}

// ----------------------------

void ui_nav_screen::child_draw_ils_search()
{
  constexpr static auto  elevVerticalTreeNodeName = "Elev. slider";
  constexpr static float CHILD_SIZE_MODIFIER_F    = 0.15f;


  auto win_size_vec2    = this->mxUiGetWindowContentWxH ();
  auto uiUpperChildInfo = ImGui::GetCurrentWindow ();

  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.305.1
  ImGui::BeginGroup (); // group 1
  {
    const auto child_vec2 = ImVec2 (this->mxUiGetWindowContentWxH ().x * 0.49f, this->mxUiGetWindowContentWxH ().y * 0.44f);

    // ------------------
    // IFR / VFR Buttons
    // ------------------
    ImGui::BeginGroup ();
    {
      const auto i_picked = this->add_ui_two_option_buttons (missionx::strct_ils_layer.isIFR, missionx::strct_ils_layer.isVFR, missionx::PICKED_IFR, missionx::PICKED_VFR);
      if (i_picked != data_manager::ui_ifr_or_vfr_i)
      {
        data_manager::ui_ifr_or_vfr_i = i_picked;
        switch (data_manager::ui_ifr_or_vfr_i)
        {
          case missionx::PICKED_VFR:
          {
              // set sliders min/max
            strct_ils_layer.ils_sliderVal1 = strct_ils_layer.ils_or_vfr_min_slider_value = mxconst::SLIDER_ILS_MIN_VFR_SEARCH_RADIUS;
            strct_ils_layer.ils_or_vfr_max_slider_value = mxconst::SLIDER_ILS_MAX_VFR_SEARCH_RADIUS;

            if (strct_ils_layer.ils_sliderVal2 > 50.0f) // search radius
              strct_ils_layer.ils_sliderVal2 = 10.0f;
            if (strct_ils_layer.slider_min_rw_length_i > 300) // runway length
              strct_ils_layer.slider_min_rw_length_i = 300;
            if (strct_ils_layer.slider_min_rw_width_i > 15) // runway width
              strct_ils_layer.slider_min_rw_width_i = 15;
          }
          break;
          case missionx::PICKED_IFR:
            [[fallthrough]];
          default:
          {
            // set sliders min/max
            strct_ils_layer.ils_sliderVal1 = strct_ils_layer.ils_or_vfr_min_slider_value = mxconst::SLIDER_ILS_MIN_SEARCH_RADIUS;
            strct_ils_layer.ils_or_vfr_max_slider_value = mxconst::SLIDER_ILS_MAX_SEARCH_RADIUS;

            // validate max radius is longer thant min
            if (strct_ils_layer.ils_sliderVal2 <= mxconst::SLIDER_ILS_MIN_SEARCH_RADIUS) // search radius
              strct_ils_layer.ils_sliderVal2 = mxconst::SLIDER_ILS_MIN_SEARCH_RADIUS * 1.5f;
            if (strct_ils_layer.slider_min_rw_length_i < 800) // runway length
              strct_ils_layer.slider_min_rw_length_i = 800;
            if (strct_ils_layer.slider_min_rw_width_i < 45) // runway width
              strct_ils_layer.slider_min_rw_width_i = 45;
          }
          break;
        }
        // refresh label
        strct_ils_layer.ils_slider2_lbl = "[" + Utils::formatNumber<float> (strct_ils_layer.ils_sliderVal1, 0) + ".." + Utils::formatNumber<float> (strct_ils_layer.ils_sliderVal2, 0) + "]";
      }
    }

    //------------------------------------------------
    //     search ILS/VFR runways button
    //------------------------------------------------
    ImGui::SameLine (0.0f, 45.0f);
    this->add_ui_ils_vfr_search_airports_button (missionx::mx_window_actions::ACTION_FETCH_ILS_AIRPORTS);
    // Display START mission button
    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow);
    ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_indigo);
    this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
    {
      if (data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running 
          && missionx::flag_generatedRandomFile_success 
          && missionx::strct_generate_template_layer.selectedTemplateKey.empty ()
          && !missionx::data_manager::flag_generate_engine_is_running /* make sure that thread is not running */) //
      {
        ImGui::SameLine (win_size_vec2.x * 0.5f + 130.0f);
        this->add_ui_start_mission_button (missionx::mx_window_actions::ACTION_START_RANDOM_MISSION);
      }
    }
    this->mxUiReleaseLastFont ();
    ImGui::PopStyleColor (2);

    ImGui::EndGroup ();

    //
    ImGui::BeginGroup ();
    ImGui::BeginChild ("Left ILS", child_vec2, ImGuiChildFlags_Borders);
    {
      // From/To ICAO tree
      if (ImGui::TreeNode (reinterpret_cast<void *>(static_cast<intptr_t> (1)), "%s", fmt::format ("From/To: {}/{}", missionx::strct_ils_layer.from_icao, missionx::strct_ils_layer.to_icao).c_str ()))
      {
        ImgWindow::HelpMarker ("Enter optional starting ICAO airport.");
        ImGui::SameLine ();
        ImGui::PushItemWidth (100.0f);
        if (ImGui::InputText ("##From_ILS_Icao_text", missionx::strct_ils_layer.buf1, 8, ImGuiInputTextFlags_CharsUppercase)) // , ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal);
        {
          missionx::strct_ils_layer.from_icao = std::string (missionx::strct_ils_layer.buf1);
        }
        this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Optional: enter departure airport ICAO code");

        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
        {
          ImGui::SameLine ();
          if (ImgWindow::ButtonTooltip (mxUtils::from_u8string (ICON_FA_TRASH_ALT).c_str (), "Clear From ICAO")) // should have been ImGui::ButtonTooltip
          {
            missionx::strct_ils_layer.from_icao.clear ();
            memset (missionx::strct_ils_layer.buf1, 0, sizeof missionx::strct_ils_layer.buf1);
          }
        }
        this->mxUiReleaseLastFont ();

        ImGui::SameLine ();
        if (ImGui::Button ("From ICAO") || missionx::strct_ils_layer.bFirstTime) // first time initialization or manual ICAO fetch
        {
          #ifdef IBM
          missionx::strct_ils_layer.navaid = data_manager::get_plane_airport_or_nearest_icao ();
          #else
          const auto tempNav                 = data_manager::get_plane_airport_or_nearest_icao ();
          missionx::strct_ils_layer.navaid = tempNav;
          #endif
          if (!missionx::strct_ils_layer.navaid.getID().empty())
            mxUtils::copy_string_to_buffer(missionx::strct_ils_layer.buf1, missionx::strct_ils_layer.navaid.ID[0], sizeof(missionx::strct_ils_layer.navaid.ID)); // v26.04.3

          missionx::strct_ils_layer.from_icao  = std::string (missionx::strct_ils_layer.buf1); // first initialization
          missionx::strct_ils_layer.sNavICAO   = missionx::strct_ils_layer.from_icao; // v24.03.2 NavData ICAO will also be initialized.
          missionx::strct_ils_layer.bFirstTime = false;
        }

        //////////////////
        // New Line
        // TO input item
        /////////////////
        ImgWindow::HelpMarker ("Enter optional destination ICAO airport.\nThe plugin will search for all ICAOs containing the string you entered.");
        ImGui::SameLine ();
        ImGui::PushItemWidth (100.0f);
        if (ImGui::InputText ("##To_ILS_Icao_text", missionx::strct_ils_layer.buf2, 8, ImGuiInputTextFlags_CharsUppercase)) // , ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal);
        {
          missionx::strct_ils_layer.to_icao = mxUtils::trim (std::string (missionx::strct_ils_layer.buf2));
        }
        this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Optional: enter arrival airport ICAO code");
        ImGui::SameLine ();
        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
        {
          ImGui::SameLine ();
          ImGui::PushID ("##ClearICAO"); // v25.12.1 fix button do not respond to clicks
          if (ImgWindow::ButtonTooltip (mxUtils::from_u8string (ICON_FA_TRASH_ALT).c_str (), "Clear to ICAO"))
          {
            missionx::strct_ils_layer.to_icao.clear ();
            memset (missionx::strct_ils_layer.buf2, 0, sizeof missionx::strct_ils_layer.buf2);
          }
          ImGui::PopID ();
        }
        this->mxUiReleaseLastFont ();
        ImGui::SameLine ();
        ImGui::TextColored (missionx::color::color_vec4_white, "To ICAO");

        if (missionx::strct_ils_layer.to_icao.length () < 2) // v3.24.1 We can ignore distance only
          missionx::strct_ils_layer.flagIgnoreDistanceFilter = false;

        // ImGui::SameLine (0.0f, 5.0f); // v25.12.1 removed
        ImgWindow::HelpMarker ("You must enter Two or more search characters to enable the option to ignore 'precise distance' filter.\nThe search result will be limited to 250 rows.");
        ImGui::SameLine ();
        const bool bICAO_isEmpty = this->mxStartUiDisableState (missionx::strct_ils_layer.to_icao.length() < 2); // v24.03.1 disable line ?
        ImGui::Checkbox ("Ignore Dist.", &missionx::strct_ils_layer.flagIgnoreDistanceFilter);
        this->mxEndUiDisableState (bICAO_isEmpty); // v24.03.1 disable line ?


        ImGui::TreePop ();
      } // END FROM/TO Tree

      // Min Distance
      {
        const bool bIgnoreDistanceFilter = this->mxStartUiDisableState (missionx::strct_ils_layer.flagIgnoreDistanceFilter); // v24.03.1 disable line ?

        ImGui::TextColored (missionx::color::color_vec4_yellow, "Choose Route Range");
        ImGui::PushID ("##Slider_ILS_MaxDistance");
        {
          // if (ImGui::SliderFloat ("", &strct_ils_layer.ils_sliderVal2, mxconst::SLIDER_SHORTEST_MAX_ILS_SEARCH_RADIUS, mxconst::SLIDER_ILS_MAX_SEARCH_RADIUS, "%.0f nm"))
          if (ImGui::SliderFloat ("", &strct_ils_layer.ils_sliderVal2, strct_ils_layer.ils_or_vfr_min_slider_value, strct_ils_layer.ils_or_vfr_max_slider_value, "%.0f nm"))
          {
            if (data_manager::ui_ifr_or_vfr_i == PICKED_IFR)
            {
              // calc and construct low/high label for slider
              if (strct_ils_layer.ils_sliderVal2 / 500.0f > 1.0f)
                strct_ils_layer.ils_sliderVal1 = strct_ils_layer.ils_sliderVal2 * 0.75f;
              else if (strct_ils_layer.ils_sliderVal2 / 250.0f > 1.0f)
                strct_ils_layer.ils_sliderVal1 = strct_ils_layer.ils_sliderVal2 * 0.5f;
              else
                strct_ils_layer.ils_sliderVal1 = mxconst::SLIDER_ILS_MIN_SEARCH_RADIUS;

            }
            else
            {
              // VFR will be limited to 500nm
              if (strct_ils_layer.ils_sliderVal2 > 500.0f)
                strct_ils_layer.ils_sliderVal2 = 500.0f;

              // calc minimum
              if (strct_ils_layer.ils_sliderVal2 / 250.0f > 1.0f)
                strct_ils_layer.ils_sliderVal1 = strct_ils_layer.ils_sliderVal2 * 0.75f;
              else if (strct_ils_layer.ils_sliderVal2 / 150.0f > 1.0f)
                strct_ils_layer.ils_sliderVal1 = strct_ils_layer.ils_sliderVal2 * 0.65f;
              else if (strct_ils_layer.ils_sliderVal2 / 50.0f > 1.0f)
                strct_ils_layer.ils_sliderVal1 = strct_ils_layer.ils_sliderVal2 * 0.5f;
              else
                strct_ils_layer.ils_sliderVal1 = mxconst::SLIDER_ILS_MIN_SEARCH_RADIUS;

            }

            // Validate max slider is greater than shorter slider
            if (strct_ils_layer.ils_sliderVal1 >= strct_ils_layer.ils_sliderVal2)
            {
              strct_ils_layer.ils_sliderVal1 = strct_ils_layer.ils_sliderVal2;
              if (data_manager::ui_ifr_or_vfr_i == PICKED_IFR)
                strct_ils_layer.ils_sliderVal2 = strct_ils_layer.ils_sliderVal1 * 1.2f;
              else
                strct_ils_layer.ils_sliderVal2 = strct_ils_layer.ils_sliderVal1 * 2.0f;
            }

            strct_ils_layer.ils_slider2_lbl = "[" + Utils::formatNumber<float> (strct_ils_layer.ils_sliderVal1, 0) + ".." + Utils::formatNumber<float> (strct_ils_layer.ils_sliderVal2, 0) + "]";
          }
        }
        ImGui::PopID ();
        ImGui::SameLine ();
        ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", strct_ils_layer.ils_slider2_lbl.c_str ());

        this->mxEndUiDisableState (bIgnoreDistanceFilter); // disable line ?

      } // end Max Distance

      // Minimum Runway Length && Minimum Runway Width mt.
      {
        // ImGui::NewLine();
        ImGui::Spacing (); // v3.305.1

        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
        ImGui::SliderInt ("Min Runway Length", &strct_ils_layer.slider_min_rw_length_i, mxconst::SLIDER_ILS_SHORTEST_RW_LENGTH_MT, mxconst::SLIDER_ILS_LONGEST_RW_LENGTH_MT, "%i meters");
        ImGui::PopStyleColor (1);
        this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Pick minimal runway Length filter");

        // ImGui::NewLine();
        ImGui::Spacing (); // v3.305.1

        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
        ImGui::SliderInt ("Min Runway Width", &strct_ils_layer.slider_min_rw_width_i, mxconst::SLIDER_ILS_SHORTEST_RW_WIDTH_MT, mxconst::SLIDER_ILS_WIDEST_RW_WIDTH_MT, "%i meters");
        ImGui::PopStyleColor (1);
        this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Pick minimal runway Width filter");
      } //  Minimum Runway Length && Minimum Runway Width mt.

      // Limit Rows
      {
        ImGui::SetNextItemWidth (70.0f);
        ImGui::Combo ("Limit rows", &missionx::strct_ils_layer.limit_indx, missionx::strct_ils_layer.limit_items, IM_ARRAYSIZE (missionx::strct_ils_layer.limit_items)); // default is 250 rows
        ImGui::SameLine ();
        ImgWindow::mxUiHelpMarker (missionx::color::color_vec4_aquamarine, fmt::format ("How many rows to retrieve. Default {}.\nCan drastically affect FPS.", missionx::strct_ils_layer.limit_items[0]).c_str ());
      } // limit rows
    }
    ImGui::EndChild (); // end left ILS child
    ImGui::EndGroup (); // end left group

    ImGui::SameLine (this->mxUiGetWindowContentWxH ().x * 0.5f);

    ImGui::BeginGroup ();
    ImGui::BeginChild ("Right ILS", child_vec2, ImGuiChildFlags_Borders);
    {
      // Which ILS types to search
      if (data_manager::ui_ifr_or_vfr_i == missionx::PICKED_IFR)
      {
        const std::string ils_type_picked_s = strct_ils_layer.get_ils_types_picked ();
        const std::string picked_lbl_s      = ((ils_type_picked_s.empty ()) ? "Any NAV type" : ils_type_picked_s);

        ImGui::TextColored (missionx::color::color_vec4_yellow, "Type:");
        ImGui::SameLine ();
        ImGui::TextColored (missionx::color::color_vec4_white, "%s", picked_lbl_s.c_str ());

        {
          if (ImGui::TreeNode (reinterpret_cast<void *>(static_cast<intptr_t> (2)), "%s", "NAV Filtering"))
          {
            ImGui::NewLine ();

            // loop over all ils in mapCheck_ILS_types and display state
            int counter = 0;
            for (auto &[keyType, bVal] : missionx::strct_ils_layer.mapCheck_ILS_types)
            {
              counter++;
              if (counter % 4 == 0)
                ImGui::NewLine ();

              ImGui::SameLine (); // we always need same line. New line is special case

              ImGui::Checkbox (missionx::mapILS_types[keyType].c_str (), &bVal); // no need to handle picked checkbox, since we are handling it prior to tree display
            }
            ImGui::Separator ();
            ImGui::TreePop ();
          }
        }
      } // end ILS types to search

      // Minimum airport elevation
      {
        const std::string min_ap_elev_ft_s = "Min Airport Elevation:";

        ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", min_ap_elev_ft_s.c_str ());
        ImGui::SameLine ();
        ImGui::TextColored (missionx::color::color_vec4_white, "%s", (Utils::formatNumber<int> (missionx::strct_ils_layer.slider_min_airport_elev_ft_i) + "ft").c_str ());

        ImGui::SameLine (0.0f, 10.0f);

        if (ImGui::TreeNode (reinterpret_cast<void *>(static_cast<intptr_t> (3)), "%s", "Elev. slider"))
        {
          if (missionx::strct_ils_layer.enum_elevSliderOpenState == missionx::enums::mx_treeNodeState::closed)
            missionx::strct_ils_layer.enum_elevSliderOpenState = missionx::enums::mx_treeNodeState::opened;

          static constexpr float vertical_slider_spacing_f = 180.0f;
          static constexpr float vertical_slider_width_f   = 80.0f;
          ImGui::NewLine ();
          ImGui::SameLine (0.0f, vertical_slider_spacing_f);
          ImGui::VSliderInt ("##airportElevSliderInt", ImVec2 (vertical_slider_width_f, 80.0f), &missionx::strct_ils_layer.slider_min_airport_elev_ft_i, mxconst::SLIDER_ILS_LOWEST_AIRPORT_ELEV_FT, mxconst::SLIDER_ILS_HIGHEST_AIRPORT_ELEV_FT, "%i");

          ImGui::NewLine ();
          ImGui::SameLine (0.0f, vertical_slider_spacing_f);
          if (ImGui::Button ("Reset", ImVec2 (vertical_slider_width_f, 30.0f)))
          {
            missionx::strct_ils_layer.slider_min_airport_elev_ft_i = mxconst::SLIDER_ILS_STARTING_AIRPORT_ELEV_VALUE_FT;
            ImGui::SetScrollHereY (1.0f);
          }

          if (missionx::strct_ils_layer.enum_elevSliderOpenState == missionx::enums::mx_treeNodeState::opened)
          {
            ImGui::SetScrollHereY (1.0f);
            missionx::strct_ils_layer.enum_elevSliderOpenState = missionx::enums::mx_treeNodeState::was_opened;
          }


          ImGui::TreePop ();
        }
        else
          missionx::strct_ils_layer.enum_elevSliderOpenState = missionx::enums::mx_treeNodeState::closed; // when close we must reset it

      } // Minimum airport elevation
    }
    ImGui::EndChild ();
    ImGui::EndGroup ();
  }

  // ImGui::EndChild (); // end outer child
  ImGui::EndGroup ();

  this->mxUiReleaseLastFont (); // v3.305.1

  //------------------------------------------------
  //     Status Messages
  //------------------------------------------------
  if (missionx::data_manager::flag_generate_engine_is_running && data_manager::strct_ui_share_data.user_message_line1.empty())
  {
    this->set_bottom_message_line1 ("Random Engine is running, please wait...", DEFAULT_MESSAGE_TIME_I);
  }
  else if (missionx::data_manager::flag_apt_dat_optimization_is_running && data_manager::strct_ui_share_data.user_message_line1.empty())
  {
    this->set_bottom_message_line1 ("Can't Generate mission, apt data optimization is currently running. Please wait for it to finish first !!!", DEFAULT_MESSAGE_TIME_I);
  }


  //------------------------------------------------
  //     Airports Query Result Table
  //------------------------------------------------

  const float fStartButtonHeight = (data_manager::missionState < missionx::mx_mission_state_enum::mission_is_running
                                    && missionx::flag_generatedRandomFile_success
                                    && missionx::strct_generate_template_layer.selectedTemplateKey.empty ()
                                    && !missionx::data_manager::flag_generate_engine_is_running) ? 25.0f : 0.0f;

  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.305.1

  ImGui::BeginGroup ();
  // ImGui::BeginChild ("draw_ils_layer_table_02", ImVec2 (0.0f, win_size_vec2.y - uiUpperChildSizeVec2.y - 0.0f /*buttons*/ - 10.0f /*bottom message space*/ - fStartButtonHeight /* Is StartButton visible */), ImGuiChildFlags_Borders); // Size relative to the upper child size
  {
    ImGui::PushStyleColor (ImGuiCol_TableRowBgAlt, IM_COL32 (0x1a, 0x1a, 0x1a, 0xff));
    constexpr const static int COLUMN_NUM = 11;

    if (ImGui::BeginTable ("TableILS", COLUMN_NUM, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit
                           // ImGuiTableFlags_ScrollFreezeTopRow | ImGuiTableFlags_ScrollFreezeLeftColumn
                           ))
    {
      ImGui::TableSetupScrollFreeze (0, 1); // Make top row always visible

      // Set up the columns of the table
      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow);
      {
        ImGui::TableSetupColumn ("TO", ImGuiTableColumnFlags_None, 210); // to ICAO + (keyName)
        ImGui::TableSetupColumn ("Dist.", ImGuiTableColumnFlags_DefaultSort, 45);
        ImGui::TableSetupColumn ("ILS Type", ImGuiTableColumnFlags_None, 70);
        ImGui::TableSetupColumn ("Freq.", ImGuiTableColumnFlags_None, 50);
        ImGui::TableSetupColumn ("RW", ImGuiTableColumnFlags_None, 30);
        ImGui::TableSetupColumn ("Len mt", ImGuiTableColumnFlags_None, 55);
        ImGui::TableSetupColumn ("Width", ImGuiTableColumnFlags_None, 50);
        ImGui::TableSetupColumn ("Elev ft.", ImGuiTableColumnFlags_None, 65);
        ImGui::TableSetupColumn ("Gen.", ImGuiTableColumnFlags_NoSort, 50); //
        ImGui::TableSetupColumn ("Surface", ImGuiTableColumnFlags_NoSort, 90); // v3.0.253.13
        ImGui::TableSetupColumn ("Bearing", ImGuiTableColumnFlags_NoSort, 50); // v3.0.253.13 bearing between start and target icao runway
        ImGui::TableHeadersRow ();
      }
      ImGui::PopStyleColor ();

      if (!missionx::data_manager::table_ILS_rows_vec.empty () && missionx::strct_ils_layer.fetch_ils_state != missionx::mxFetchState_enum::fetch_in_process && missionx::data_manager::s_thread_sync_mutex.locked_by_caller () == false)
      {
        // Sort the data if and as needed
        ImGuiTableSortSpecs *sortSpecs = ImGui::TableGetSortSpecs ();
        if (sortSpecs && sortSpecs->SpecsDirty && sortSpecs->Specs && sortSpecs->SpecsCount >= 1 && data_manager::table_ILS_rows_vec.size () > 1)
        {
          // tableDataListTy
          // We sort only by one column, no multi-column sort
          const ImGuiTableColumnSortSpecs &colSpec = *(sortSpecs->Specs);

          // We directly sort the tableList: tableExternalFPLN_vec
          std::ranges::sort (data_manager::table_ILS_rows_vec, // lambda function
                             [colSpec] (const missionx::mx_ils_airport_row_strct &a, const missionx::mx_ils_airport_row_strct &b)
                             {
                               const int cmp = // less than 0 if a < b
                                 colSpec.ColumnIndex == 0   ? a.toICAO_s.compare (b.toICAO_s)
                                 : colSpec.ColumnIndex == 1 ? static_cast<int> (a.distnace_d - b.distnace_d)
                                 : colSpec.ColumnIndex == 2 ? a.locType_s.compare (b.locType_s)
                                 : colSpec.ColumnIndex == 3 ? 0
                                                            : // no sorting
                                   colSpec.ColumnIndex == 4 ? a.loc_rw_s.compare (b.loc_rw_s)
                                 : colSpec.ColumnIndex == 5 ? a.rw_length_mt_i - b.rw_length_mt_i
                                 : colSpec.ColumnIndex == 6 ? static_cast<int> (a.rw_width_d - b.rw_width_d)
                                                            : // width of rw
                                   colSpec.ColumnIndex == 7 ? a.ap_elev_ft_i - b.ap_elev_ft_i
                                                            : // elevation ft.
                                   // colSpec.ColumnIndex == 8 ? 0 // v24.03.1 info button - replaced localizer bearing
                                   colSpec.ColumnIndex == 8 ? 0
                                                            : // GEN button - don't sort
                                   colSpec.ColumnIndex == 9 ? 0
                                                            : // Surface - don't sort surface type v3.0.253.13
                                   colSpec.ColumnIndex == 10 ? static_cast<int> (a.bearing_from_to_icao_d - b.bearing_from_to_icao_d)
                                                             : // v3.0.253.13
                                   0; // last option should have : 0 at the end like else

                               return colSpec.SortDirection == ImGuiSortDirection_Ascending ? cmp < 0 : cmp > 0;
                             });

          sortSpecs->SpecsDirty = false;
        } // end Sorting logic

        // Add rows to the table
        static int picked_fpln_id_i = -1;
        for (const auto &rowData : data_manager::table_ILS_rows_vec)
        {
          int i = 0;
          // auto& td = *td;
          ImGui::TableNextRow ();
          ImGui::TableSetColumnIndex (i);
          i++;

          ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_mx_bluish);
          if (ImGui::Button (fmt::format ("{} ({}){}", rowData.toICAO_s, ((rowData.toName_s.length () > 23) ? rowData.toName_s.substr (0, 20) + "..." : rowData.toName_s), fmt::format ("##ButtonInfo{}", rowData.seq)).c_str ()))
          {
            this->callNavData (rowData.toICAO_s, false);
          }
          ImGui::PopStyleColor ();

          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::Text ("%.0f", static_cast<float> (rowData.distnace_d));
          this->mx_add_tooltip (missionx::color::color_vec4_beige, "Distance in nautical miles.");
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::TextUnformatted (rowData.locType_s.c_str ()); // Type
          ImGui::TableSetColumnIndex (i);
          i++;
          const std::string sLocTypesToFormat = "VORDMENDBILS-CAT-IILS-CAT-IIIILS-CAT-IILOC";
          const std::string locTypeUpperCase  = mxUtils::stringToUpper (rowData.locType_s);
          const bool        bFormatFrq        = (sLocTypesToFormat.find (locTypeUpperCase) != std::string::npos);
          std::string       frq_s             = mxUtils::formatNumber<int> (rowData.loc_frq_mhz);
          frq_s                               = (bFormatFrq && frq_s.length () > 3) ? frq_s.insert (3, 1, '.') : frq_s; // v25.08.1 added length test for VFR
          ImGui::TextUnformatted (frq_s.c_str ());
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::TextUnformatted (rowData.loc_rw_s.c_str ()); // on which rw
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::Text ("%i", rowData.rw_length_mt_i);
          this->mx_add_tooltip (missionx::color::color_vec4_beige, "Runway length in meters.");
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::Text ("%.2f", static_cast<float> (rowData.rw_width_d));
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::Text ("%i", rowData.ap_elev_ft_i); // elevation ft.

          ImGui::TableSetColumnIndex (i);
          i++;

          // Last field, the creation mission
          // ---- Actions
          if (ImGui::Button (fmt::format (" ... ###ButtonGen{}", rowData.seq).c_str ())) // v24.03.1 replaced buff with fmt::format
          {
            // display modal window and rest of information
            picked_fpln_id_i = rowData.seq; // store picked seq

            // Generate mission from this after showing "are you sure" modal window
            IXMLNode node_ptr = missionx::data_manager::prop_userDefinedMission_ui.node;
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_MED_CARGO_OR_OILRIG (), static_cast<int> (mx_ui_mission_type::cargo)); //, node_ptr, node_ptr.getName()); // always cargo
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_NO_OF_LEGS (), 0); //, node_ptr, node_ptr.getName()); // legs will be dectated by RandomEngine. Should only be 1 and simmer will add the rest
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double> (mxconst::get_PROP_MIN_DISTANCE_SLIDER (), strct_ils_layer.ils_sliderVal1); //, node_ptr, node_ptr.getName());
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<double> (mxconst::get_PROP_MAX_DISTANCE_SLIDER (), strct_ils_layer.ils_sliderVal2); //, node_ptr, node_ptr.getName());
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_USE_OSM_CHECKBOX (), false); //, node_ptr, node_ptr.getName());     // always false
            missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_USE_WEB_OSM_CHECKBOX (), false); //, node_ptr, node_ptr.getName()); // always false

            missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_FROM_ICAO (), strct_ils_layer.navaid.getID ()); //, node_ptr, node_ptr.getName()); //
            missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_TO_ICAO (), rowData.toICAO_s); //, node_ptr, node_ptr.getName());                 //

            ImGui::OpenPopup (GENERATE_ILS_QUESTION.c_str ());
          }

          // v3.0.253.13 Other information about runway/airport
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::TextUnformatted (rowData.surfType_s.c_str ()); // Surface Type
          ImGui::TableSetColumnIndex (i);
          i++;
          ImGui::Text ("%.0f", static_cast<float> (rowData.bearing_from_to_icao_d)); // bearing between start and target icao
          this->mx_add_tooltip (missionx::color::color_vec4_beige, "Bearing to airport relative to plane position.");


          // DISPLAY POPUP
          ImVec2 center (ImGui::GetIO ().DisplaySize.x * 0.5f, ImGui::GetIO ().DisplaySize.y * 0.5f); // center of screen
          ImGui::SetNextWindowPos (center, ImGuiCond_Appearing, ImVec2 (0.5f, 0.5f));
          ImGui::SetNextWindowSize (ImVec2 (480.0f, 435.0f));

          ImGui::PushStyleColor (ImGuiCol_PopupBg, missionx::color::color_vec4_black);
          {
            if (ImGui::BeginPopupModal (GENERATE_ILS_QUESTION.c_str (), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
              ImVec2 modal_center (mxUiGetContentWidth () * 0.5f, ImGui::GetWindowHeight () * 0.5f);
              if (rowData.seq == picked_fpln_id_i)
              {
                this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
                ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", "To: ");
                this->mxUiReleaseLastFont ();

                ImGui::SameLine (0.0f, 1.0f); // one space
                ImGui::TextColored (missionx::color::color_vec4_greenyellow, "%s", (rowData.toICAO_s + " - " + rowData.toName_s.substr (0, 30)).c_str ());

                // Pick Plane Type
                ImGui::NewLine ();
                // label
                ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow); // yellow
                ImGui::Text ("Pick Preferred Plane:");
                ImGui::PopStyleColor (1);
                ImGui::NewLine ();

                int radio_btn_counter = 0;
                for (const auto &planeTypeLabel : missionx::mapListPlaneRadioLabel | std::views::values) // v24.12.1
                {
                  if (planeTypeLabel.type == mx_plane_types_enum::plane_type_helos || planeTypeLabel.type == mx_plane_types_enum::plane_type_ga_floats)
                    continue;

                  ImGui::SameLine ();
                  if (ImGui::RadioButton (planeTypeLabel.label.c_str (), missionx::strct_ils_layer.iRadioPlaneType == planeTypeLabel.type))
                  {
                    missionx::strct_ils_layer.iRadioPlaneType = planeTypeLabel.type;
                  }
                  radio_btn_counter++;
                  if (radio_btn_counter%4 == 0)
                    ImGui::NewLine();

                } // end loop over all plane types

                ImGui::NewLine ();
                ImGui::Checkbox ("Start from plane position", &missionx::strct_cross_layer_properties.flag_start_from_plane_position); // v3.0.253.11 // v3.0.253.12 reposition checkbox in the popup generate window

                if (missionx::strct_ils_layer.iRadioPlaneType > mx_plane_types_enum::plane_type_helos &&  missionx::strct_ils_layer.iRadioPlaneType < mx_plane_types_enum::plane_type_jets)
                {
                  ImGui::SameLine(0.0f, 10.0f);
                  add_ui_is_amphibian();
                }

                ImGui::Spacing ();
                ImGui::Checkbox ("Generate GPS waypoints.", &missionx::strct_cross_layer_properties.flag_generate_gps_waypoints); // v3.0.253.12
                this->add_ui_auto_load_checkbox (missionx::mx_window_actions::ACTION_SAVE_USER_SETUP_OPTIONS); // v25.04.2
                ImGui::Spacing (); // v3.303.14.2 added default weight to the generate screen
                ImGui::Checkbox ("Add default base weights.\n(Not advisable for planes > GAs)", &missionx::adv_settings_strct.flag_add_default_weight_settings);
                ImGui::Spacing ();

                this->add_ui_pick_subcategories (missionx::mapMissionCategories[static_cast<int> (missionx::mx_ui_mission_type::cargo)]);
                ImGui::Spacing ();
                // v3.303.10 // v25.04.1 moved advance button to the popup window for better flow
                this->add_ui_advance_settings_random_date_time_weather_and_weight_button (missionx::adv_settings_strct.iClockDayOfYearPicked, missionx::adv_settings_strct.iClockHourPicked, missionx::adv_settings_strct.iClockMinutesPicked, mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.10 convert the random dateTime button to a self contain function
                ImGui::Spacing ();
                add_designer_mode_checkbox (); // v24.03.2 Designer mode flag

                ImGui::NewLine ();
                ImGui::Separator ();
                ImGui::NewLine ();
                ImGui::NewLine ();
                ImGui::SameLine (modal_center.x * 0.2f);
                ImGui::SetItemDefaultFocus ();

                // v3.303.10
                static bool bRerunRandomDateTime{ false };

                // display the option only if we are not in the middle of a running mission
                if (missionx::data_manager::missionState != missionx::mx_mission_state_enum::mission_is_running)
                {

                  bRerunRandomDateTime = add_ui_checkbox_rerun_random_date_and_time ();
                  ImGui::SameLine (0.0f, 5.0f);

                  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
                }

                if (missionx::data_manager::missionState == missionx::mx_mission_state_enum::mission_is_running)
                {
                  ImGui::TextColored (missionx::color::color_vec4_aqua, "%s", "Can't generate at this time.");
                }
                else if (ImGui::Button (">> Generate <<", ImVec2 (120, 0)))
                {
                  if (bRerunRandomDateTime) // v3.303.10
                    this->execAction (missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);

                  // Prepare and call ACTION_GENERATE_RANDOM_MISSION
                  data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_FPLN_ID_PICKED (), picked_fpln_id_i);
                  missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<int> (mxconst::get_PROP_PLANE_TYPE_I (), static_cast<int> (strct_ils_layer.iRadioPlaneType)); // v25.04.1 moved into popup
                  // v26.04.4 force amphibian flag base on plane type
                  if (missionx::strct_ils_layer.iRadioPlaneType > mx_plane_types_enum::plane_type_turboprops || missionx::strct_ils_layer.iRadioPlaneType == mx_plane_types_enum::plane_type_helos)
                    missionx::strct_user_create_layer.flag_plane_is_amphibian = false;
                  missionx::data_manager::prop_userDefinedMission_ui.setBoolProperty(mxconst::get_PLANE_IS_AMPHIBIAN(), strct_user_create_layer.flag_plane_is_amphibian); // v26.04.4

                  missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_START_FROM_PLANE_POSITION (), missionx::strct_cross_layer_properties.flag_start_from_plane_position); // v3.0.253.11 start from plane position
                  missionx::data_manager::prop_userDefinedMission_ui.setNodeProperty<bool> (mxconst::get_PROP_GENERATE_GPS_WAYPOINTS (), missionx::strct_cross_layer_properties.flag_generate_gps_waypoints); // v3.0.253.12 generate GPS waypoints


                  if (const auto vecToDisplay = missionx::mapMissionCategories[static_cast<int> (missionx::mx_ui_mission_type::cargo)]; static_cast<int>(vecToDisplay.size ()) > missionx::strct_user_create_layer.iMissionSubCategoryPicked)
                    missionx::data_manager::prop_userDefinedMission_ui.setNodeStringProperty (mxconst::get_PROP_MISSION_SUBCATEGORY_LBL (), vecToDisplay.at (missionx::strct_user_create_layer.iMissionSubCategoryPicked));

                  missionx::addAdvancedSettingsPropertiesBeforeGeneratingRandomMission (); // v3.303.14


                  missionx::strct_generate_template_layer.selectedTemplateKey = mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI ();
                  this->set_bottom_message_line1 ("Generating mission is in progress, please wait...", 10);

                  ImGui::CloseCurrentPopup ();
                  this->execAction (mx_window_actions::ACTION_GENERATE_RANDOM_MISSION);
                }
                ImGui::SetItemDefaultFocus ();
                ImGui::SameLine (modal_center.x * 1.40f);
                // back button
                if (ImGui::Button ("Back", ImVec2 (80, 0)))
                {
                  ImGui::CloseCurrentPopup ();
                }
                this->mxUiReleaseLastFont ();
              }

              ImGui::EndPopup ();
            } // END POPUP ILS
          }
          ImGui::PopStyleColor ();
        } // end for iteration loop
      }

      ImGui::EndTable ();
    } // END ImGui::BeginTable

    ImGui::PopStyleColor ();
  }
  // ImGui::EndChild ();
  ImGui::EndGroup ();

  this->mxUiReleaseLastFont ();

  // END DRAW ILS SCREEN
}


// ----------------------------

void ui_nav_screen::child_draw_nav_search()
{
   const auto win_size_vec2 = this->mxUiGetWindowContentWxH ();

  ImGui::BeginGroup ();
  ImGui::PushItemWidth (100.0f);
  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
  {
    ImGui::TextColored (missionx::color::color_vec4_aqua, "%s", "Enter airport ICAO:");
    ImGui::SameLine ();
    if (ImGui::InputText ("##NavInput", missionx::strct_ils_layer.buf1, 8, ImGuiInputTextFlags_CharsUppercase)) // , ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal);
    {
      missionx::strct_ils_layer.sNavICAO = std::string (missionx::strct_ils_layer.buf1);
    }
    this->mx_add_tooltip (missionx::color::color_vec4_yellow, "Enter airport name to fetch its data.");
  }
  this->mxUiReleaseLastFont (); // release text regular

  // ---------------
  //  Row 1 - Search
  // ---------------


  // search button
  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  ImGui::SameLine ();
  if (ImGui::Button (">> Search <<"))
  {
    if (missionx::strct_ils_layer.fetch_metar_state == missionx::mxFetchState_enum::fetch_in_process)
    {
      this->set_bottom_message_line1 ("Active METAR fetch is in progress, please wait for it to finish or [Abort] the action.", DEFAULT_MESSAGE_TIME_I);
    }
    else
    {
      missionx::strct_ils_layer.sNavICAO = mxUtils::trim (missionx::strct_ils_layer.sNavICAO);

      if (missionx::strct_ils_layer.sNavICAO.empty ())
      {
        this->set_bottom_message_line1 ("Please enter a valid ICAO.", 6);
      }
      else
      {
        // get Navaid information
        missionx::strct_ils_layer.navaid = missionx::data_manager::getICAO_info (missionx::strct_ils_layer.sNavICAO);
        if (missionx::strct_ils_layer.navaid.navRef)
        {
          missionx::strct_ils_layer.fetch_nav_state = missionx::mxFetchState_enum::fetch_not_started;

          this->execAction (mx_window_actions::ACTION_FETCH_NAV_INFORMATION);
        }
        else
        {
          this->set_bottom_message_line1 ("Navaid: " + missionx::strct_ils_layer.sNavICAO + " is invalid.", 6);
        }
      }
    }
  }


  this->mxUiReleaseLastFont (); // release title regular
  ImGui::SameLine (0.0f, 5.0f);
  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  if (ImgWindow::ButtonTooltip (mxUtils::from_u8string (ICON_FA_TRASH_ALT).append ("##ClearNavICAO").c_str (), "Clear Nav ICAO"))
  {
    missionx::strct_ils_layer.sNavICAO.clear ();
    memset (missionx::strct_ils_layer.buf1, 0, sizeof missionx::strct_ils_layer.buf1);
  }

  // v24.03.2 abort button
  if (missionx::strct_ils_layer.fetch_metar_state == missionx::mxFetchState_enum::fetch_in_process)
  {
    ImGui::SameLine (0.0f, 20.0f);
    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
    ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_white);
    if (ImGui::Button (" Abort Metar Request "))
    {
      missionx::data_manager::threadStateMetar.flagAbortThread = true;
      this->set_bottom_message_line1 ("Trying to abort Metar fetch thread. Give it a few seconds to be released.", 8);
    }
    ImGui::PopStyleColor (3);
  }


  this->mxUiReleaseLastFont ();


  ImGui::EndGroup ();

  ImGui::Separator ();

  // ---------------
  //  Row 2 - Nav Data to display
  // ---------------


  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
  ImGui::BeginGroup ();
  ImGui::BeginChild ("draw_nav_info_output", ImVec2 (0.0f, win_size_vec2.y - 55.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar);
  {
    if (!missionx::strct_ils_layer.mapNavaidData.empty () && missionx::strct_ils_layer.fetch_nav_state != missionx::mxFetchState_enum::fetch_in_process
        /*&& missionx::data_manager::s_thread_sync_mutex.locked_by_caller() == false*/)
    {
      for (const auto &data : missionx::strct_ils_layer.mapNavaidData | std::views::values)
      {
        ////////////////////
        // Print ICAO title
        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_BIG ());
        {

          this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_MED ());
          {
            ImGui::TextColored (missionx::color::color_vec4_orange, "%s, %.0fnm", data.sApDesc.c_str (), data.dDistance);
            ImGui::SameLine ();
            this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
            ImGui::TextColored (missionx::color::color_vec4_dimgray, "(icao_id: %i)", data.icao_id);
            this->mxUiReleaseLastFont ();
          }
          this->mxUiReleaseLastFont ();

          ///////////////////////////////////
          // v24.03.1 METAR information
          this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
          this->mxUiHelpMarker (missionx::color::color_vec4_greenyellow, "Place, Day+Time, COR/AUTO/NIL, Wind, Visibility, Weather, Clouds, Temperature, Air-Pressure, Trend\n\nhttps://metar-taf.com/explanation\nhttps://www.flightutilities.com/MRonline.aspx\n\nMETAR data is downloaded and dependent on flightplandatabase.com availability.\nThe site limits the number of anonymous requests to ~100 per hour, You can extend it to ~1000 if you will add your \"API authorization key\" in the External FPLN screen.");
          this->mxUiReleaseLastFont ();

          if (missionx::strct_ils_layer.fetch_metar_state >= missionx::mxFetchState_enum::fetch_in_process)
          {
            ImGui::SameLine ();
            this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
            if (missionx::strct_ils_layer.fetch_metar_state == missionx::mxFetchState_enum::fetch_in_process)
              ImGui::TextColored (missionx::color::color_vec4_dimgray, "%s", "Fetching METAR... Please wait (will try 3 times with 5 sec sleep interval)." );
            else
            {
              ImGui::TextColored (missionx::color::color_vec4_dimgray, "%s", (data.sMetar.empty ()) ? "No METAR data was found." : data.sMetar.c_str () );
              if (!data.sTaf.empty ())
                ImGui::TextColored (missionx::color::color_vec4_dimgray, "Taf: %s", data.sTaf.c_str () );
            }
            // ImGui::TextColored (missionx::color::color_vec4_dimgray, "%s", ((missionx::strct_ils_layer.fetch_metar_state == missionx::mxFetchState_enum::fetch_in_process) ? "Fetching METAR... Please wait (will try 3 times with 5 sec sleep interval)." : (data.sMetar.empty ()) ? "No METAR data was found." : data.sMetar.c_str ()));
            this->mxUiReleaseLastFont ();
          }


          ///////////////////////////////////
          // v24.03.1 Frequencies information
          if (!data.listFrq.empty ())
          {
            ImGui::Spacing ();
            ImGui::Spacing ();

            // title
            ImGui::TextColored (missionx::color::color_vec4_beige, "%s", "Tower Frequencies");

            this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_MED ());
            {
              bool nonEven = true;
              for (const auto &rw : data.listFrq)
              {

                ImGui::TextColored (missionx::color::color_vec4_navajowhite, "%s", rw.field2.c_str ());
                if (nonEven)
                  ImGui::SameLine (win_size_vec2.x * 0.5f);

                nonEven ^= 1; // toggle
              }

              if (!nonEven)
                ImGui::NewLine ();
            }
            this->mxUiReleaseLastFont ();
          }

          //////////////////////////////////
          // Runway information
          if (!data.mapRunwayData.empty ())
          {
            ImGui::NewLine ();
            ImGui::Spacing ();

            // title
            ImGui::TextColored (missionx::color::color_vec4_beige, "%s", "Runways");

            this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_MED ());
            {
              for (const auto &[key, rw] : data.mapRunwayData)
              {
                ImGui::TextColored (missionx::color::color_vec4_lemonchiffon, "%s", rw.desc.c_str ());
                ImGui::SameLine ();
                ImGui::TextColored (missionx::color::color_vec4_dimgray, "(%.0fnm)", rw.dDistance);
              }
            }
            this->mxUiReleaseLastFont ();
          }

          // Nearby Navigation aids information
          if (!data.listVor.empty ())
          {
            ImGui::NewLine ();
            ImGui::Spacing ();

            // title
            ImGui::TextColored (missionx::color::color_vec4_beige, "%s", "Navigation Aids");

            this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_SMALL ());
            {
              ImGui::TextColored (missionx::color::color_vec4_grey, "%s", "{ident} {frq}  ({brg. ap},{type}) {name} {dist. ap} ");
            }
            this->mxUiReleaseLastFont ();

            this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
            {
              bool nonEven = true;
              for (const auto &loc : data.listVor)
              {
                constexpr const int SPACING_FOR_NAVAIDS = 60;
                ImGui::TextColored (missionx::color::color_vec4_cornsilk, "%s", loc.field1.c_str ());
                ImGui::SameLine (((nonEven) ? SPACING_FOR_NAVAIDS : (win_size_vec2.x * 0.5f) + SPACING_FOR_NAVAIDS - 2)); // absolute position
                ImGui::TextColored (missionx::color::color_vec4_cornsilk, "%s", loc.field2.c_str ());
                ImGui::SameLine ();
                ImGui::TextColored (missionx::color::color_vec4_dimgray, "(%.2fnm)", loc.dDistance);
                if (nonEven)
                  ImGui::SameLine (win_size_vec2.x * 0.5f);

                nonEven ^= 1; // toggle
              }

              if (!nonEven)
                ImGui::NewLine ();
            }
            this->mxUiReleaseLastFont ();
          }

          // Localizer information
          if (!data.listLoc.empty ())
          {
            ImGui::NewLine ();
            ImGui::Spacing ();

            // title
            ImGui::TextColored (missionx::color::color_vec4_beige, "%s", "Localizers");

            this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v24.06.1
            {
              ImGui::TextColored (missionx::color::color_vec4_yellowgreen, "%s", "Will display ILS and LOC first and then the other localizer types."); // v24.06.1
            }
            this->mxUiReleaseLastFont (); // v24.06.1


            this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_MED ());
            {
              // v24.06.1 visual separation between standard localizer frequencies and other type of navigation frequencies (LPV, VOR, DME).
              int loc_type                      = 1; // 1 = ILS or LOC, if it is not equal to "one", then it is
              int flag_loc_type_changed_x_times = 0; // I'll add "1" everytime I'll find a loc_type that is not equal to "one".

              for (const auto &loc : data.listLoc)
              {
                // v24.06.1 special separation logic
                if (loc.iField1 != 1)
                  ++flag_loc_type_changed_x_times;

                if (flag_loc_type_changed_x_times == 1)
                {
                  ImGui::Separator ();
                  ImGui::Separator ();
                  ImGui::Spacing ();
                }

                ImGui::TextColored (missionx::color::color_vec4_navajowhite, "%s", loc.field2.c_str ());
                ImGui::SameLine ();
                ImGui::TextColored (missionx::color::color_vec4_dimgray, "(%.0fnm)", loc.dDistance);
                ImGui::Separator ();
              }
            }
            this->mxUiReleaseLastFont ();
          }

          this->mxUiReleaseLastFont (); // release the TITLE BIG font
        }

        ImGui::Spacing ();
        ImGui::Separator ();
        ImGui::Separator ();
        ImGui::Separator ();
        ImGui::Spacing ();
      } // end loop over Nav Data Map


    } // end if async Query was done and we have Nav data
  }
  ImGui::EndChild ();
  ImGui::EndGroup ();
  this->mxUiReleaseLastFont ();
}


// ----------------------------


int ui_nav_screen::add_ui_two_option_buttons(bool& bOptA, bool& bOptB, const int& returnValueForA, const int& returnValueForB)
{
  // Default colors are for the IFR
  const ImVec4 picked_color = missionx::color::color_vec4_green;
  const ImVec4 unpicked_color = missionx::color::color_vec4_gray;

  ImVec4 option_A_button_color = picked_color;
  ImVec4 option_B_button_color = unpicked_color;

  if (missionx::strct_ils_layer.isVFR)
  {
    option_A_button_color = unpicked_color;
    option_B_button_color = picked_color;
  }

  ImGui::PushStyleColor (ImGuiCol_Button, option_A_button_color);
  if (ImGui::Button (" IFR "))
  {
    bOptA = true;
    bOptB = false;
  }
  ImGui::PopStyleColor (1);

  ImGui::SameLine (0.0f, 5.0f);

  ImGui::PushStyleColor (ImGuiCol_Button, option_B_button_color);
  if (ImGui::Button (" VFR "))
  {
    bOptA = false;
    bOptB = true;
  }
  ImGui::PopStyleColor (1);

  // We have to return a value even if no button was pressed
  if (bOptA)
    return returnValueForA;

  return returnValueForB;
}


// ----------------------------


void ui_nav_screen::add_ui_ils_vfr_search_airports_button(missionx::mx_window_actions inActionToExecute)
{
    constexpr static auto lbl = "Search for airports based on user pref.";
  int style_i = 0;

  ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow);
  style_i++;
  ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_indigo);
  style_i++;

  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
  ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_orange);
  ImGui::PushStyleColor (ImGuiCol_ButtonActive, missionx::color::color_vec4_azure);
  if (ImGui::Button (lbl))
  {

    // select icao, round(distance_nm) as distance_nm, loc_rw, loc_type, frq_mhz, loc_bearing, rw_length_mt, rw_width, ap_elev, ap_name, surf_type_text, bearing_from_to_icao
    // from (
    // select xp_loc.icao
    //       , mx_calc_distance ({}, {}, xp_loc.lat, xp_loc.lon, 3440) as distance_nm
    //       , xp_loc.loc_rw, xp_loc.loc_type, xp_loc.frq_mhz, xp_loc.loc_bearing, xp_rw.rw_length_mt, xp_rw.rw_width, xa.ap_elev, xa.ap_name
    //       , case xp_rw.rw_surf when 1 then 'Asphalt' when 2 then 'Concrete' when 3 then 'Turf or grass' when 4 then 'Dirt' when 5 then 'Gravel' when 12 then 'Dry lakebed' when 13 then 'Water runways' when 14 then 'Snow or ice' when 15 then 'Transparent' else 'other' end as surf_type_text
    //       , mx_bearing({}, {}, xp_loc.lat, xp_loc.lon) as bearing_from_to_icao
    // from xp_loc, xp_rw, xp_airports xa
    // where xp_rw.icao = xp_loc.icao
    // and (xp_rw.rw_no_1 = xp_loc.loc_rw or xp_rw.rw_no_2 = xp_loc.loc_rw)
    // and xa.icao = xp_rw.icao
    //)
    // where 1 = 1


    // v24.03.1 The search code is split into two parts, the filter is constructed from the UI and the base query is provided in the "data_manager::fetch_ils_rw_from_sqlite()" function.
    std::string sql_filter = " and rw_length_mt >= " + mxUtils::formatNumber<int> (strct_ils_layer.slider_min_rw_length_i); // " and rw_length_mt >= 1000 "
    sql_filter += " and rw_width >= " + mxUtils::formatNumber<int> (strct_ils_layer.slider_min_rw_width_i); // " and rw_width >= 45 "
    sql_filter += " and ap_elev >= " + mxUtils::formatNumber<int> (strct_ils_layer.slider_min_airport_elev_ft_i); // " and ap_elev >= 0 "
    if (!missionx::strct_ils_layer.to_icao.empty ()) // v24.03.1 add the TO
      sql_filter += fmt::format (" and icao like '%{}%' ", missionx::strct_ils_layer.to_icao);


    // v25.08.1 added VFR filter
    if (missionx::strct_ils_layer.get_ils_types_picked ().empty () || data_manager::ui_ifr_or_vfr_i == missionx::PICKED_VFR)
      sql_filter += " ";
    else
      sql_filter += " and lower(loc_type) in ( " + missionx::strct_ils_layer.get_ils_types_picked () + " )"; //

    if (!missionx::strct_ils_layer.flagIgnoreDistanceFilter) // v24.03.1
      sql_filter += " and distance_nm between " + mxUtils::formatNumber<float> (strct_ils_layer.ils_sliderVal1, 0) + " and " + mxUtils::formatNumber<float> (strct_ils_layer.ils_sliderVal2, 0); // " and distance between 50 and 100 "

    sql_filter += fmt::format (" ORDER BY distance_nm LIMIT {} ", mx_ils_layer::limit_items[missionx::strct_ils_layer.limit_indx]); // v24.03.1 We always limit rows, for UI performance reasons


    missionx::strct_ils_layer.filter_query_s = sql_filter; // v24.03.1 store the final filter for the thread use.

    if (missionx::strct_ils_layer.from_icao.empty ())
    {
      #ifdef IBM
      missionx::strct_ils_layer.navaid = data_manager::get_plane_airport_or_nearest_icao ();
      #else
      auto tempNav                 = data_manager::get_plane_airport_or_nearest_icao ();
      missionx::strct_ils_layer.navaid = tempNav;
      #endif
    }

    if ((!missionx::strct_ils_layer.from_icao.empty ()) && (missionx::strct_ils_layer.navaid.getID ().empty () || missionx::strct_ils_layer.navaid.lat == 0 || missionx::strct_ils_layer.navaid.lon == 0))
    {
      #ifdef IBM
      missionx::strct_ils_layer.navaid = data_manager::getICAO_info (missionx::strct_ils_layer.from_icao);
      #else
      auto tempNav                 = data_manager::getICAO_info (missionx::strct_ils_layer.from_icao);
      missionx::strct_ils_layer.navaid = tempNav;
      #endif
    }

    // last validation
    if (missionx::strct_ils_layer.navaid.getID ().empty ()) // if navaid ID is still empty then pick plane position
    {
      this->set_bottom_message_line1 ("Could not initialize starting ICAO. Please consider entering it manually.", missionx::DEFAULT_MESSAGE_TIME_I);
    }
    else
    {
      missionx::strct_ils_layer.fetch_ils_state  = missionx::mxFetchState_enum::fetch_not_started;
      missionx::flag_generatedRandomFile_success = false; // reset state if already generated information. Will hide Start button until next mission generated.

      // this->execAction (missionx::mx_window_actions::ACTION_FETCH_ILS_AIRPORTS);
      this->execAction (inActionToExecute);
    }
  }
  ImGui::PopStyleColor (3);

  this->mxUiReleaseLastFont ();
  ImGui::PopStyleColor (style_i);

}

// ----------------------------


void ui_nav_screen::callNavData(std::string_view inICAO, bool bNavigatingFromOtherLayer)
{
  if (missionx::strct_ils_layer.fetch_nav_state != missionx::mxFetchState_enum::fetch_in_process)
  {
    missionx::strct_ils_layer.sNavICAO = inICAO;

    auto size = sizeof missionx::strct_ils_layer.buf1;
    #ifdef IBM
    strncpy_s (missionx::strct_ils_layer.buf1, sizeof missionx::strct_ils_layer.buf1, inICAO.data (), sizeof missionx::strct_ils_layer.buf1);
    #else
    strncpy (missionx::strct_ils_layer.buf1, inICAO.data (), sizeof (missionx::strct_ils_layer.buf1) - 1);
    #endif
    missionx::strct_ils_layer.flagForceNavDataTab = true;

    missionx::strct_ils_layer.fetch_nav_state = missionx::mxFetchState_enum::fetch_not_started;
    this->execAction (mx_window_actions::ACTION_FETCH_NAV_INFORMATION);

    if (bNavigatingFromOtherLayer)
      this->execAction (missionx::mx_window_actions::ACTION_OPEN_NAV_LAYER);
  }
  else
    this->set_bottom_message_line1 ("A previous request is running in the background.", DEFAULT_MESSAGE_TIME_I);
}

// ----------------------------
// ----------------------------
// ----------------------------
// ----------------------------
// ----------------------------
// ----------------------------

} // missionx