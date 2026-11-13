//
// v26.10.2 ui_stats_screen: draws the stats of one flight (one stats database file).
//          Used by the end screen (the live stats file) and by the stats screen (history flights, one or two side by side).
//          The draw functions are static and read only their parameters: no mission globals, no window members.
//

#include "ui_stats_screen.h"
#include <IconsFontAwesome5.h> // inside libs/imgui4xp
#include <algorithm> // v26.10.2
#include <array>
#include <cmath>
#include <ranges>
#include <cstdio> // v26.10.2 snprintf (3D axis format)
#include <ctime>  // v26.10.2 in-flight stats copy time
#include <future> // v26.10.2 std::async (flight path fetch)
#include <memory>

namespace missionx
{

// -------------------------------------------
// v26.10.2 All the stats of one flight, top to bottom: summary (time flew, min/max, scores, runway list), legs, landing analysis, graph.
//          The ImGui ID stack gets in_draw_options.id_suffix, so two flights can be drawn in the same window.
void
ui_stats_screen::draw_flight_stats (const missionx::mx_stats_view_data &in_view_data, const mx_stats_draw_options &in_draw_options)
{
  ImGui::PushID (in_draw_options.id_suffix.c_str ());

  // v26.10.2 focus view: only the flight path, filling the screen
  if (in_draw_options.show_graph && in_draw_options.path_view != nullptr && in_draw_options.path_view->flag_focus_view)
  {
    draw_flight_path_section (in_view_data, *in_draw_options.path_view, in_draw_options.id_suffix, in_draw_options.path_line_color, in_draw_options.path_plot_height);
    ImGui::PopID ();
    return;
  }

  const float column_width = (in_draw_options.column_width > 0.0f) ? in_draw_options.column_width : mxUiGetContentWidth ();

  if (in_draw_options.show_summary)
    draw_summary (in_view_data, column_width);

  if (in_draw_options.show_legs)
  {
    const auto &vecLegs = (in_draw_options.fallback_legs != nullptr) ? *in_draw_options.fallback_legs : in_view_data.vecLegs;
    ImGui::BeginGroup ();
    draw_legs (vecLegs);
    ImGui::EndGroup ();
  }

  if (in_draw_options.show_landing_analysis)
    draw_landing_analysis (in_view_data, in_draw_options.fallback_max_weight_kg);

  if (in_draw_options.show_graph)
  {
    if (in_draw_options.path_view != nullptr) // v26.10.2 flight path plots, the waypoint list includes "Full flight"
      draw_flight_path_section (in_view_data, *in_draw_options.path_view, in_draw_options.id_suffix, in_draw_options.path_line_color, in_draw_options.path_plot_height);
    else
      draw_graph (in_view_data, in_draw_options.graph_child_size);
  }

  ImGui::PopID ();
}

// -------------------------------------------
// v26.10.2 Time flew, min/max pitch, roll and G with their scores, the total score, and the runway list (center line score, bounces).
//          Moved from WinImguiBriefer::child_flight_leg_info_draw_end_summary().
void
ui_stats_screen::draw_summary (const missionx::mx_stats_view_data &in_view_data, const float column_width)
{
  missionx::mx_img_window::mxUiSetFont (mxconst::get_TEXT_TYPE_DEFAULT_PLUS_1 ()); // v3.303.14

  ImGui::BeginGroup ();
  ImGui::TextColored (missionx::color::color_vec4_yellowgreen, "Time Flew: ");
  ImGui::SameLine ();
  ImGui::TextColored (missionx::color::color_vec4_white, "%s", in_view_data.stats.time_flew_format_s.c_str ());

  // 3.303.8.3 show min/max stats
  const auto fThird  = column_width * 0.333f;
  const auto f2Third = column_width * 0.666f;

  if (in_view_data.flag_scoring_missing) // v26.10.2 was: the mission scoring global is empty
    ImGui::TextColored (missionx::color::color_vec4_orangered, "!!! No Scoring information is available !!!");

  ImGui::TextColored (missionx::color::color_vec4_yellowgreen, "Min Pitch: %.2f", in_view_data.stats.minPitch);
  ImGui::SameLine (fThird, 0.0f);
  ImGui::TextColored (missionx::color::color_vec4_yellowgreen, "Max Pitch: %.2f", in_view_data.stats.maxPitch);
  ImGui::SameLine (f2Third, 0.0f);
  ImGui::TextColored (missionx::color::color_vec4_beige, "Score: %.2f", in_view_data.stats.pitchScore);

  ImGui::TextColored (missionx::color::color_vec4_yellowgreen, "Min Roll: %.2f", in_view_data.stats.minRoll);
  ImGui::SameLine (fThird, 0.0f);
  ImGui::TextColored (missionx::color::color_vec4_yellowgreen, "Max Roll: %.2f", in_view_data.stats.maxRoll);
  ImGui::SameLine (f2Third, 0.0f);
  ImGui::TextColored (missionx::color::color_vec4_beige, "Score: %.2f", in_view_data.stats.rollScore);

  ImGui::TextColored (missionx::color::color_vec4_yellowgreen, "Min G: %.2f", in_view_data.stats.minGforce);
  ImGui::SameLine (fThird, 0.0f);
  ImGui::TextColored (missionx::color::color_vec4_yellowgreen, "Max G: %.2f", in_view_data.stats.maxGforce);
  ImGui::SameLine (f2Third, 0.0f);
  ImGui::TextColored (missionx::color::color_vec4_beige, "Score: %.2f", in_view_data.stats.gForceScore);

  ImGui::TextColored (missionx::color::color_vec4_beige, "Total Flight Score: %.2f", in_view_data.stats.pitchScore + in_view_data.stats.rollScore + in_view_data.stats.gForceScore);

  // min/max

  ImGui::Separator ();
  for (auto &land : in_view_data.stats.vecLandingStatsResults)
  {
    const bool flag_bounced_row = missionx::mx_img_window::mxStartUiDisableState (land.bounce == 1 && !land.did_landing_contained_bounce);
    {
      ImGui::Text ("%s", fmt::format("{}", (flag_bounced_row)? fmt::format ("\t{:>10}", "[" + land.activity_s + "]") : "Landed in: " ).c_str());
      ImGui::SameLine ();
      ImGui::TextColored (missionx::color::color_vec4_green, "%s", land.icao.c_str ());

      ImGui::SameLine ();
      ImGui::Text ("%s", ", Runway: ");
      ImGui::SameLine ();
      ImGui::TextColored (missionx::color::color_vec4_lightblue, "%s", land.runway_s.c_str ());

      ImGui::SameLine ();
      ImGui::Text ("%s", ", Accuracy from center: ");
      ImGui::SameLine ();
      ImGui::TextColored (missionx::color::color_vec4_beige, "%s", mxUtils::formatNumber<double> (land.distance_from_center_of_rw_d, 2).c_str ());

      // skip score for bounced data
      if (!flag_bounced_row)
      {
        ImGui::SameLine ();
        ImGui::Text ("%s", " => Score: ");
        ImGui::SameLine ();
        ImGui::TextColored (missionx::color::color_vec4_white, "%s", mxUtils::formatNumber<double> (land.score_centerLine, 4).c_str ());

        if (land.did_landing_contained_bounce)
        {
          ImGui::SameLine ();
          // ImGui::TextColored (missionx::color::color_vec4_orange, "%s", fmt::format("(bounced: {} time{})", land.bounce, (land.bounce == 1)? "" : "s").c_str ());
          ImGui::TextColored (missionx::color::color_vec4_orange, "%s", "(bounced)");
        }
      } // end if to display score + bounce info
    } // end enabled/disabled block
    missionx::mx_img_window::mxEndUiDisableState (flag_bounced_row);
  }
  ImGui::Separator ();

  ImGui::EndGroup ();

  missionx::mx_img_window::mxUiReleaseLastFont (); // v3.303.14
}

// -------------------------------------------
// v26.10.2 The legs, as listed in in_vec_legs (newest first). A leg that did not end shows its status.
//          Moved from WinImguiBriefer::add_ui_stats_child().
void
ui_stats_screen::draw_legs (const std::vector<missionx::mx_enrout_stats_strct> &in_vec_legs)
{
  missionx::mx_img_window::mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
  for (const auto &stats : in_vec_legs)
  {
    // const auto pctTdWeight_f = stats.fTouchDownWeight / stats.fTouchDown_acf_m_max;
    ImGui::TextColored (missionx::color::color_vec4_beige, "Waypoint Name: %s", stats.sLegName.c_str ());
    if (!stats.sStatus.empty () && stats.sStatus != mxconst::get_LEG_STATUS_COMPLETED ()) // v26.10.2 a leg that did not end: running, skipped, failure, aborted, stopped
      ImGui::TextColored (missionx::color::color_vec4_orange, "Leg not finished: %s", stats.sStatus.c_str ());
    ImGui::TextColored (missionx::color::color_vec4_white, "Overall Distance flew: %10.2f nm\tLeg Flew: %5.2f", stats.fCumulativeDistanceFlew_beforeCurrentLeg + stats.fDistanceFlew, stats.fDistanceFlew);
    ImGui::TextColored (missionx::color::color_vec4_grey, "Start Fuel: %5.2f\tEnd Fuel: %5.2f\tFuel Consumed: %5.2f", stats.fStartingFuel, stats.fEndFuel, stats.fStartingFuel - stats.fEndFuel);
    ImGui::TextColored (missionx::color::color_vec4_white, "Start Payload: %6.2f\tEnd Payload: %6.2f", stats.fStartingPayload, stats.fEndPayload);

    // add_landing_rate_ui (stats);

    ImGui::Separator ();
  }
  missionx::mx_img_window::mxUiReleaseLastFont ();
}

// -------------------------------------------
// v26.10.1 Landing analysis for each landing: grade line, touchdown summary, height-step table and the notes (yellow).
//          Display only. The data is prepared by the stats fetch thread (data_manager::fetch_landing_analysis()).
// v26.10.2 Moved from WinImguiBriefer::add_ui_landing_analysis().
void
ui_stats_screen::draw_landing_analysis (const missionx::mx_stats_view_data &in_view_data, const float in_fallback_max_weight_kg, const bool in_flag_natural_width_table)
{
  const auto &mapLandings = in_view_data.stats.mapLandingAnalysis;
  if (mapLandings.empty ())
    return;

  missionx::mx_img_window::mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
  for (const auto &[td_line_id, landing] : mapLandings)
    draw_one_landing (in_view_data, td_line_id, landing, in_fallback_max_weight_kg, in_flag_natural_width_table);
  missionx::mx_img_window::mxUiReleaseLastFont ();
}

// -------------------------------------------
// v26.10.2 One landing: grade line, touchdown summary, height-step table and the notes. The caller sets the font.
//          Split from draw_landing_analysis(), so the compare screen can draw landing N of both flights one after the other.
void
ui_stats_screen::draw_one_landing (const missionx::mx_stats_view_data &in_view_data, const int td_line_id, const missionx::mx_landing_analysis &landing, const float in_fallback_max_weight_kg, const bool in_flag_natural_width_table)
{
  // cell color limits (display only). Same limits as the notes in data_manager::build_landing_findings()
  constexpr double STEEP_GLIDE_RATIO    = 1.7;  // V/S above 1.7x the sink of a 3 degree path (steps at 50ft and above)
  constexpr double FINAL_BANK_LIMIT_DEG = 10.0; // bank above it, before touchdown
  constexpr double TD_BANK_LIMIT_DEG    = 5.0;  // bank above it, at touchdown
  constexpr int    TABLE_COLUMNS        = 11;

  const auto lmbda_grade_color = [](const missionx::mx_landing_analysis &in_landing) -> ImVec4
  {
    if (in_landing.is_doctor || in_landing.is_overrun || in_landing.is_excursion) // v26.10.2 off the runway
      return missionx::color::color_vec4_red;
    if (in_landing.grade_s == "Butter")
      return missionx::color::color_vec4_green;
    if (in_landing.grade_s == "Smooth")
      return missionx::color::color_vec4_greenyellow;
    if (in_landing.grade_s == "Acceptable")
      return missionx::color::color_vec4_yellow;
    if (in_landing.grade_s == "Firm")
      return missionx::color::color_vec4_orange;
    if (in_landing.grade_s == "Hard")
      return missionx::color::color_vec4_orangered;

    return missionx::color::color_vec4_white; // N/A
  };

  const auto lmbda_cell = [](const ImVec4 &in_color, const std::string &in_text)
  {
    ImGui::TableNextColumn ();
    ImGui::TextColored (in_color, "%s", in_text.c_str ());
  };

  ImGui::PushID (td_line_id); // unique table id per landing

  // ---- grade line: "Landing at KDEN, runway 7: Butter (score 90/100)" ----
  // v26.10.2 the runway end the plane landed from, when known ("7"). Else both ends ("7-25")
  std::string landed_at_s;
  for (const auto &land : in_view_data.stats.vecLandingStatsResults)
  {
    if (land.line_id == td_line_id)
      landed_at_s = fmt::format (" at {}, runway {}", land.icao, (landing.has_runway_data && !landing.landing_runway_s.empty ()) ? landing.landing_runway_s : land.runway_s);
  }

  ImGui::TextColored (missionx::color::color_vec4_beige, "Landing%s:", landed_at_s.c_str ());
  ImGui::SameLine ();
  ImGui::TextColored (lmbda_grade_color (landing), "%s", landing.grade_s.c_str ());
  ImGui::SameLine ();
  if (landing.is_crash) // v26.10.2 a crash during the landing or the rollout: no score
    ImGui::TextColored (missionx::color::color_vec4_white, "(not graded)");
  else if (landing.bounce_penalty < 0.0f || landing.is_overrun || landing.is_excursion) // v26.10.1 "(base 90  -5 bounce  (score 85/100)", the penalty in red. v26.10.2 "overrun" / "excursion" set the score to 0
  {
    ImGui::TextColored (missionx::color::color_vec4_white, "(base %.0f", landing.base_score);
    ImGui::SameLine ();
    if (landing.bounce_penalty < 0.0f)
    {
      ImGui::TextColored (missionx::color::color_vec4_red, "%.0f bounce", landing.bounce_penalty); // the value is negative, so it shows "-5"
      ImGui::SameLine ();
    }
    if (landing.is_overrun || landing.is_excursion) // v26.10.2
    {
      ImGui::TextColored (missionx::color::color_vec4_red, "%s", (landing.is_overrun) ? "overrun" : "excursion");
      ImGui::SameLine ();
    }
  }
  if (!landing.is_crash) // v26.10.2
    ImGui::TextColored (missionx::color::color_vec4_white, "(score %.0f/100)", landing.landing_score);

  // ---- touchdown summary line ----
  if (landing.has_summary)
  {
    std::string summary_s = fmt::format ("Touchdown: {:.0f} fpm", landing.td_fpm);
    // v26.10.2 where on the runway: "177 m past the threshold (274 m of runway left)"
    if (landing.has_runway_data)
    {
      if (landing.td_from_threshold_m < 0.0)
        summary_s += fmt::format (", {:.0f} m before the displaced threshold ({:.0f} m of runway left)", -landing.td_from_threshold_m, landing.runway_left_m);
      else
        summary_s += fmt::format (", {:.0f} m past the threshold ({:.0f} m of runway left)", landing.td_from_threshold_m, landing.runway_left_m);
    }
    if (landing.has_gear_load)
      summary_s += fmt::format (", gear load {:.2f} (G {:.2f} avg, {:.2f} peak)", landing.td_gear_load, landing.td_g_avg, landing.td_g_peak);
    // v26.10.2 the steepest moment of the final, next to the sink of a normal (3 degree) approach at the same groundspeed
    if (landing.has_final_glide && landing.final_steep_normal_fpm > 0.0)
      summary_s += fmt::format (", steepest final {:.0f} fpm (normal ~{:.0f})", landing.final_steep_sink_fpm, landing.final_steep_normal_fpm);
    else if (landing.has_final_glide)
      summary_s += fmt::format (", steepest final {:.1f} times a normal approach sink", landing.final_max_glide_ratio);
    if (landing.bounce_count > 0) // v26.10.1
      summary_s += fmt::format (", bounced {} {} (max {:.1f} ft)", landing.bounce_count, (landing.bounce_count == 1) ? "time" : "times", landing.max_bounce_agl_ft);
    // v26.10.2 the max weight of the plane flown at this touchdown (plane_meta). Not known: the fallback value
    float max_weight_kg = in_view_data.get_max_weight_kg_at_line_id_cai (td_line_id);
    if (max_weight_kg <= 0.0f)
      max_weight_kg = in_fallback_max_weight_kg;

    if (max_weight_kg > 0.0f)
      summary_s += fmt::format (", weight {:.0f} kg of {:.0f} kg max ({:.0f}%)", landing.td_weight_kg, max_weight_kg, (landing.td_weight_kg / max_weight_kg) * 100.0 );
    else
      summary_s += fmt::format (", weight {:.0f} kg", landing.td_weight_kg);

    ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellowgreen);
    ImGui::TextWrapped("%s", summary_s.c_str());
    ImGui::PopStyleColor();
  }

  // ---- height-step table ----
  // in_flag_natural_width_table: the table keeps the width of its columns (a narrow column scrolls sideways) instead of squeezing them
  const ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings | ((in_flag_natural_width_table) ? ImGuiTableFlags_NoHostExtendX : ImGuiTableFlags_None);
  if (!landing.vecSteps.empty () && ImGui::BeginTable ("landing_steps_table", TABLE_COLUMNS, table_flags))
  {
    ImGui::TableSetupColumn ("Step");
    ImGui::TableSetupColumn ("AGL ft");
    ImGui::TableSetupColumn ("IAS kt");
    ImGui::TableSetupColumn ("GS kt");
    ImGui::TableSetupColumn ("V/S fpm");
    ImGui::TableSetupColumn ("Pitch");
    ImGui::TableSetupColumn ("Bank");
    ImGui::TableSetupColumn ("G");
    ImGui::TableSetupColumn ("Flaps %");
    ImGui::TableSetupColumn ("Time to TD(s)");
    ImGui::TableSetupColumn ("Distance to TD(m)");
    ImGui::TableHeadersRow ();

    for (const auto &row : landing.vecSteps)
    {
      ImGui::TableNextRow ();
      const bool is_td_row = (row.step_agl_ft <= 0.0); // the "TD" row has step height 0

      lmbda_cell (missionx::color::color_vec4_aqua, row.step_label);

      // no own sample at this height (1 Hz data). The step below shows the same stats row
      if (row.same_row_as_next)
      {
        for (int col = 1; col < TABLE_COLUMNS; ++col)
          lmbda_cell (missionx::color::color_vec4_grey, "-");
        continue;
      }

      // V/S: orangered at touchdown when "Call the Doctor". Orange when steeper than STEEP_GLIDE_RATIO x a 3 degree path (50ft and above)
      ImVec4 vs_color = missionx::color::color_vec4_white;
      if (is_td_row && landing.is_doctor)
        vs_color = missionx::color::color_vec4_orangered;
      else if (!is_td_row && row.step_agl_ft >= 50.0 && row.gs_kt > 20.0 && std::abs (row.vs_fpm) > row.gs_kt * 5.3 * STEEP_GLIDE_RATIO)
        vs_color = missionx::color::color_vec4_orange;

      // bank: orange above the limit (lower limit at touchdown)
      const double bank_limit = (is_td_row) ? TD_BANK_LIMIT_DEG : FINAL_BANK_LIMIT_DEG;
      const ImVec4 bank_color = (std::abs (row.roll) > bank_limit) ? missionx::color::color_vec4_orange : missionx::color::color_vec4_white;

      // pitch: orange for a nose low touchdown
      const ImVec4 pitch_color = (is_td_row && row.pitch < 0.0) ? missionx::color::color_vec4_orange : missionx::color::color_vec4_white;

      lmbda_cell (missionx::color::color_vec4_white, fmt::format ("{:.0f}", row.agl_ft));
      lmbda_cell (missionx::color::color_vec4_white, fmt::format ("{:.0f}", row.ias_kt));
      lmbda_cell (missionx::color::color_vec4_white, fmt::format ("{:.0f}", row.gs_kt));
      lmbda_cell (vs_color, fmt::format ("{:.0f}", row.vs_fpm));
      lmbda_cell (pitch_color, fmt::format ("{:.1f}", row.pitch));
      lmbda_cell (bank_color, fmt::format ("{:.1f}", row.roll));
      lmbda_cell (missionx::color::color_vec4_white, fmt::format ("{:.2f}", row.g));
      lmbda_cell (missionx::color::color_vec4_white, fmt::format ("{:.0f}", row.flap_ratio * 100.0));
      lmbda_cell (missionx::color::color_vec4_white, fmt::format ("{:.1f}", row.sec_to_td));
      lmbda_cell (missionx::color::color_vec4_white, fmt::format ("{:.0f}", row.dist_to_td_mt));
    }

    ImGui::EndTable ();
  }

  // ---- notes (yellow, wrapped) ----
  ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow);
  for (const auto &note : landing.vecFindings)
    ImGui::TextWrapped ("- %s", note.c_str ());
  ImGui::PopStyleColor ();

  ImGui::Separator ();
  ImGui::PopID ();
}

// -------------------------------------------
// v26.10.2 Elevation and airspeed over the flight (ImPlot). Moved from WinImguiBriefer::child_flight_leg_info_draw_end_summary().
void
ui_stats_screen::draw_graph (const missionx::mx_stats_view_data &in_view_data, const ImVec2 &in_child_size)
{
  ////// Draw stats graph using ImPlot
  ImGui::BeginGroup ();
  ImGui::BeginChild ("child_statistics", in_child_size, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar); // draw on the all the remaining window area

  if (ImPlot::BeginPlot ("Mission Stats", ImVec2 (-1, 0))) //, ImPlotFlags_YAxis2) )
  {
    const auto &st = in_view_data.stats; // v26.10.1 decimated plot series

    // v26.10.2 X = flight time in minutes (was the row number: rows are written faster around takeoffs and landings, so it stretched them)
    const double max_minutes = std::max (1.0, st.total_elapsed_min);

    ImPlot::SetupAxis (ImAxis_X1, "Flight time (min)", ImPlotAxisFlags_Lock);
    ImPlot::SetupAxisLimits (ImAxis_X1, 0.0, max_minutes, ImPlotCond_Always);

    ImPlot::SetupAxis (ImAxis_Y1, "Elevation ft", ImPlotAxisFlags_Lock);
    ImPlot::SetupAxisLimits (ImAxis_Y1, 0.0, st.max_elev_ft + 1000.0, ImPlotCond_Always);

    ImPlot::SetupAxis (ImAxis_Y2, "Airspeed kt", ImPlotAxisFlags_AuxDefault | ImPlotAxisFlags_Lock);
    ImPlot::SetupAxisLimits (ImAxis_Y2, 0.0, st.max_airspeed + 100.0, ImPlotCond_Always);

    ImPlot::SetAxes (ImAxis_X1, ImAxis_Y1);
    ImPlot::PlotLine ("Elevation ft", st.vecPlotElevMin_d.data (), st.vecPlotElev_ft_d.data (), static_cast<int> (st.vecPlotElevMin_d.size ()));

    ImPlot::SetAxes (ImAxis_X1, ImAxis_Y2);
    ImPlot::PlotLine ("Air Speed", st.vecPlotAirMin_d.data (), st.vecPlotAirSpeed_d.data (), static_cast<int> (st.vecPlotAirMin_d.size ()));
    ImPlot::EndPlot ();
  }

  ImGui::EndChild ();
  ImGui::EndGroup ();
}

// -------------------------------------------
// v26.10.2 Flight path plots (2D/3D). One part of the flight, picked by waypoint and range. Steps 16b, 16c.
//          ImPlot3D and ImPlot mouse input is off: X-Plane takes the right button (window move) and the wheel scrolls the page.
//          Left drag rotates the 3D plot, the buttons zoom and pick a view.
// -------------------------------------------

// The waypoint list belongs to one flight (file + flight_id). A new flight (or a new end screen) rebuilds it and drops the cached sections.
void
ui_stats_screen::sync_flight_path_sections (const missionx::mx_stats_view_data &in_view_data, missionx::mx_flight_path_view_state &inout_state)
{
  const std::string flight_key = in_view_data.stats_db_path + "|" + in_view_data.flight_info.flight_id + "|" + std::to_string (in_view_data.stats.vecSeq_d.size ());
  if (inout_state.flight_key == flight_key && !inout_state.vecSections.empty ())
    return;

  inout_state.reset (); // a new flight: back to the small plot, default view
  inout_state.flight_key      = flight_key;
  inout_state.vecSections     = missionx::data_manager::build_flight_path_sections_cai (in_view_data);

  // default: the first entry ("Departure"), up to 5 nm
  inout_state.selected_section = 0;
  inout_state.range            = missionx::mx_path_range_enum::up_to_5nm;
}

// -------------------------------------------
// The ranges a section offers. Departure: no "whole leg". Full flight: only the whole flight.
void
ui_stats_screen::normalize_flight_path_range (missionx::mx_flight_path_view_state &inout_state)
{
  if (inout_state.vecSections.empty ())
    return;

  inout_state.selected_section = std::clamp (inout_state.selected_section, 0, static_cast<int> (inout_state.vecSections.size ()) - 1);
  const auto section_type      = inout_state.vecSections[inout_state.selected_section].type;

  if (section_type == missionx::mx_path_section::type_enum::full_flight)
    inout_state.range = missionx::mx_path_range_enum::whole_leg;
  else if (section_type == missionx::mx_path_section::type_enum::departure && inout_state.range == missionx::mx_path_range_enum::whole_leg)
    inout_state.range = missionx::mx_path_range_enum::up_to_5nm;
}

// -------------------------------------------
// The cached section, or start its background fetch. nullptr when there are no sections.
missionx::mx_flight_path_cache_entry *
ui_stats_screen::get_or_fetch_flight_path (const missionx::mx_stats_view_data &in_view_data, missionx::mx_flight_path_view_state &inout_state)
{
  if (inout_state.vecSections.empty ())
    return nullptr;

  const int cache_key = inout_state.get_cache_key_cai ();
  if (const auto found = inout_state.mapCache.find (cache_key); found != inout_state.mapCache.end ())
    return found->second.get ();

  auto entry                      = std::make_shared<missionx::mx_flight_path_cache_entry> ();
  entry->fetch_state              = missionx::mxFetchState_enum::fetch_in_process;
  inout_state.mapCache[cache_key] = entry;

  const std::string                  stats_db_path = in_view_data.stats_db_path;
  const missionx::mx_path_section    section       = inout_state.vecSections[inout_state.selected_section];
  const missionx::mx_path_range_enum range         = inout_state.range;

  // the lambda holds the entry: it stays alive until the fetch ends, even when the cache is cleared
  missionx::data_manager::mFetchFutures.push_back (std::async (std::launch::async, [entry, stats_db_path, section, range] () {
    missionx::data_manager::fetch_flight_path_section_cai (stats_db_path, section, range, &entry->path, &entry->fetch_state, &entry->fetch_msg);
  }));

  return entry.get ();
}

// -------------------------------------------
// The section ready to draw (the horizontal nm made once).
// While a new section is read, the last section drawn is returned (out_flag_reading = true), so the plot stays and the page keeps its height:
// a shorter page would move the scroll position up ("the viewport jumps").
// nullptr: nothing to draw. The status text is drawn in a space of in_reserved_height for the same reason.
missionx::mx_flight_path_cache_entry *
ui_stats_screen::get_ready_flight_path_or_draw_status (const missionx::mx_stats_view_data &in_view_data, missionx::mx_flight_path_view_state &inout_state, const float in_reserved_height, bool *out_flag_reading)
{
  if (out_flag_reading != nullptr)
    *out_flag_reading = false;

  const auto lmbda_draw_status = [in_reserved_height] (const ImVec4 &in_color, const std::string &in_text) {
    ImGui::TextColored (in_color, "%s", in_text.c_str ());
    ImGui::Dummy (ImVec2 (0.0f, std::max (0.0f, in_reserved_height - ImGui::GetTextLineHeightWithSpacing ())));
  };

  // the horizontal nm, once for each entry
  const auto lmbda_prepare_nm = [] (missionx::mx_flight_path_cache_entry &inout_entry) {
    if (inout_entry.vecEast_nm.size () == inout_entry.path.vecEast_ft.size ())
      return;
    constexpr double FEET_PER_NM = static_cast<double> (missionx::nm2meter) * static_cast<double> (missionx::meter2feet);
    inout_entry.vecEast_nm.clear ();
    inout_entry.vecNorth_nm.clear ();
    for (size_t i = 0; i < inout_entry.path.vecEast_ft.size (); ++i)
    {
      inout_entry.vecEast_nm.emplace_back (inout_entry.path.vecEast_ft[i] / FEET_PER_NM);
      inout_entry.vecNorth_nm.emplace_back (inout_entry.path.vecNorth_ft[i] / FEET_PER_NM);
    }
  };

  auto *entry = get_or_fetch_flight_path (in_view_data, inout_state);
  if (entry == nullptr)
  {
    lmbda_draw_status (missionx::color::color_vec4_grey, "No flight path data.");
    return nullptr;
  }

  if (entry->fetch_state != missionx::mxFetchState_enum::fetch_ended)
  {
    // keep the last section on screen until the new one is ready
    if (const auto previous = inout_state.mapCache.find (inout_state.last_ready_cache_key); previous != inout_state.mapCache.end () && previous->second->fetch_state == missionx::mxFetchState_enum::fetch_ended && !previous->second->path.empty_cai ())
    {
      if (out_flag_reading != nullptr)
        *out_flag_reading = true;
      return previous->second.get ();
    }
    lmbda_draw_status (missionx::color::color_vec4_yellow, "Reading the flight path...");
    return nullptr;
  }

  if (entry->path.empty_cai ())
  {
    lmbda_draw_status (missionx::color::color_vec4_orange, (entry->fetch_msg.empty ()) ? std::string ("No flight path data.") : entry->fetch_msg);
    return nullptr;
  }

  lmbda_prepare_nm (*entry);
  inout_state.last_ready_cache_key = inout_state.get_cache_key_cai ();
  return entry;
}

// -------------------------------------------
// Row 1: waypoint combo and range buttons. in_label: "" or "A"/"B" in front of the combo.
void
ui_stats_screen::draw_flight_path_selector (missionx::mx_flight_path_view_state &inout_state, const std::string &in_label, const ImVec4 &in_label_color, const bool in_flag_compact, const bool in_flag_show_range)
{
  const auto &sections = inout_state.vecSections;

  if (!in_label.empty ())
  {
    ImGui::TextColored (in_label_color, "%s", in_label.c_str ());
    ImGui::SameLine ();
  }
  if (!in_flag_compact)
  {
    ImGui::TextColored (missionx::color::color_vec4_yellowgreen, "%s", "Waypoint:");
    ImGui::SameLine ();
  }
  ImGui::SetNextItemWidth ((in_flag_compact) ? 140.0f : 160.0f);
  if (ImGui::BeginCombo ("##path_waypoint", sections[inout_state.selected_section].label.c_str ()))
  {
    for (int section_index = 0; section_index < static_cast<int> (sections.size ()); ++section_index)
    {
      const bool flag_selected = (section_index == inout_state.selected_section);
      ImGui::PushStyleColor (ImGuiCol_Text, (sections[section_index].is_skipped_cai ()) ? missionx::color::color_vec4_grey : missionx::color::color_vec4_white);
      if (ImGui::Selectable (sections[section_index].label.c_str (), flag_selected))
        inout_state.selected_section = section_index;
      ImGui::PopStyleColor ();
      if (flag_selected)
        ImGui::SetItemDefaultFocus ();
    }
    ImGui::EndCombo ();
  }

  // ranges: radio buttons (easier to hit in VR), or a small combo when the space is narrow (compare columns)
  const auto section_type = sections[inout_state.selected_section].type;
  if (in_flag_show_range && in_flag_compact && section_type != missionx::mx_path_section::type_enum::full_flight)
  {
    const auto lmbda_range_text = [] (const missionx::mx_path_range_enum in_range) {
      switch (in_range)
      {
        case missionx::mx_path_range_enum::up_to_5nm:
          return "up to 5 nm";
        case missionx::mx_path_range_enum::up_to_20nm:
          return "up to 20 nm";
        default:
          return "whole leg";
      }
    };
    ImGui::SameLine (0.0f, 8.0f);
    ImGui::SetNextItemWidth (110.0f);
    if (ImGui::BeginCombo ("##path_range", lmbda_range_text (inout_state.range)))
    {
      std::vector<missionx::mx_path_range_enum> vecRanges = { missionx::mx_path_range_enum::up_to_5nm, missionx::mx_path_range_enum::up_to_20nm };
      if (section_type == missionx::mx_path_section::type_enum::leg)
        vecRanges.emplace_back (missionx::mx_path_range_enum::whole_leg);
      for (const auto range : vecRanges)
      {
        if (ImGui::Selectable (lmbda_range_text (range), inout_state.range == range))
          inout_state.range = range;
      }
      ImGui::EndCombo ();
    }
  }
  else if (in_flag_show_range && section_type != missionx::mx_path_section::type_enum::full_flight)
  {
    const auto lmbda_range_button = [&inout_state] (const char *in_range_label, const missionx::mx_path_range_enum in_range) {
      ImGui::SameLine (0.0f, 12.0f);
      if (ImGui::RadioButton (in_range_label, inout_state.range == in_range))
        inout_state.range = in_range;
    };
    lmbda_range_button ("up to 5 nm", missionx::mx_path_range_enum::up_to_5nm);
    lmbda_range_button ("up to 20 nm", missionx::mx_path_range_enum::up_to_20nm);
    if (section_type == missionx::mx_path_section::type_enum::leg)
      lmbda_range_button ("whole leg", missionx::mx_path_range_enum::whole_leg);
  }

  normalize_flight_path_range (inout_state);
}

// -------------------------------------------
// Row 2 (3D only): zoom, pan and preset views. Starts on the current line (the caller places it).
//   Pan moves the view by a quarter of its width, relative to the screen (left/right/away/towards), whatever the rotation.
void
ui_stats_screen::draw_camera_buttons (missionx::mx_plot_camera &inout_camera)
{
  constexpr double ZOOM_STEP = 1.5;
  if (ImGui::Button ("Zoom +"))
    inout_camera.zoom = std::min (inout_camera.zoom * ZOOM_STEP, 20.0);
  ImGui::SameLine ();
  if (ImGui::Button ("Zoom -"))
    inout_camera.zoom = std::max (inout_camera.zoom / ZOOM_STEP, 0.5);

  // pan: screen directions. The screen "right" is the world direction (-azimuth), "away" is 90 degrees from it
  const double pan_step_nm = inout_camera.last_view_span_nm * 0.25;
  const double right_rad   = -inout_camera.azimuth_deg * missionx::PI / 180.0;
  const auto   lmbda_pan   = [&inout_camera, pan_step_nm] (const double in_direction_rad) {
    inout_camera.pan_east_nm += pan_step_nm * std::cos (in_direction_rad);
    inout_camera.pan_north_nm += pan_step_nm * std::sin (in_direction_rad);
  };
  const auto lmbda_tooltip = [] (const char *in_tip) {
    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("%s", in_tip);
  };
  ImGui::SameLine (0.0f, 15.0f);
  if (ImGui::Button ("<##pan_left"))
    lmbda_pan (right_rad + missionx::PI);
  lmbda_tooltip ("Pan left");
  ImGui::SameLine ();
  if (ImGui::Button ("^##pan_away"))
    lmbda_pan (right_rad + missionx::PI / 2.0);
  lmbda_tooltip ("Pan away");
  ImGui::SameLine ();
  if (ImGui::Button ("v##pan_towards"))
    lmbda_pan (right_rad - missionx::PI / 2.0);
  lmbda_tooltip ("Pan towards you");
  ImGui::SameLine ();
  if (ImGui::Button (">##pan_right"))
    lmbda_pan (right_rad);
  lmbda_tooltip ("Pan right");

  ImGui::SameLine (0.0f, 15.0f);
  if (ImGui::Button ("3D"))
  {
    inout_camera.elevation_deg     = missionx::mx_plot_camera::DEFAULT_ELEVATION_DEG;
    inout_camera.flag_auto_azimuth = true; // seen from the side of the route again
  }
  ImGui::SameLine ();
  if (ImGui::Button ("Top"))
  {
    inout_camera.elevation_deg     = 90.0;
    inout_camera.azimuth_deg       = -90.0;
    inout_camera.flag_auto_azimuth = false;
  }
  ImGui::SameLine ();
  if (ImGui::Button ("Side"))
  {
    inout_camera.elevation_deg     = 0.0;
    inout_camera.flag_auto_azimuth = true; // looking at the route from its side
  }
  ImGui::SameLine (0.0f, 15.0f);
  if (ImGui::Button ("Reset view"))
    inout_camera.reset ();
}

// -------------------------------------------
// The 2D/3D toggle, then the camera buttons when 3D. Starts on the current line.
void
ui_stats_screen::draw_view_buttons (bool &inout_flag_show_3d, missionx::mx_plot_camera &inout_camera)
{
  if (ImGui::Button ((inout_flag_show_3d) ? "Show 2D" : "Show 3D"))
    inout_flag_show_3d = !inout_flag_show_3d;

  if (inout_flag_show_3d)
  {
    ImGui::SameLine (0.0f, 15.0f);
    draw_camera_buttons (inout_camera);
  }
}

// -------------------------------------------
// The segments of a path: start and end (exclusive) index pairs. A new segment starts after a reposition.
static std::vector<std::pair<int, int>>
get_path_segments (const missionx::mx_flight_path_data &in_path)
{
  std::vector<std::pair<int, int>> vecSegments;
  const auto                       points_count = static_cast<int> (in_path.vecEast_ft.size ());
  for (size_t i = 0; i < in_path.vecSegmentStart.size (); ++i)
  {
    const int start = static_cast<int> (in_path.vecSegmentStart[i]);
    const int end   = (i + 1 < in_path.vecSegmentStart.size ()) ? static_cast<int> (in_path.vecSegmentStart[i + 1]) : points_count;
    if (end - start >= 2)
      vecSegments.emplace_back (start, end);
  }
  return vecSegments;
}

// -------------------------------------------
// One or two paths in one 3D plot. Each layer has its own offset (merged plots) and colour.
//   East/north: nm, same scale on both (the ground track keeps its shape). Altitude: ft, stretched so the glide path is visible.
//   Depth cues: the ground track on the floor of the box, and thin vertical lines ("curtain") from the path down to it.
//   The legend is drawn above the plot (ImPlot3D can only place it inside).
//   in_box_scale: > 1 makes the box bigger in the plot area (focus and merged views).
void
ui_stats_screen::draw_paths_3d (const std::vector<mx_path_layer> &in_layers, missionx::mx_plot_camera &inout_camera, const char *in_z_axis_label, const float in_plot_height, const float in_box_scale)
{
  // the box: every point of every layer
  double east_min = 1e12, east_max = -1e12, north_min = 1e12, north_max = -1e12, alt_min = 1e12, alt_max = -1e12;
  for (const auto &layer : in_layers)
  {
    const auto &path = layer.entry->path;
    for (size_t i = 0; i < path.vecAlt_ft.size (); ++i)
    {
      east_min  = std::min (east_min, layer.entry->vecEast_nm[i] + layer.east_offset_nm);
      east_max  = std::max (east_max, layer.entry->vecEast_nm[i] + layer.east_offset_nm);
      north_min = std::min (north_min, layer.entry->vecNorth_nm[i] + layer.north_offset_nm);
      north_max = std::max (north_max, layer.entry->vecNorth_nm[i] + layer.north_offset_nm);
      alt_min   = std::min (alt_min, path.vecAlt_ft[i] + layer.alt_offset_ft);
      alt_max   = std::max (alt_max, path.vecAlt_ft[i] + layer.alt_offset_ft);
    }
  }
  if (east_min > east_max)
    return; // no points

  // ---- auto angle: look at the route from its side, turned ~35 degrees, so it is not pointing at the camera ----
  //      ImPlot3D turns the world by the azimuth first: a route heading at angle "dir" (from east, counter clockwise) is seen
  //      across the screen when dir + azimuth = 0 or 180. Two routes: the mean of their axes (a line has no direction: doubled angles).
  if (inout_camera.flag_auto_azimuth)
  {
    double sum_sin = 0.0, sum_cos = 0.0;
    for (const auto &layer : in_layers)
    {
      const auto  &path       = layer.entry->path;
      const size_t last_index = (path.anchor_index > 0) ? static_cast<size_t> (path.anchor_index) : path.vecAlt_ft.size () - 1;
      const double d_east     = layer.entry->vecEast_nm[last_index] - layer.entry->vecEast_nm[0];
      const double d_north    = layer.entry->vecNorth_nm[last_index] - layer.entry->vecNorth_nm[0];
      if (std::hypot (d_east, d_north) < 0.01)
        continue;
      const double dir_rad = std::atan2 (d_north, d_east);
      sum_sin += std::sin (2.0 * dir_rad);
      sum_cos += std::cos (2.0 * dir_rad);
    }
    if (sum_sin != 0.0 || sum_cos != 0.0)
    {
      const double route_dir_deg = 0.5 * std::atan2 (sum_sin, sum_cos) * 180.0 / missionx::PI;
      constexpr double SIDE_TURN_DEG = 35.0;

      // four good angles (both sides, turned left or right), take the one closest to the default view
      double best_azimuth = missionx::mx_plot_camera::DEFAULT_AZIMUTH_DEG;
      double best_diff    = 1e9;
      for (const double candidate : { -route_dir_deg + SIDE_TURN_DEG, -route_dir_deg - SIDE_TURN_DEG, -route_dir_deg + 180.0 + SIDE_TURN_DEG, -route_dir_deg + 180.0 - SIDE_TURN_DEG })
      {
        const double normalized = std::remainder (candidate, 360.0); // -180..180
        const double diff       = std::abs (std::remainder (normalized - missionx::mx_plot_camera::DEFAULT_AZIMUTH_DEG, 360.0));
        if (diff < best_diff)
        {
          best_diff    = diff;
          best_azimuth = normalized;
        }
      }
      inout_camera.azimuth_deg = best_azimuth;
    }
  }

  // ---- the view: zoom goes towards the anchor (touchdown/takeoff) of the first layer that has one, then the pan ----
  const double box_east_center  = (east_min + east_max) * 0.5;
  const double box_north_center = (north_min + north_max) * 0.5;
  double       focus_east       = box_east_center;
  double       focus_north      = box_north_center;
  for (const auto &layer : in_layers)
  {
    if (layer.entry->path.anchor_index >= 0)
    {
      focus_east  = layer.entry->vecEast_nm[layer.entry->path.anchor_index] + layer.east_offset_nm;
      focus_north = layer.entry->vecNorth_nm[layer.entry->path.anchor_index] + layer.north_offset_nm;
      break;
    }
  }
  const double zoom_pull    = 1.0 - 1.0 / std::max (inout_camera.zoom, 1.0); // 0 at zoom 1: the whole box. Closer: towards the anchor
  const double east_center  = box_east_center + (focus_east - box_east_center) * zoom_pull + inout_camera.pan_east_nm;
  const double north_center = box_north_center + (focus_north - box_north_center) * zoom_pull + inout_camera.pan_north_nm;
  // the east range is BOX_RATIO_X / BOX_RATIO_Y times the north range: one nm is the same length on both box sides
  const double x_to_y_ratio    = BOX_RATIO_X / BOX_RATIO_Y;
  const double half_span_north = std::max ({ (east_max - east_min) / x_to_y_ratio, north_max - north_min, 0.1 }) * 0.55 / inout_camera.zoom;
  const double half_span_east  = half_span_north * x_to_y_ratio;
  const double alt_padding  = std::max ((alt_max - alt_min) * 0.05, 50.0);
  const double floor_ft     = alt_min - alt_padding;
  inout_camera.last_view_span_nm = 2.0 * std::min (half_span_east, half_span_north); // pan step

  // ---- legend, above the plot ----
  {
    const auto lmbda_swatch = [] (const ImVec4 &in_color, const char *in_label, const bool in_flag_round) {
      const float  size   = ImGui::GetTextLineHeight () * 0.6f;
      const ImVec2 corner = ImGui::GetCursorScreenPos ();
      const ImVec2 center (corner.x + size * 0.5f, corner.y + ImGui::GetTextLineHeight () * 0.5f);
      if (in_flag_round)
        ImGui::GetWindowDrawList ()->AddCircleFilled (center, size * 0.5f, ImGui::ColorConvertFloat4ToU32 (in_color));
      else
        ImGui::GetWindowDrawList ()->AddRectFilled (ImVec2 (center.x - size * 0.5f, center.y - size * 0.5f), ImVec2 (center.x + size * 0.5f, center.y + size * 0.5f), ImGui::ColorConvertFloat4ToU32 (in_color));
      ImGui::Dummy (ImVec2 (size, ImGui::GetTextLineHeight ()));
      ImGui::SameLine (0.0f, 4.0f);
      ImGui::TextUnformatted (in_label);
      ImGui::SameLine (0.0f, 14.0f);
    };
    for (const auto &layer : in_layers)
      lmbda_swatch (layer.color, layer.label.c_str (), false);
    lmbda_swatch (missionx::color::color_vec4_green, "start", true);
    lmbda_swatch (missionx::color::color_vec4_magenta, "touchdown/takeoff", true);
    ImGui::TextColored (missionx::color::color_vec4_grey, "%s", "(?)");
    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("%s", "Drag to rotate. Zoom goes towards the touchdown/takeoff, the arrows pan.\nThe altitude scale is stretched. The thin vertical lines connect the route to its ground track.");
  }

  const ImVec2 plot_top_left = ImGui::GetCursorScreenPos ();
  const float  plot_width    = ImGui::GetContentRegionAvail ().x;
  const float  plot_height   = (in_plot_height > 0.0f) ? in_plot_height : std::max (150.0f, ImGui::GetContentRegionAvail ().y - 4.0f); // <= 0: fill the rest (focus views)

  if (ImPlot3D::BeginPlot ("##paths3d", ImVec2 (plot_width, plot_height), ImPlot3DFlags_NoTitle | ImPlot3DFlags_NoInputs | ImPlot3DFlags_NoMenus | ImPlot3DFlags_NoLegend))
  {
    // the axis label stays where ImPlot3D puts it, so the direction is in each tick: "0.8 W", "0", "1.6 E" (and N/S). Rotating keeps them right.
    ImPlot3D::SetupAxes ("West - East (nm)", "South - North (nm)", in_z_axis_label);
    ImPlot3D::SetupAxisFormat (ImAxis3D_X, [] (const double in_value, char *out_buffer, const int in_size, void *) {
      if (std::abs (in_value) < 1e-9)
        return snprintf (out_buffer, static_cast<size_t> (in_size), "0");
      return snprintf (out_buffer, static_cast<size_t> (in_size), "%g %s", std::abs (in_value), (in_value > 0.0) ? "E" : "W");
    });
    ImPlot3D::SetupAxisFormat (ImAxis3D_Y, [] (const double in_value, char *out_buffer, const int in_size, void *) {
      if (std::abs (in_value) < 1e-9)
        return snprintf (out_buffer, static_cast<size_t> (in_size), "0");
      return snprintf (out_buffer, static_cast<size_t> (in_size), "%g %s", std::abs (in_value), (in_value > 0.0) ? "N" : "S");
    });
    ImPlot3D::SetupAxisFormat (ImAxis3D_Z, [] (const double in_value, char *out_buffer, const int in_size, void *) { return snprintf (out_buffer, static_cast<size_t> (in_size), "%.0f", in_value); });
    ImPlot3D::SetupAxesLimits (east_center - half_span_east, east_center + half_span_east, north_center - half_span_north, north_center + half_span_north, floor_ft, alt_max + alt_padding, ImPlot3DCond_Always);
    ImPlot3D::SetupBoxScale (BOX_RATIO_X * in_box_scale, BOX_RATIO_Y * in_box_scale, BOX_RATIO_Z * in_box_scale);
    ImPlot3D::SetupBoxRotation (inout_camera.elevation_deg, inout_camera.azimuth_deg, false, ImPlot3DCond_Always);

    for (const auto &layer : in_layers)
    {
      const auto &path = layer.entry->path;

      // this layer, moved by its offset (local copies, at most STATS_PATH_MAX_POINTS)
      std::vector<double> vecEast, vecNorth, vecAlt;
      vecEast.reserve (path.vecAlt_ft.size ());
      vecNorth.reserve (path.vecAlt_ft.size ());
      vecAlt.reserve (path.vecAlt_ft.size ());
      for (size_t i = 0; i < path.vecAlt_ft.size (); ++i)
      {
        vecEast.emplace_back (layer.entry->vecEast_nm[i] + layer.east_offset_nm);
        vecNorth.emplace_back (layer.entry->vecNorth_nm[i] + layer.north_offset_nm);
        vecAlt.emplace_back (path.vecAlt_ft[i] + layer.alt_offset_ft);
      }
      const std::vector<double> vecFloor (vecAlt.size (), floor_ft);

      // the curtain: about 40 vertical lines from the route down to the floor (one item, drawn as segments: point, floor point, point, floor point...)
      {
        const size_t        curtain_step = std::max<size_t> (1, vecAlt.size () / 40);
        std::vector<double> vecCurtainEast, vecCurtainNorth, vecCurtainAlt;
        for (size_t i = 0; i < vecAlt.size (); i += curtain_step)
        {
          vecCurtainEast.insert (vecCurtainEast.end (), { vecEast[i], vecEast[i] });
          vecCurtainNorth.insert (vecCurtainNorth.end (), { vecNorth[i], vecNorth[i] });
          vecCurtainAlt.insert (vecCurtainAlt.end (), { vecAlt[i], floor_ft });
        }
        ImPlot3DSpec curtain_spec;
        curtain_spec.LineColor  = ImVec4 (layer.color.x, layer.color.y, layer.color.z, 0.22f);
        curtain_spec.LineWeight = 1.0f;
        curtain_spec.Flags      = static_cast<ImPlot3DItemFlags> (ImPlot3DItemFlags_NoLegend) | static_cast<ImPlot3DItemFlags> (ImPlot3DLineFlags_Segments); // int flags: no enum | enum warning
        ImPlot3D::PlotLine ((layer.label + "##curtain").c_str (), vecCurtainEast.data (), vecCurtainNorth.data (), vecCurtainAlt.data (), static_cast<int> (vecCurtainAlt.size ()), curtain_spec);
      }

      ImPlot3DSpec shadow_spec;
      shadow_spec.LineColor  = ImVec4 (layer.color.x, layer.color.y, layer.color.z, 0.30f);
      shadow_spec.LineWeight = 1.0f;
      shadow_spec.Flags      = ImPlot3DItemFlags_NoLegend;

      ImPlot3DSpec path_spec;
      path_spec.LineColor  = layer.color;
      path_spec.LineWeight = 3.5f;
      path_spec.Flags      = ImPlot3DItemFlags_NoLegend;

      for (const auto &[start, end] : get_path_segments (path))
      {
        ImPlot3D::PlotLine ((layer.label + "##ground").c_str (), &vecEast[start], &vecNorth[start], &vecFloor[start], end - start, shadow_spec);
        ImPlot3D::PlotLine (layer.label.c_str (), &vecEast[start], &vecNorth[start], &vecAlt[start], end - start, path_spec);
      }

      // markers: start of the section (green) and the anchor (magenta)
      const auto lmbda_marker = [&] (const std::string &in_label, const int in_index, const ImVec4 &in_color) {
        ImPlot3DSpec marker_spec;
        marker_spec.Marker          = ImPlot3DMarker_Circle;
        marker_spec.MarkerSize      = 5.0f;
        marker_spec.MarkerFillColor = in_color;
        marker_spec.MarkerLineColor = layer.color;
        marker_spec.Flags           = ImPlot3DItemFlags_NoLegend;
        ImPlot3D::PlotScatter (in_label.c_str (), &vecEast[in_index], &vecNorth[in_index], &vecAlt[in_index], 1, marker_spec);
      };
      lmbda_marker (layer.label + " start", 0, missionx::color::color_vec4_green);
      if (path.anchor_index >= 0)
        lmbda_marker (layer.label + " anchor", path.anchor_index, missionx::color::color_vec4_magenta);
    }

    ImPlot3D::EndPlot ();
  }

  // ---- left drag rotates. An invisible button over the plot takes the mouse, so the drag does not move the window ----
  //      The frame the click starts is ignored: in VR the pointer jumps to where the controller points, that jump is not a drag.
  //      Each frame is capped. VR: only around the vertical axis (left/right), slower.
  const ImVec2 cursor_after_plot = ImGui::GetCursorScreenPos ();
  ImGui::SetCursorScreenPos (plot_top_left);
  ImGui::InvisibleButton ("##rotate_3d", ImVec2 (plot_width, plot_height));
  if (ImGui::IsItemActive () && !ImGui::IsItemActivated () && ImGui::IsMouseDragging (ImGuiMouseButton_Left, 0.0f))
  {
    constexpr float MAX_PIXELS_PER_FRAME = 25.0f;
    // VR: ImgWindow knows if the window being built is in VR (no need for mxvr)
    const ImgWindow *building_window = ImgWindow::GetCurrentBuildingWindow ();
    const bool       flag_vr         = (building_window != nullptr) && building_window->IsInVR ();
    const double     degrees_per_pixel = (flag_vr) ? 0.15 : 0.4; // VR: slower, the controller is less steady than a mouse
    const ImVec2     mouse_delta       = ImGui::GetIO ().MouseDelta;
    const double     delta_x           = std::clamp (mouse_delta.x, -MAX_PIXELS_PER_FRAME, MAX_PIXELS_PER_FRAME);
    const double     delta_y           = std::clamp (mouse_delta.y, -MAX_PIXELS_PER_FRAME, MAX_PIXELS_PER_FRAME);

    inout_camera.flag_auto_azimuth = false; // the user turns it now
    inout_camera.azimuth_deg       = std::fmod (inout_camera.azimuth_deg + delta_x * degrees_per_pixel, 360.0); // drag left = clockwise
    inout_camera.elevation_deg     = std::clamp (inout_camera.elevation_deg + delta_y * degrees_per_pixel, -10.0, 90.0); // v26.10.2 VR rotates up/down too

    // v26.10.2 old: VR only turned the plot left/right (around the vertical axis), the tilt came from the 3D/Top/Side buttons
    // if (!flag_vr)
    //   inout_camera.elevation_deg = std::clamp (inout_camera.elevation_deg + delta_y * degrees_per_pixel, -10.0, 90.0);
  }
  ImGui::SetCursorScreenPos (cursor_after_plot);
  ImGui::Dummy (ImVec2 (0.0f, 0.0f)); // ImGui 1.92: a SetCursorScreenPos() must be followed by an item, else EndChild() asserts (ErrorCheckUsingSetCursorPosToExtendParentBoundaries)
}

// -------------------------------------------
// One or two paths in one 2D plot: altitude and airspeed.
//   X = distance from the touchdown/takeoff (nm), or the flight time for the full flight. Input is off (the wheel scrolls the page).
void
ui_stats_screen::draw_paths_2d (const std::vector<mx_path_layer> &in_layers, const bool in_flag_x_is_minutes, const char *in_x_axis_label, const char *in_y_axis_label, const float in_plot_height)
{
  double x_min = 1e12, x_max = -1e12, alt_min = 1e12, alt_max = -1e12, max_airspeed = 0.0;
  for (const auto &layer : in_layers)
  {
    const auto &path = layer.entry->path;
    const auto &vecX = (in_flag_x_is_minutes) ? path.vecMinutes : path.vecDist_nm;
    for (size_t i = 0; i < vecX.size (); ++i)
    {
      x_min        = std::min (x_min, vecX[i]);
      x_max        = std::max (x_max, vecX[i]);
      alt_min      = std::min (alt_min, path.vecAlt_ft[i] + layer.alt_offset_ft);
      alt_max      = std::max (alt_max, path.vecAlt_ft[i] + layer.alt_offset_ft);
      max_airspeed = std::max (max_airspeed, path.vecAirspeed_kt[i]);
    }
  }
  if (x_min > x_max)
    return;

  const float plot_height = (in_plot_height > 0.0f) ? in_plot_height : std::max (150.0f, ImGui::GetContentRegionAvail ().y - 4.0f); // <= 0: fill the rest (focus views)
  if (ImPlot::BeginPlot ("##paths2d", ImVec2 (-1.0f, plot_height), ImPlotFlags_NoInputs | ImPlotFlags_NoMenus))
  {
    ImPlot::SetupAxis (ImAxis_X1, in_x_axis_label);
    ImPlot::SetupAxisLimits (ImAxis_X1, x_min, std::max (x_max, x_min + 0.01), ImPlotCond_Always);
    ImPlot::SetupAxis (ImAxis_Y1, in_y_axis_label);
    ImPlot::SetupAxisLimits (ImAxis_Y1, alt_min - 100.0, alt_max + 200.0, ImPlotCond_Always);
    ImPlot::SetupAxis (ImAxis_Y2, "Airspeed kt", ImPlotAxisFlags_AuxDefault);
    ImPlot::SetupAxisLimits (ImAxis_Y2, 0.0, max_airspeed + 20.0, ImPlotCond_Always);

    for (const auto &layer : in_layers)
    {
      const auto &path = layer.entry->path;
      const auto &vecX = (in_flag_x_is_minutes) ? path.vecMinutes : path.vecDist_nm;

      std::vector<double> vecAlt;
      vecAlt.reserve (path.vecAlt_ft.size ());
      for (const double alt_ft : path.vecAlt_ft)
        vecAlt.emplace_back (alt_ft + layer.alt_offset_ft);

      ImPlotSpec airspeed_spec;
      airspeed_spec.LineColor  = ImVec4 (layer.color.x, layer.color.y, layer.color.z, 0.40f);
      airspeed_spec.LineWeight = 1.2f;

      ImPlotSpec altitude_spec;
      altitude_spec.LineColor  = layer.color;
      altitude_spec.LineWeight = 2.5f;

      for (const auto &[start, end] : get_path_segments (path))
      {
        ImPlot::SetAxes (ImAxis_X1, ImAxis_Y2);
        ImPlot::PlotLine ((layer.label + " airspeed").c_str (), &vecX[start], &path.vecAirspeed_kt[start], end - start, airspeed_spec);
        ImPlot::SetAxes (ImAxis_X1, ImAxis_Y1);
        ImPlot::PlotLine ((layer.label + " altitude").c_str (), &vecX[start], &vecAlt[start], end - start, altitude_spec);
      }

      if (path.anchor_index >= 0 && !in_flag_x_is_minutes)
      {
        ImPlotSpec anchor_spec;
        anchor_spec.LineColor = missionx::color::color_vec4_magenta;
        anchor_spec.Flags     = ImPlotItemFlags_NoLegend;
        const double anchor_x = vecX[path.anchor_index];
        ImPlot::PlotInfLines ((layer.label + "##anchor").c_str (), &anchor_x, 1, anchor_spec);
      }
    }
    ImPlot::EndPlot ();
  }
}

// -------------------------------------------
// The X axis of the 2D plot for one section.
static const char *
get_path_x_axis_label (const missionx::mx_path_section &in_section)
{
  switch (in_section.type)
  {
    case missionx::mx_path_section::type_enum::full_flight:
      return "Flight time (min)";
    case missionx::mx_path_section::type_enum::departure:
      return "nm from takeoff";
    default:
      return "nm from touchdown";
  }
}

// -------------------------------------------
// One flight in one plot (2D or 3D).
void
ui_stats_screen::draw_path_plot (missionx::mx_flight_path_cache_entry &in_entry, const missionx::mx_path_section &in_section, const ImVec4 &in_line_color, const std::string &in_layer_label, const bool in_flag_show_3d,
                                 missionx::mx_plot_camera &inout_camera, const float in_plot_height, const float in_box_scale)
{
  const std::vector<mx_path_layer> vecLayers = { mx_path_layer{ &in_entry, in_line_color, in_layer_label, 0.0, 0.0, 0.0 } };
  if (in_flag_show_3d)
    draw_paths_3d (vecLayers, inout_camera, "Altitude (ft)", in_plot_height, in_box_scale);
  else
    draw_paths_2d (vecLayers, in_section.type == missionx::mx_path_section::type_enum::full_flight, get_path_x_axis_label (in_section), "Altitude ft", in_plot_height);
}

// -------------------------------------------
// One flight: selector (row 1), view buttons and the focus button (row 2), then the plot.
// Returns true when the "focus" button was pressed (the caller switches the view).
bool
ui_stats_screen::draw_flight_path (const missionx::mx_stats_view_data &in_view_data, missionx::mx_flight_path_view_state &inout_state, const std::string &in_id_suffix, const ImVec4 &in_line_color, const float in_plot_height,
                                   const char *in_focus_button_label, const float in_box_scale)
{
  bool flag_focus_pressed = false;
  ImGui::PushID (("flight_path_" + in_id_suffix).c_str ());

  sync_flight_path_sections (in_view_data, inout_state);
  if (inout_state.vecSections.empty ())
  {
    ImGui::TextColored (missionx::color::color_vec4_grey, "%s", "No flight path data.");
    ImGui::PopID ();
    return false;
  }

  normalize_flight_path_range (inout_state);
  draw_flight_path_selector (inout_state, "", in_line_color);

  draw_view_buttons (inout_state.flag_show_3d, inout_state.camera);
  if (in_focus_button_label != nullptr)
  {
    ImGui::SameLine (0.0f, 25.0f);
    flag_focus_pressed = ImGui::Button (in_focus_button_label);
  }

  bool flag_reading = false;
  if (auto *entry = get_ready_flight_path_or_draw_status (in_view_data, inout_state, in_plot_height + ImGui::GetTextLineHeightWithSpacing (), &flag_reading); entry != nullptr)
  {
    const auto &section = inout_state.vecSections[inout_state.selected_section];
    ImGui::TextColored ((flag_reading) ? missionx::color::color_vec4_yellow : missionx::color::color_vec4_beige, "%s",
                        (flag_reading) ? "Reading the flight path..." : fmt::format ("{}, {}", section.label, entry->path.range_label).c_str ());

    draw_path_plot (*entry, section, in_line_color, "Flight path", inout_state.flag_show_3d, inout_state.camera, in_plot_height, in_box_scale);
  }

  ImGui::PopID ();
  return flag_focus_pressed;
}

// -------------------------------------------
// The end screen and the one-flight screen: the small plot under the stats, or (focus view) the plot alone, filling the screen.
// Returns true when it drew the focus view (the caller skips the other stats).
bool
ui_stats_screen::draw_flight_path_section (const missionx::mx_stats_view_data &in_view_data, missionx::mx_flight_path_view_state &inout_state, const std::string &in_id_suffix, const ImVec4 &in_line_color, const float in_small_plot_height)
{
  if (inout_state.flag_focus_view)
  {
    mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // the font with the icons
    if (ImGui::Button (mxUtils::from_u8string (ICON_FA_REPLY " Back to the stats").c_str ()))
      inout_state.flag_focus_view = false;
    mxUiReleaseLastFont ();

    // the plot fills what is left of the screen (height 0 = measured right before the plot)
    draw_flight_path (in_view_data, inout_state, in_id_suffix, in_line_color, 0.0f, nullptr, FOCUS_BOX_SCALE);
    return true;
  }

  ImGui::Separator ();
  if (draw_flight_path (in_view_data, inout_state, in_id_suffix, in_line_color, in_small_plot_height, "Focus plot", SMALL_BOX_SCALE))
    inout_state.flag_focus_view = true;
  return false;
}

// -------------------------------------------
// Compare screen: the flight paths of A and B. Split: two plots side by side. Merged: one plot with both routes.
//   Both plots use one camera and one 2D/3D choice, so A and B are always seen from the same side.
//   Merged: the range is shared, and B follows A's waypoint unless "Same waypoint" is off.
//   Anchors closer than STATS_PATH_SAME_PLACE_NM: real positions (MSL). Else touchdown on touchdown, altitude = height above each anchor.
void
ui_stats_screen::draw_compare_flight_paths (missionx::mx_stats_flight_slot &inout_slot_a, missionx::mx_stats_flight_slot &inout_slot_b, const bool in_flag_focus_view)
{
  auto &layer = missionx::strct_stats_layer;

  sync_flight_path_sections (inout_slot_a.view_data, inout_slot_a.path_view);
  const bool flag_b_is_new = inout_slot_b.path_view.flight_key.empty ();
  sync_flight_path_sections (inout_slot_b.view_data, inout_slot_b.path_view);
  if (flag_b_is_new && !inout_slot_b.path_view.vecSections.empty ())
  {
    inout_slot_b.path_view.selected_section = std::min (inout_slot_a.path_view.selected_section, static_cast<int> (inout_slot_b.path_view.vecSections.size ()) - 1);
    inout_slot_b.path_view.range            = inout_slot_a.path_view.range;
  }
  if (inout_slot_a.path_view.vecSections.empty () || inout_slot_b.path_view.vecSections.empty ())
  {
    ImGui::TextColored (missionx::color::color_vec4_grey, "%s", "No flight path data.");
    return;
  }

  bool &flag_merged = (in_flag_focus_view) ? layer.flag_compare_paths_merged_focus : layer.flag_compare_paths_merged_inline;

  // row 1: back (focus), merge/split, focus, then the shared view buttons
  if (in_flag_focus_view)
  {
    mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // the font with the icons
    if (ImGui::Button (mxUtils::from_u8string (ICON_FA_REPLY " Back to compare").c_str ()))
      layer.flag_compare_paths_focus = false;
    mxUiReleaseLastFont ();
    ImGui::SameLine (0.0f, 20.0f);
  }
  if (ImGui::Button ((flag_merged) ? "Split plots" : "Merge plots"))
    flag_merged = !flag_merged;
  if (!in_flag_focus_view)
  {
    ImGui::SameLine (0.0f, 10.0f);
    if (ImGui::Button ("Focus plots"))
      layer.flag_compare_paths_focus = true;
  }
  ImGui::SameLine (0.0f, 20.0f);
  draw_view_buttons (layer.flag_compare_show_3d, layer.merged_camera);

  const float plot_height = (in_flag_focus_view) ? 0.0f : 250.0f; // 0: the plot fills what is left of the screen (measured right before the plot)

  if (!flag_merged)
  {
    if (ImGui::BeginTable ("##compare_paths", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame))
    {
      ImGui::TableNextRow ();
      const std::array<missionx::mx_stats_flight_slot *, 2> slots  = { &inout_slot_a, &inout_slot_b };
      const std::array<std::string, 2>                       labels = { "A", "B" };
      const std::array<ImVec4, 2>                            colors = { COLOR_FLIGHT_A, COLOR_FLIGHT_B };
      for (size_t flight_index = 0; flight_index < slots.size (); ++flight_index)
      {
        ImGui::TableSetColumnIndex (static_cast<int> (flight_index));
        ImGui::PushID (labels[flight_index].c_str ());
        auto &path_view = slots[flight_index]->path_view;
        draw_flight_path_selector (path_view, labels[flight_index], colors[flight_index], true, true);
        bool flag_reading = false;
        if (auto *entry = get_ready_flight_path_or_draw_status (slots[flight_index]->view_data, path_view, plot_height + ImGui::GetTextLineHeightWithSpacing (), &flag_reading); entry != nullptr)
        {
          const auto &section = path_view.vecSections[path_view.selected_section];
          ImGui::TextColored ((flag_reading) ? missionx::color::color_vec4_yellow : missionx::color::color_vec4_beige, "%s",
                              (flag_reading) ? "Reading the flight path..." : fmt::format ("{}, {}", section.label, entry->path.range_label).c_str ());
          draw_path_plot (*entry, section, colors[flight_index], labels[flight_index], layer.flag_compare_show_3d, layer.merged_camera, plot_height, (in_flag_focus_view) ? FOCUS_BOX_SCALE : 1.0f);
        }
        ImGui::PopID ();
      }
      ImGui::EndTable ();
    }
    return;
  }

  // ---- merged: A picks the waypoint and the range, B follows (same range always, same waypoint unless turned off) ----
  ImGui::PushID ("merged_paths");
  ImGui::PushID ("A");
  draw_flight_path_selector (inout_slot_a.path_view, "A", COLOR_FLIGHT_A, false, true);
  ImGui::PopID ();
  ImGui::SameLine (0.0f, 20.0f);
  ImGui::Checkbox ("sync waypoints B=A", &layer.flag_compare_same_waypoint);

  auto &path_view_b = inout_slot_b.path_view;
  if (layer.flag_compare_same_waypoint)
    path_view_b.selected_section = std::min (inout_slot_a.path_view.selected_section, static_cast<int> (path_view_b.vecSections.size ()) - 1);
  else
  {
    ImGui::PushID ("B");
    draw_flight_path_selector (path_view_b, "B", COLOR_FLIGHT_B, false, false);
    ImGui::PopID ();
  }
  path_view_b.range = inout_slot_a.path_view.range;
  normalize_flight_path_range (path_view_b);

  // both must have something to draw. The reserved space keeps the page height while a section is read (no scroll jump)
  bool        flag_reading_a  = false;
  bool        flag_reading_b  = false;
  const float reserved_height = plot_height + ImGui::GetTextLineHeightWithSpacing ();
  auto       *entry_a         = get_ready_flight_path_or_draw_status (inout_slot_a.view_data, inout_slot_a.path_view, 0.0f, &flag_reading_a);
  auto       *entry_b         = get_ready_flight_path_or_draw_status (inout_slot_b.view_data, path_view_b, 0.0f, &flag_reading_b);
  if (entry_a == nullptr || entry_b == nullptr)
  {
    ImGui::Dummy (ImVec2 (0.0f, reserved_height));
    ImGui::PopID ();
    return;
  }
  if (flag_reading_a || flag_reading_b)
  {
    ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", "Reading the flight path...");
    ImGui::SameLine (0.0f, 20.0f);
  }

  // where B goes in A's plot
  const auto  &path_a        = entry_a->path;
  const auto  &path_b        = entry_b->path;
  const double anchors_nm    = mxUtils::mxCalcDistanceBetween2Points (path_a.origin_lat, path_a.origin_lon, path_b.origin_lat, path_b.origin_lon);
  const bool   is_same_place = anchors_nm <= mxconst::STATS_PATH_SAME_PLACE_NM;
  mx_path_layer layer_a{ entry_a, COLOR_FLIGHT_A, "A", 0.0, 0.0, 0.0 };
  mx_path_layer layer_b{ entry_b, COLOR_FLIGHT_B, "B", 0.0, 0.0, 0.0 };
  if (is_same_place && anchors_nm > 0.0)
  {
    const double bearing_rad = mxUtils::mxCalcBearingBetween2Points (path_a.origin_lat, path_a.origin_lon, path_b.origin_lat, path_b.origin_lon) * missionx::PI / 180.0;
    layer_b.east_offset_nm   = anchors_nm * std::sin (bearing_rad);
    layer_b.north_offset_nm  = anchors_nm * std::cos (bearing_rad);
  }
  else if (!is_same_place)
  {
    layer_a.alt_offset_ft = -path_a.origin_alt_ft; // height above each touchdown/takeoff
    layer_b.alt_offset_ft = -path_b.origin_alt_ft;
  }

  const auto &section_a = inout_slot_a.path_view.vecSections[inout_slot_a.path_view.selected_section];
  const auto &section_b = path_view_b.vecSections[path_view_b.selected_section];
  ImGui::TextColored (COLOR_FLIGHT_A, "%s", fmt::format ("A: {}, {}", section_a.label, path_a.range_label).c_str ());
  ImGui::SameLine (0.0f, 20.0f);
  ImGui::TextColored (COLOR_FLIGHT_B, "%s", fmt::format ("B: {}, {}", section_b.label, path_b.range_label).c_str ());
  ImGui::SameLine (0.0f, 20.0f);
  const std::string placement_text = (is_same_place) ? fmt::format ("Real positions ({:.1f} nm apart)", anchors_nm) : fmt::format ("Touchdown on touchdown ({:.0f} nm apart), height above each anchor", anchors_nm);
  ImGui::TextColored (missionx::color::color_vec4_beige, "%s", placement_text.c_str ());

  const std::vector<mx_path_layer> vecLayers = { layer_a, layer_b };
  const char *z_label = (is_same_place) ? "Altitude (ft)" : "Height above anchor (ft)";
  if (layer.flag_compare_show_3d)
    draw_paths_3d (vecLayers, layer.merged_camera, z_label, plot_height, (in_flag_focus_view) ? FOCUS_BOX_SCALE : SMALL_BOX_SCALE);
  else
  {
    const bool flag_x_is_minutes = section_a.type == missionx::mx_path_section::type_enum::full_flight && section_b.type == missionx::mx_path_section::type_enum::full_flight;
    draw_paths_2d (vecLayers, flag_x_is_minutes, (flag_x_is_minutes) ? "Flight time (min)" : get_path_x_axis_label (section_a), z_label, plot_height);
  }

  ImGui::PopID ();
}

// -------------------------------------------
// v26.10.2 The stats layer is opened (home screen "Stats" button): start at the list and read the history folder again.
void
ui_stats_screen::on_layer_opened ()
{
  missionx::strct_stats_layer.screen              = mx_stats_screen_enum::history_list;
  missionx::strct_stats_layer.flag_refresh_needed = true;
}

// -------------------------------------------
// v26.10.2 The stats layer: the list of saved flights, one flight, or two flights compared.
void
ui_stats_screen::draw_stats_layer ()
{
  // ---- start the list scan when needed ----
  auto &layer = missionx::strct_stats_layer;
  if (layer.flag_refresh_needed && layer.list_fetch_state != missionx::mxFetchState_enum::fetch_in_process)
  {
    layer.flag_refresh_needed = false;
    layer.vecSelectedFiles.clear ();
    layer.list_fetch_state = missionx::mxFetchState_enum::fetch_in_process;
    missionx::data_manager::mFetchFutures.push_back (std::async (std::launch::async, missionx::data_manager::fetch_stats_history_list_cai, &layer.vecRows, &layer.list_fetch_state, &layer.list_fetch_msg));
  }

  ImGui::BeginChild ("stats_layer_main", ImVec2 (0.0f, -25.0f), ImGuiChildFlags_None); // v26.10.2 was -50: 25px left for the messages line

  switch (layer.screen)
  {
    case mx_stats_screen_enum::one_flight:
      draw_one_flight_screen ();
      break;
    case mx_stats_screen_enum::compare_flights:
      draw_compare_screen ();
      break;
    default:
      draw_history_list_screen ();
      break;
  }

  ImGui::EndChild ();
}

// -------------------------------------------
// v26.10.2 The list of saved flights, newest first. A click anywhere on a row selects it (up to 2 flights).
void
ui_stats_screen::draw_history_list_screen ()
{
  auto &layer = missionx::strct_stats_layer;

  // ---- title and explanation ----
  ImGui::Spacing ();
  mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_BIG ());
  ImGui::TextColored (missionx::color::color_vec4_white, "%s", "Your Flights"); // v26.10.2 inner title: white
  mxUiReleaseLastFont ();
  ImGui::Separator ();

  mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
  ImGui::TextWrapped ("%s", "Every flight that reaches the end screen (success, failure or abort) is saved here, newest first. "
                            "Pick one flight to see its stats, or two flights to compare them: scores, landings, legs and the flight graph.");
  mxUiReleaseLastFont ();
  ImGui::Spacing ();

  if (layer.list_fetch_state == missionx::mxFetchState_enum::fetch_in_process)
  {
    ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", "Reading the saved flights...");
    return;
  }

  if (!layer.list_fetch_msg.empty ())
    ImGui::TextColored (missionx::color::color_vec4_orangered, "%s", layer.list_fetch_msg.c_str ());

  // ---- the buttons above the list ----
  const int  selected_count       = static_cast<int> (layer.vecSelectedFiles.size ());
  const bool flag_disable_display = mxStartUiDisableState (selected_count != 1);
  mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  if (ImGui::Button ("Display", ImVec2 (140.0f, 34.0f)))
    open_one_flight (layer.vecSelectedFiles.front ());
  mxUiReleaseLastFont ();
  mxEndUiDisableState (flag_disable_display);

  ImGui::SameLine (0.0f, 10.0f);
  const bool flag_disable_compare = mxStartUiDisableState (selected_count != 2);
  mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  if (ImGui::Button ("Compare", ImVec2 (140.0f, 34.0f)))
    open_two_flights (layer.vecSelectedFiles.at (0), layer.vecSelectedFiles.at (1));
  mxUiReleaseLastFont ();
  mxEndUiDisableState (flag_disable_compare);

  ImGui::SameLine (0.0f, 10.0f);
  const bool flag_disable_clear = mxStartUiDisableState (selected_count == 0);
  if (ImGui::Button ("Clear selection", ImVec2 (0.0f, 34.0f)))
    layer.vecSelectedFiles.clear ();
  mxEndUiDisableState (flag_disable_clear);

  ImGui::SameLine (0.0f, 10.0f);
  if (ImGui::Button ("Refresh", ImVec2 (0.0f, 34.0f))) // the sync icon is not in the loaded icon font
    layer.flag_refresh_needed = true;

  ImGui::SameLine (0.0f, 20.0f);
  const int max_files = missionx::data_manager::stats_history_get_max_files_cai ();
  ImGui::TextColored (missionx::color::color_vec4_grey, "%s", fmt::format ("{} saved flight{} (keeps the last {}). Selected: {}/2", layer.vecRows.size (), (layer.vecRows.size () == 1) ? "" : "s", max_files, selected_count).c_str ());

  // ---- the list ----
  if (layer.vecRows.empty ())
  {
    ImGui::Spacing ();
    ImGui::TextColored (missionx::color::color_vec4_white, "%s", (max_files > 0) ? "No saved flights yet. Fly a mission to the end screen and it will be listed here."
                                                                                   : "Saving flights is off (setup option \"stats_history_max_files\" is 0).");
    return;
  }

  draw_history_table ();
}

// -------------------------------------------
// v26.10.2 The table of saved flights. Rows are large (VR): a click anywhere on the row selects or unselects it, a double click opens it.
void
ui_stats_screen::draw_history_table ()
{
  auto &layer = missionx::strct_stats_layer;

  // "1:05:09"
  const auto lmbda_format_time = [](const double in_seconds) -> std::string {
    const int total_sec = static_cast<int> (in_seconds);
    return fmt::format ("{}:{:02}:{:02}", total_sec / 3600, (total_sec % 3600) / 60, total_sec % 60);
  };

  // the selection label: "A" for the first flight picked, "B" for the second
  const auto lmbda_selection_label = [&](const std::string &in_file_path) -> std::string {
    for (size_t i = 0; i < layer.vecSelectedFiles.size (); ++i)
    {
      if (layer.vecSelectedFiles[i] == in_file_path)
        return (i == 0) ? "A" : "B";
    }
    return "";
  };

  constexpr ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings;
  constexpr float           ROW_HEIGHT  = 30.0f; // easy to point at in VR

  mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
  if (ImGui::BeginTable ("stats_history_table", 7, table_flags, ImVec2 (0.0f, 0.0f)))
  {
    ImGui::TableSetupScrollFreeze (0, 1);
    ImGui::TableSetupColumn ("", ImGuiTableColumnFlags_WidthFixed, 28.0f);
    ImGui::TableSetupColumn ("Date", ImGuiTableColumnFlags_WidthFixed, 130.0f);
    ImGui::TableSetupColumn ("Route", ImGuiTableColumnFlags_WidthStretch, 3.0f);
    ImGui::TableSetupColumn ("Plane", ImGuiTableColumnFlags_WidthStretch, 2.0f);
    ImGui::TableSetupColumn ("Result", ImGuiTableColumnFlags_WidthFixed, 70.0f);
    ImGui::TableSetupColumn ("Time flew", ImGuiTableColumnFlags_WidthFixed, 75.0f);
    ImGui::TableSetupColumn ("Landings", ImGuiTableColumnFlags_WidthFixed, 65.0f);
    ImGui::TableHeadersRow ();

    for (const auto &row : layer.vecRows)
    {
      ImGui::PushID (row.file_name.c_str ());
      ImGui::TableNextRow (ImGuiTableRowFlags_None, ROW_HEIGHT);

      // a file from a newer plugin, or a file that can not be read: listed, but it can not be picked
      const bool        flag_can_pick    = row.is_readable && row.schema_version <= mxconst::STATS_SCHEMA_VERSION;
      const std::string pick_label       = lmbda_selection_label (row.file_path);
      const bool        flag_is_selected = !pick_label.empty ();

      // column 0: the selectable spans the whole row
      ImGui::TableNextColumn ();
      const bool flag_disable_row = mxStartUiDisableState (!flag_can_pick);
      // a click selects or unselects the row. A double click opens the flight (Display)
      if (ImGui::Selectable (fmt::format ("{}##row", pick_label).c_str (), flag_is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick, ImVec2 (0.0f, ROW_HEIGHT)))
      {
        if (ImGui::IsMouseDoubleClicked (ImGuiMouseButton_Left))
        {
          // the first click of the double click toggled the row: make sure it stays selected, then open it
          const bool flag_not_selected = lmbda_selection_label (row.file_path).empty ();
          if (flag_not_selected && layer.vecSelectedFiles.size () < 2)
            layer.vecSelectedFiles.emplace_back (row.file_path);

          open_one_flight (row.file_path);
        }
        else if (flag_is_selected)
          std::erase (layer.vecSelectedFiles, row.file_path);
        else if (layer.vecSelectedFiles.size () < 2)
          layer.vecSelectedFiles.emplace_back (row.file_path);
      }

      ImGui::TableNextColumn ();
      ImGui::TextColored (missionx::color::color_vec4_white, "%s", row.saved_local_time_s.c_str ());

      ImGui::TableNextColumn ();
      if (!row.is_readable)
        ImGui::TextColored (missionx::color::color_vec4_grey, "%s", fmt::format ("(can not read: {})", row.error_s).c_str ());
      else if (row.schema_version > mxconst::STATS_SCHEMA_VERSION)
        ImGui::TextColored (missionx::color::color_vec4_grey, "%s", "(saved by a newer plugin version)");
      else
        ImGui::TextColored (missionx::color::color_vec4_beige, "%s", (row.route_s.empty ()) ? "-" : row.route_s.c_str ());

      ImGui::TableNextColumn ();
      const std::string plane_s = (row.planes_count > 1) ? fmt::format ("{} (+{})", row.plane_s, row.planes_count - 1) : row.plane_s;
      ImGui::TextColored (missionx::color::color_vec4_lightblue, "%s", plane_s.c_str ());

      ImGui::TableNextColumn ();
      const auto [result_s, result_color] = get_result_text_and_color (row.outcome, row.crashed, flag_is_selected);
      ImGui::TextColored (result_color, "%s", result_s.c_str ());

      ImGui::TableNextColumn ();
      ImGui::TextColored (missionx::color::color_vec4_white, "%s", lmbda_format_time (row.time_flew_sec).c_str ());

      ImGui::TableNextColumn ();
      ImGui::TextColored (missionx::color::color_vec4_white, "%d", row.landings_count);

      mxEndUiDisableState (flag_disable_row);
      ImGui::PopID ();
    }
    ImGui::EndTable ();
  }
  mxUiReleaseLastFont ();
}

// -------------------------------------------
// v26.10.2 The result text and its color. in_flag_on_selected_row: brighter colors, readable on the blue selection background.
std::pair<std::string, ImVec4>
ui_stats_screen::get_result_text_and_color (const std::string &in_outcome, const bool in_crashed, const bool in_flag_on_selected_row)
{
  if (in_crashed)
    return { "Crashed", (in_flag_on_selected_row) ? missionx::color::color_vec4_orangered : missionx::color::color_vec4_red };
  if (in_outcome == mxconst::get_FLIGHT_OUTCOME_SUCCESS ())
    return { "Success", (in_flag_on_selected_row) ? missionx::color::color_vec4_greenyellow : missionx::color::color_vec4_green };
  if (in_outcome == mxconst::get_FLIGHT_OUTCOME_FAILURE ())
    return { "Failed", (in_flag_on_selected_row) ? missionx::color::color_vec4_yellow : missionx::color::color_vec4_orangered };
  if (in_outcome == mxconst::get_FLIGHT_OUTCOME_ABORTED ())
    return { "Aborted", (in_flag_on_selected_row) ? missionx::color::color_vec4_yellow : missionx::color::color_vec4_orange };
  if (in_outcome.empty ())
    return { "-", missionx::color::color_vec4_grey };

  return { in_outcome, missionx::color::color_vec4_grey };
}

// -------------------------------------------
// v26.10.2 Read a saved flight into a flight slot (0 = A, 1 = B) in the background. Nothing to do when the slot already holds that file.
void
ui_stats_screen::load_flight_slot (const int in_slot_index, const std::string &in_file_path)
{
  auto &slot = missionx::strct_stats_layer.flight_slots.at (in_slot_index);
  if (slot.fetch_state == missionx::mxFetchState_enum::fetch_in_process)
    return; // this slot is being read

  // the same file is already loaded
  if (slot.file_path == in_file_path && slot.fetch_state == missionx::mxFetchState_enum::fetch_ended && !slot.view_data.stats.vecSeq_d.empty ())
    return;

  slot.file_path = in_file_path;
  slot.row       = missionx::mx_stats_history_row ();
  for (const auto &row : missionx::strct_stats_layer.vecRows)
  {
    if (row.file_path == in_file_path)
      slot.row = row;
  }

  slot.vecGraphElevX_pct.clear ();
  slot.vecGraphAirX_pct.clear ();
  slot.path_view.reset (); // v26.10.2 a new flight: new waypoint list
  slot.fetch_msg.clear ();
  slot.fetch_state = missionx::mxFetchState_enum::fetch_in_process;
  missionx::data_manager::mFetchFutures.push_back (std::async (std::launch::async, missionx::data_manager::fetch_mission_stats_cai, in_file_path, &slot.view_data, &slot.fetch_state, &slot.fetch_msg));
  Log::logMsg (fmt::format ("[{}] Slot {}: reading '{}'.", __func__, (in_slot_index == 0) ? "A" : "B", in_file_path));
}

// -------------------------------------------
// v26.10.2 Show one saved flight (slot A).
void
ui_stats_screen::open_one_flight (const std::string &in_file_path)
{
  missionx::strct_stats_layer.screen = mx_stats_screen_enum::one_flight;
  load_flight_slot (0, in_file_path);
}

// -------------------------------------------
// v26.10.2 Compare two saved flights: A and B, in the order they were picked.
void
ui_stats_screen::open_two_flights (const std::string &in_file_path_a, const std::string &in_file_path_b)
{
  missionx::strct_stats_layer.screen                   = mx_stats_screen_enum::compare_flights;
  missionx::strct_stats_layer.flag_compare_paths_focus = false; // v26.10.2 start with the whole compare screen
  missionx::strct_stats_layer.merged_camera.reset ();
  load_flight_slot (0, in_file_path_a);
  load_flight_slot (1, in_file_path_b);
}

// -------------------------------------------
// v26.10.2 The header line of a flight: "A  2026-11-10 15:39  Cessna 172 ...  title  Success". in_label: "" for the one-flight screen.
void
ui_stats_screen::draw_flight_header (const missionx::mx_stats_flight_slot &in_slot, const std::string &in_label, const ImVec4 &in_label_color)
{
  const auto &row = in_slot.row;

  if (!in_label.empty ())
  {
    mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
    ImGui::TextColored (in_label_color, "%s", in_label.c_str ());
    mxUiReleaseLastFont ();
    ImGui::SameLine (0.0f, 10.0f);
    ImGui::TextColored (missionx::color::color_vec4_beige, "%s", (row.route_s.empty ()) ? "-" : row.route_s.c_str ());
    ImGui::SameLine (0.0f, 15.0f);
  }

  ImGui::TextColored (missionx::color::color_vec4_white, "%s", row.saved_local_time_s.c_str ());
  ImGui::SameLine (0.0f, 15.0f);
  ImGui::TextColored (missionx::color::color_vec4_lightblue, "%s", row.plane_s.c_str ());
  if (!row.mission_title.empty ())
  {
    ImGui::SameLine (0.0f, 15.0f);
    ImGui::TextColored (missionx::color::color_vec4_grey, "%s", row.mission_title.c_str ());
  }
  ImGui::SameLine (0.0f, 15.0f);
  const auto [result_s, result_color] = get_result_text_and_color (row.outcome, row.crashed, false);
  ImGui::TextColored (result_color, "%s", result_s.c_str ());

  if (in_slot.fetch_state == missionx::mxFetchState_enum::fetch_ended && !in_slot.view_data.flight_info.outcome_reason.empty ())
    ImGui::TextColored (missionx::color::color_vec4_orange, "%s", fmt::format ("Reason: {}", in_slot.view_data.flight_info.outcome_reason).c_str ());
}

// -------------------------------------------
// v26.10.2 Is the slot ready to draw? When not, draws "Reading..." or the fetch message and returns false.
//          The fetch message holds success text too ("Fetched: N of statistics rows."), so the decision is by the data: no stats rows = show the message.
bool
ui_stats_screen::is_slot_ready_or_draw_status (const missionx::mx_stats_flight_slot &in_slot)
{
  if (in_slot.fetch_state == missionx::mxFetchState_enum::fetch_in_process)
  {
    ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", "Reading the flight...");
    return false;
  }
  if (in_slot.view_data.stats.vecSeq_d.empty ())
  {
    ImGui::TextColored (missionx::color::color_vec4_orangered, "%s", (in_slot.fetch_msg.empty ()) ? "No stats were found in this file." : in_slot.fetch_msg.c_str ());
    return false;
  }
  return true;
}

// -------------------------------------------
// v26.10.2 "Back to the list" button
void
ui_stats_screen::add_back_to_list_button ()
{
  mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  if (ImGui::Button (mxUtils::from_u8string (ICON_FA_REPLY " Back to the list").c_str (), ImVec2 (0.0f, 34.0f)))
    missionx::strct_stats_layer.screen = mx_stats_screen_enum::history_list;
  mxUiReleaseLastFont ();
}

// -------------------------------------------
// v26.10.2 One saved flight: a header (route, date, plane, result) and the same stats as the end screen.
void
ui_stats_screen::draw_one_flight_screen ()
{
  const auto &slot = missionx::strct_stats_layer.flight_slots.at (0);

  // ---- header (hidden in the flight path focus view: the plot gets the space) ----
  if (!slot.path_view.flag_focus_view)
  {
    add_back_to_list_button ();
    ImGui::SameLine (0.0f, 20.0f);
    mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_BIG ());
    ImGui::TextColored (missionx::color::color_vec4_white, "%s", (slot.row.route_s.empty ()) ? "Flight" : slot.row.route_s.c_str ()); // v26.10.2 inner title: white (the screen title is aqua)
    mxUiReleaseLastFont ();

    mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
    draw_flight_header (slot, "", missionx::color::color_vec4_white);
    mxUiReleaseLastFont ();
    ImGui::Separator ();
  }

  // ---- the stats ----
  if (!is_slot_ready_or_draw_status (slot))
    return;

  ImGui::BeginChild ("one_flight_stats", ImVec2 (0.0f, 0.0f), ImGuiChildFlags_None);
  mx_stats_draw_options draw_options;
  draw_options.id_suffix = "flight_a";
  draw_options.path_view = &missionx::strct_stats_layer.flight_slots.at (0).path_view; // v26.10.2
  draw_flight_stats (slot.view_data, draw_options);
  ImGui::EndChild ();
}

// -------------------------------------------
// v26.10.2 The values of one flight that the compare table shows. Calculated from its view data.
ui_stats_screen::mx_compare_values
ui_stats_screen::get_compare_values (const missionx::mx_stats_view_data &in_view_data)
{
  const auto        &stats = in_view_data.stats;
  mx_compare_values  values;

  values.pitch_score = stats.pitchScore;
  values.roll_score  = stats.rollScore;
  values.g_score     = stats.gForceScore;
  values.total_score = stats.pitchScore + stats.rollScore + stats.gForceScore;
  values.max_bank    = std::max (std::abs (stats.minRoll), std::abs (stats.maxRoll));
  values.max_g       = stats.maxGforce;

  // landings: the graded ones give the scores, every landing gives the touchdown rates
  for (const auto &landing : stats.mapLandingAnalysis | std::views::values)
  {
    ++values.landings_count;
    values.bounces_count += landing.bounce_count;

    if (landing.has_summary)
    {
      values.hardest_td_fpm = std::max (values.hardest_td_fpm, landing.td_fpm);
      values.softest_td_fpm = (values.softest_td_fpm < 0.0) ? landing.td_fpm : std::min (values.softest_td_fpm, landing.td_fpm);
    }

    if (landing.is_crash)
      continue; // not graded

    ++values.graded_landings_count;
    values.best_landing_score = std::max (values.best_landing_score, static_cast<double> (landing.landing_score));
    values.sum_landing_score += landing.landing_score;
  }

  // legs: distance, fuel used, finished legs
  for (const auto &leg : in_view_data.vecLegs)
  {
    ++values.legs_count;
    if (leg.sStatus.empty () || leg.sStatus == mxconst::get_LEG_STATUS_COMPLETED ())
      ++values.legs_completed_count;

    values.distance_nm += leg.fDistanceFlew;
    values.fuel_used_kg += (leg.fStartingFuel - leg.fEndFuel);
  }

  return values;
}

// -------------------------------------------
// v26.10.2 Two saved flights: the headers, the compare table, both flights in one graph, and the landings and legs side by side.
void
ui_stats_screen::draw_compare_screen ()
{
  const auto &slot_a = missionx::strct_stats_layer.flight_slots.at (0);
  const auto &slot_b = missionx::strct_stats_layer.flight_slots.at (1);

  // ---- header (hidden in the flight path focus view: the plots get the space) ----
  if (!missionx::strct_stats_layer.flag_compare_paths_focus)
  {
    add_back_to_list_button ();
    ImGui::SameLine (0.0f, 20.0f);
    mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_BIG ());
    ImGui::TextColored (missionx::color::color_vec4_white, "%s", "Compare Flights"); // v26.10.2 inner title: white
    mxUiReleaseLastFont ();

    mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
    draw_flight_header (slot_a, "A", COLOR_FLIGHT_A);
    draw_flight_header (slot_b, "B", COLOR_FLIGHT_B);
    mxUiReleaseLastFont ();
    ImGui::Separator ();
  }

  // ---- both flights must be read ----
  ImGui::PushID ("A");
  const bool flag_a_ready = is_slot_ready_or_draw_status (slot_a);
  ImGui::PopID ();
  ImGui::PushID ("B");
  const bool flag_b_ready = is_slot_ready_or_draw_status (slot_b);
  ImGui::PopID ();
  if (!flag_a_ready || !flag_b_ready)
    return;

  // v26.10.2 focus view: only the flight paths, filling the screen
  if (missionx::strct_stats_layer.flag_compare_paths_focus)
  {
    draw_compare_flight_paths (missionx::strct_stats_layer.flight_slots.at (0), missionx::strct_stats_layer.flight_slots.at (1), true);
    return;
  }

  ImGui::BeginChild ("compare_flights_scroll", ImVec2 (0.0f, 0.0f), ImGuiChildFlags_None);

  draw_compare_table (slot_a, slot_b);
  ImGui::NewLine (); // v26.10.2 a clear break between the sections
  ImGui::Separator ();

  // the sections can be closed, so a VR user scrolls less
  mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  const bool flag_graph_open = ImGui::CollapsingHeader ("Flight graph (elevation and airspeed)", ImGuiTreeNodeFlags_DefaultOpen);
  mxUiReleaseLastFont ();
  if (flag_graph_open)
    draw_compare_graph (missionx::strct_stats_layer.flight_slots.at (0), missionx::strct_stats_layer.flight_slots.at (1));

  ImGui::NewLine (); // v26.10.2 a clear break between the sections
  ImGui::Separator ();
  // v26.10.2 flight path plots, one for each flight
  mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  const bool flag_path_open = ImGui::CollapsingHeader ("Flight path (2D/3D)", ImGuiTreeNodeFlags_DefaultOpen);
  mxUiReleaseLastFont ();
  if (flag_path_open)
    draw_compare_flight_paths (missionx::strct_stats_layer.flight_slots.at (0), missionx::strct_stats_layer.flight_slots.at (1), false);

  ImGui::NewLine (); // v26.10.2 a clear break between the sections
  ImGui::Separator ();
  mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  const bool flag_landings_open = ImGui::CollapsingHeader ("Landings", ImGuiTreeNodeFlags_DefaultOpen);
  mxUiReleaseLastFont ();
  if (flag_landings_open)
  {
    // how to show the landings: one after the other (full width, easy to read, also in VR) or two columns
    auto &layer = missionx::strct_stats_layer;
    ImGui::RadioButton ("Landing by landing", &layer.compare_landings_layout, 0);
    ImGui::SameLine (0.0f, 20.0f);
    ImGui::RadioButton ("Side by side", &layer.compare_landings_layout, 1);
    if (layer.compare_landings_layout == 1)
    {
      ImGui::SameLine (0.0f, 20.0f);
      ImGui::Checkbox ("Link scrolling", &layer.flag_compare_link_scroll);
      draw_landings_side_by_side (slot_a, slot_b);
    }
    else
      draw_landings_stacked (slot_a, slot_b);
  }

  ImGui::NewLine (); // v26.10.2 a clear break between the sections
  ImGui::Separator ();
  mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  const bool flag_legs_open = ImGui::CollapsingHeader ("Legs");
  mxUiReleaseLastFont ();
  if (flag_legs_open)
    draw_compare_legs_table (slot_a, slot_b);

  ImGui::EndChild ();
}

// -------------------------------------------
// v26.10.2 The compare table: one row for each value, A and B columns. The better value has a highlighted cell (gold background, yellow text).
void
ui_stats_screen::draw_compare_table (const missionx::mx_stats_flight_slot &in_slot_a, const missionx::mx_stats_flight_slot &in_slot_b)
{
  const auto values_a = get_compare_values (in_slot_a.view_data);
  const auto values_b = get_compare_values (in_slot_b.view_data);

  constexpr ImU32 BETTER_CELL_BG = IM_COL32 (110, 90, 20, 200); // muted gold

  // one value cell: the better one gets the gold background and yellow text
  const auto lmbda_value_cell = [&](const std::string &in_text, const bool in_flag_is_better) {
    ImGui::TableNextColumn ();
    if (in_flag_is_better)
    {
      ImGui::TableSetBgColor (ImGuiTableBgTarget_CellBg, BETTER_CELL_BG);
      ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", in_text.c_str ());
    }
    else
      ImGui::TextColored (missionx::color::color_vec4_white, "%s", in_text.c_str ());
  };

  // a row with numbers. in_rule: which value is better. in_flag_has_values = false: show "-" and no highlight
  const auto lmbda_number_row = [&](const std::string &in_label, const double in_value_a, const double in_value_b, const int in_decimals, const mx_better_rule in_rule,
                                    const bool in_flag_has_values = true) {
    ImGui::TableNextRow ();
    ImGui::TableNextColumn ();
    ImGui::TextColored (missionx::color::color_vec4_beige, "%s", in_label.c_str ());

    if (!in_flag_has_values)
    {
      lmbda_value_cell ("-", false);
      lmbda_value_cell ("-", false);
      return;
    }

    const std::string text_a = fmt::format ("{:.{}f}", in_value_a, in_decimals);
    const std::string text_b = fmt::format ("{:.{}f}", in_value_b, in_decimals);

    // compare what is displayed, so "87.5" and "87.5" are a tie even when the doubles differ a little
    const bool flag_tie     = (text_a == text_b);
    const bool flag_a_wins  = !flag_tie && ((in_rule == mx_better_rule::higher) ? (in_value_a > in_value_b) : (in_value_a < in_value_b));
    const bool flag_compare = (in_rule != mx_better_rule::none);

    lmbda_value_cell (text_a, flag_compare && flag_a_wins);
    lmbda_value_cell (text_b, flag_compare && !flag_tie && !flag_a_wins);
  };

  // a row with text, no "better"
  const auto lmbda_text_row = [&](const std::string &in_label, const std::string &in_text_a, const std::string &in_text_b) {
    ImGui::TableNextRow ();
    ImGui::TableNextColumn ();
    ImGui::TextColored (missionx::color::color_vec4_beige, "%s", in_label.c_str ());
    lmbda_value_cell (in_text_a, false);
    lmbda_value_cell (in_text_b, false);
  };

  constexpr ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings;

  mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
  if (ImGui::BeginTable ("compare_table", 3, table_flags))
  {
    ImGui::TableSetupColumn ("", ImGuiTableColumnFlags_WidthStretch, 1.3f);
    ImGui::TableSetupColumn ("A", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn ("B", ImGuiTableColumnFlags_WidthStretch, 1.0f);

    // header row with the flight colors
    ImGui::TableNextRow (ImGuiTableRowFlags_Headers);
    ImGui::TableNextColumn ();
    ImGui::TextColored (missionx::color::color_vec4_grey, "%s", "Better value: highlighted");
    ImGui::TableNextColumn ();
    ImGui::TextColored (COLOR_FLIGHT_A, "%s", "A");
    ImGui::TableNextColumn ();
    ImGui::TextColored (COLOR_FLIGHT_B, "%s", "B");

    const auto [result_a, color_a] = get_result_text_and_color (in_slot_a.row.outcome, in_slot_a.row.crashed, false);
    const auto [result_b, color_b] = get_result_text_and_color (in_slot_b.row.outcome, in_slot_b.row.crashed, false);
    lmbda_text_row ("Result", result_a, result_b);
    lmbda_text_row ("Time flew", in_slot_a.view_data.stats.time_flew_format_s, in_slot_b.view_data.stats.time_flew_format_s);
    lmbda_number_row ("Distance (nm)", values_a.distance_nm, values_b.distance_nm, 1, mx_better_rule::none, values_a.legs_count > 0 && values_b.legs_count > 0);
    lmbda_number_row ("Fuel used (kg)", values_a.fuel_used_kg, values_b.fuel_used_kg, 1, mx_better_rule::none, values_a.legs_count > 0 && values_b.legs_count > 0);
    lmbda_text_row ("Legs finished", fmt::format ("{}/{}", values_a.legs_completed_count, values_a.legs_count), fmt::format ("{}/{}", values_b.legs_completed_count, values_b.legs_count));

    lmbda_number_row ("Total flight score", values_a.total_score, values_b.total_score, 2, mx_better_rule::higher);
    lmbda_number_row ("  Pitch score", values_a.pitch_score, values_b.pitch_score, 2, mx_better_rule::higher);
    lmbda_number_row ("  Roll score", values_a.roll_score, values_b.roll_score, 2, mx_better_rule::higher);
    lmbda_number_row ("  G score", values_a.g_score, values_b.g_score, 2, mx_better_rule::higher);
    lmbda_number_row ("Max bank (deg)", values_a.max_bank, values_b.max_bank, 1, mx_better_rule::lower);
    lmbda_number_row ("Max G", values_a.max_g, values_b.max_g, 2, mx_better_rule::lower);

    const bool flag_both_graded = (values_a.graded_landings_count > 0 && values_b.graded_landings_count > 0);
    const bool flag_both_landed = (values_a.hardest_td_fpm > 0.0 && values_b.hardest_td_fpm > 0.0);
    lmbda_number_row ("Landings", values_a.landings_count, values_b.landings_count, 0, mx_better_rule::none);
    lmbda_number_row ("Average landing score", values_a.get_average_landing_score_cai (), values_b.get_average_landing_score_cai (), 1, mx_better_rule::higher, flag_both_graded);
    lmbda_number_row ("Best landing score", values_a.best_landing_score, values_b.best_landing_score, 1, mx_better_rule::higher, flag_both_graded);
    lmbda_number_row ("Softest touchdown (fpm)", values_a.softest_td_fpm, values_b.softest_td_fpm, 0, mx_better_rule::lower, flag_both_landed);
    lmbda_number_row ("Hardest touchdown (fpm)", values_a.hardest_td_fpm, values_b.hardest_td_fpm, 0, mx_better_rule::lower, flag_both_landed);
    lmbda_number_row ("Bounces", values_a.bounces_count, values_b.bounces_count, 0, mx_better_rule::lower);

    ImGui::EndTable ();
  }
  mxUiReleaseLastFont ();
}

// -------------------------------------------
// v26.10.2 Both flights in one graph. X = flight time in minutes, or % of the flight (two flights of a different length line up from start to end).
//          A is drawn wider and under B, so when the lines are the same both colors still show.
void
ui_stats_screen::draw_compare_graph (missionx::mx_stats_flight_slot &inout_slot_a, missionx::mx_stats_flight_slot &inout_slot_b)
{
  auto &layer = missionx::strct_stats_layer;

  // ---- graph options ----
  ImGui::RadioButton ("Flight time (min)", &layer.compare_graph_x_axis, 0);
  ImGui::SameLine (0.0f, 20.0f);
  ImGui::RadioButton ("% of the flight", &layer.compare_graph_x_axis, 1);
  ImGui::SameLine (0.0f, 30.0f);
  ImGui::Checkbox ("Airspeed", &layer.flag_compare_graph_airspeed);
  const bool flag_x_is_percent = (layer.compare_graph_x_axis == 1);

  // minutes -> 0..100 % of the flight. Made once for each loaded flight (the slot keeps them)
  const auto lmbda_to_percent = [](const std::vector<double> &in_vecMinutes, const double in_total_minutes, std::vector<double> &out_vecPercent) {
    if (out_vecPercent.size () == in_vecMinutes.size ())
      return; // made already
    out_vecPercent.clear ();
    out_vecPercent.reserve (in_vecMinutes.size ());
    const double total_minutes = std::max (in_total_minutes, 0.001);
    for (const double minute : in_vecMinutes)
      out_vecPercent.emplace_back (minute / total_minutes * 100.0);
  };

  for (auto *slot : { &inout_slot_a, &inout_slot_b })
  {
    const auto &stats = slot->view_data.stats;
    lmbda_to_percent (stats.vecPlotElevMin_d, stats.total_elapsed_min, slot->vecGraphElevX_pct);
    lmbda_to_percent (stats.vecPlotAirMin_d, stats.total_elapsed_min, slot->vecGraphAirX_pct);
  }

  const auto  &stats_a      = inout_slot_a.view_data.stats;
  const auto  &stats_b      = inout_slot_b.view_data.stats;
  const double max_x        = (flag_x_is_percent) ? 100.0 : std::max ({ 1.0, stats_a.total_elapsed_min, stats_b.total_elapsed_min });
  const double max_elev_ft  = std::max (stats_a.max_elev_ft, stats_b.max_elev_ft);
  const double max_airspeed = std::max (stats_a.max_airspeed, stats_b.max_airspeed);

  // the X series of one flight
  const auto lmbda_x_elev = [&](const missionx::mx_stats_flight_slot &in_slot) -> const std::vector<double> & { return (flag_x_is_percent) ? in_slot.vecGraphElevX_pct : in_slot.view_data.stats.vecPlotElevMin_d; };
  const auto lmbda_x_air  = [&](const missionx::mx_stats_flight_slot &in_slot) -> const std::vector<double> & { return (flag_x_is_percent) ? in_slot.vecGraphAirX_pct : in_slot.view_data.stats.vecPlotAirMin_d; };

  // ImPlot 1.1: the line style is an ImPlotSpec passed to PlotLine (it replaced the "set next line style" call)
  const auto lmbda_line_spec = [](const ImVec4 &in_color, const float in_alpha, const float in_weight) -> ImPlotSpec {
    ImPlotSpec spec;
    spec.LineColor  = ImVec4 (in_color.x, in_color.y, in_color.z, in_alpha);
    spec.LineWeight = in_weight;
    return spec;
  };

  if (ImPlot::BeginPlot ("##compare_graph", ImVec2 (-1.0f, 320.0f)))
  {
    ImPlot::SetupAxis (ImAxis_X1, (flag_x_is_percent) ? "% of the flight" : "Flight time (min)", ImPlotAxisFlags_Lock);
    ImPlot::SetupAxisLimits (ImAxis_X1, 0.0, max_x, ImPlotCond_Always);
    ImPlot::SetupAxis (ImAxis_Y1, "Elevation ft", ImPlotAxisFlags_Lock);
    ImPlot::SetupAxisLimits (ImAxis_Y1, 0.0, max_elev_ft + 1000.0, ImPlotCond_Always);
    if (layer.flag_compare_graph_airspeed)
    {
      ImPlot::SetupAxis (ImAxis_Y2, "Airspeed kt", ImPlotAxisFlags_AuxDefault | ImPlotAxisFlags_Lock);
      ImPlot::SetupAxisLimits (ImAxis_Y2, 0.0, max_airspeed + 100.0, ImPlotCond_Always);
    }

    // airspeed first (under the elevation lines): thin and faded, right axis
    if (layer.flag_compare_graph_airspeed)
    {
      ImPlot::SetAxes (ImAxis_X1, ImAxis_Y2);
      const auto &x_air_a = lmbda_x_air (inout_slot_a);
      const auto &x_air_b = lmbda_x_air (inout_slot_b);
      ImPlot::PlotLine ("A airspeed", x_air_a.data (), stats_a.vecPlotAirSpeed_d.data (), static_cast<int> (x_air_a.size ()), lmbda_line_spec (COLOR_FLIGHT_A, 0.55f, 3.0f));
      ImPlot::PlotLine ("B airspeed", x_air_b.data (), stats_b.vecPlotAirSpeed_d.data (), static_cast<int> (x_air_b.size ()), lmbda_line_spec (COLOR_FLIGHT_B, 0.55f, 1.2f));
    }

    // elevation: A wide, B narrow on top
    ImPlot::SetAxes (ImAxis_X1, ImAxis_Y1);
    const auto &x_elev_a = lmbda_x_elev (inout_slot_a);
    const auto &x_elev_b = lmbda_x_elev (inout_slot_b);
    ImPlot::PlotLine ("A elevation", x_elev_a.data (), stats_a.vecPlotElev_ft_d.data (), static_cast<int> (x_elev_a.size ()), lmbda_line_spec (COLOR_FLIGHT_A, 1.0f, 4.0f));
    ImPlot::PlotLine ("B elevation", x_elev_b.data (), stats_b.vecPlotElev_ft_d.data (), static_cast<int> (x_elev_b.size ()), lmbda_line_spec (COLOR_FLIGHT_B, 1.0f, 1.6f));

    ImPlot::EndPlot ();
  }
}

// -------------------------------------------
// v26.10.2 The landings of both flights, one after the other: "Landing 1" A, then B, "Landing 2" A, then B...
//          Full width tables, nothing to scroll sideways.
void
ui_stats_screen::draw_landings_stacked (const missionx::mx_stats_flight_slot &in_slot_a, const missionx::mx_stats_flight_slot &in_slot_b)
{
  // the landings in the order flown (the map key is the touchdown line_id)
  const auto lmbda_landings_in_order = [](const missionx::mx_stats_flight_slot &in_slot) {
    std::vector<std::pair<int, const missionx::mx_landing_analysis *>> vecLandings;
    for (const auto &[td_line_id, landing] : in_slot.view_data.stats.mapLandingAnalysis)
      vecLandings.emplace_back (td_line_id, &landing);
    return vecLandings;
  };

  const auto   vecLandingsA  = lmbda_landings_in_order (in_slot_a);
  const auto   vecLandingsB  = lmbda_landings_in_order (in_slot_b);
  const size_t landings_rows = std::max (vecLandingsA.size (), vecLandingsB.size ());
  if (landings_rows == 0)
  {
    ImGui::TextColored (missionx::color::color_vec4_grey, "%s", "No landings in both flights.");
    return;
  }

  const std::array<const decltype (vecLandingsA) *, 2>             landings_per_flight = { &vecLandingsA, &vecLandingsB };
  const std::array<const missionx::mx_stats_flight_slot *, 2>      slots               = { &in_slot_a, &in_slot_b };
  const std::array<std::string, 2>                                 labels              = { "A", "B" };
  const std::array<ImVec4, 2>                                      label_colors        = { COLOR_FLIGHT_A, COLOR_FLIGHT_B };

  for (size_t landing_index = 0; landing_index < landings_rows; ++landing_index)
  {
    // v26.10.2 a clear break before the next landing: a gap and two separators
    if (landing_index > 0)
    {
      ImGui::Dummy (ImVec2 (0.0f, 18.0f));
      ImGui::Separator ();
      ImGui::Dummy (ImVec2 (0.0f, 2.0f));
      ImGui::Separator ();
      ImGui::Dummy (ImVec2 (0.0f, 8.0f));
    }

    mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
    ImGui::TextColored (missionx::color::color_vec4_aqua, "%s", fmt::format ("Landing {}", landing_index + 1).c_str ());
    mxUiReleaseLastFont ();

    mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
    for (size_t flight_index = 0; flight_index < slots.size (); ++flight_index)
    {
      ImGui::PushID (static_cast<int> (flight_index));
      ImGui::TextColored (label_colors[flight_index], "%s", labels[flight_index].c_str ());
      ImGui::SameLine ();

      const auto &vecLandings = *landings_per_flight[flight_index];
      if (landing_index < vecLandings.size ())
      {
        ImGui::BeginGroup (); // the whole landing to the right of the "A" / "B" label
        draw_one_landing (slots[flight_index]->view_data, vecLandings[landing_index].first, *vecLandings[landing_index].second, 0.0f, false);
        ImGui::EndGroup ();
      }
      else
      {
        ImGui::TextColored (missionx::color::color_vec4_grey, "%s", "No landing");
        ImGui::Separator ();
      }
      ImGui::PopID ();
    }
    mxUiReleaseLastFont ();
    ImGui::Spacing ();
  }
}

// -------------------------------------------
// v26.10.2 The landings in two columns. The tables keep their width, each column scrolls sideways and down.
//          Link scrolling: moving one column moves the other. The column the user moved this frame leads; the other follows the next frame.
void
ui_stats_screen::draw_landings_side_by_side (const missionx::mx_stats_flight_slot &in_slot_a, const missionx::mx_stats_flight_slot &in_slot_b)
{
  constexpr float COLUMN_HEIGHT = 420.0f;
  auto           &layer         = missionx::strct_stats_layer;

  if (!ImGui::BeginTable ("side_by_side_landings", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings))
    return;

  const std::array<const missionx::mx_stats_flight_slot *, 2> slots        = { &in_slot_a, &in_slot_b };
  const std::array<std::string, 2>                            labels       = { "A", "B" };
  const std::array<ImVec4, 2>                                 label_colors = { COLOR_FLIGHT_A, COLOR_FLIGHT_B };

  for (size_t flight_index = 0; flight_index < slots.size (); ++flight_index)
  {
    ImGui::TableNextColumn ();
    ImGui::PushID (labels[flight_index].c_str ());

    ImGui::TextColored (label_colors[flight_index], "%s", labels[flight_index].c_str ());
    ImGui::BeginChild ("landings_column", ImVec2 (0.0f, COLUMN_HEIGHT), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);

    // ---- linked scrolling ----
    if (layer.flag_compare_link_scroll)
    {
      const ImVec2 current_scroll (ImGui::GetScrollX (), ImGui::GetScrollY ());
      auto        &last_scroll = layer.compare_last_scroll.at (flight_index);

      if (current_scroll.x != last_scroll.x || current_scroll.y != last_scroll.y)
        layer.compare_shared_scroll = current_scroll; // the user moved this column: the other one follows

      // follow the shared position, within what this column can scroll (a shorter column stops at its end)
      const ImVec2 target_scroll (std::min (layer.compare_shared_scroll.x, ImGui::GetScrollMaxX ()), std::min (layer.compare_shared_scroll.y, ImGui::GetScrollMaxY ()));
      if (target_scroll.x != current_scroll.x)
        ImGui::SetScrollX (target_scroll.x);
      if (target_scroll.y != current_scroll.y)
        ImGui::SetScrollY (target_scroll.y);

      last_scroll = target_scroll; // what this column will show next frame
    }

    if (slots[flight_index]->view_data.stats.mapLandingAnalysis.empty ())
      ImGui::TextColored (missionx::color::color_vec4_grey, "%s", "No landings.");
    else
      draw_landing_analysis (slots[flight_index]->view_data, 0.0f, true); // natural width tables: the column scrolls sideways

    ImGui::EndChild ();
    ImGui::PopID ();
  }

  ImGui::EndTable ();
}

// -------------------------------------------
// v26.10.2 The legs of both flights in one table: a row for each leg (in the order flown), A and B values next to each other.
void
ui_stats_screen::draw_compare_legs_table (const missionx::mx_stats_flight_slot &in_slot_a, const missionx::mx_stats_flight_slot &in_slot_b)
{
  // vecLegs is newest first: the legs in the order flown
  const std::vector<missionx::mx_enrout_stats_strct> vecLegsA (in_slot_a.view_data.vecLegs.rbegin (), in_slot_a.view_data.vecLegs.rend ());
  const std::vector<missionx::mx_enrout_stats_strct> vecLegsB (in_slot_b.view_data.vecLegs.rbegin (), in_slot_b.view_data.vecLegs.rend ());
  const size_t                                       legs_rows = std::max (vecLegsA.size (), vecLegsB.size ());
  if (legs_rows == 0)
  {
    ImGui::TextColored (missionx::color::color_vec4_grey, "%s", "No leg data (older files).");
    return;
  }

  // a value cell: "-" when the flight has no such leg
  const auto lmbda_number_cell = [](const std::vector<missionx::mx_enrout_stats_strct> &in_vecLegs, const size_t in_index, const ImVec4 &in_color, const std::function<float (const missionx::mx_enrout_stats_strct &)> &in_lmbda_value) {
    ImGui::TableNextColumn ();
    if (in_index < in_vecLegs.size ())
      ImGui::TextColored (in_color, "%.1f", in_lmbda_value (in_vecLegs[in_index]));
    else
      ImGui::TextColored (missionx::color::color_vec4_grey, "%s", "-");
  };

  const auto lmbda_status_cell = [](const std::vector<missionx::mx_enrout_stats_strct> &in_vecLegs, const size_t in_index) {
    ImGui::TableNextColumn ();
    if (in_index >= in_vecLegs.size ())
    {
      ImGui::TextColored (missionx::color::color_vec4_grey, "%s", "-");
      return;
    }
    const auto &status = in_vecLegs[in_index].sStatus;
    if (status.empty () || status == mxconst::get_LEG_STATUS_COMPLETED ())
      ImGui::TextColored (missionx::color::color_vec4_green, "%s", "done");
    else
      ImGui::TextColored (missionx::color::color_vec4_orange, "%s", status.c_str ());
  };

  constexpr ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings;

  mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
  if (ImGui::BeginTable ("compare_legs_table", 8, table_flags))
  {
    ImGui::TableSetupColumn ("#", ImGuiTableColumnFlags_WidthFixed, 22.0f);
    ImGui::TableSetupColumn ("Leg", ImGuiTableColumnFlags_WidthStretch, 3.0f);
    ImGui::TableSetupColumn ("A nm", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn ("B nm", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn ("A fuel kg", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn ("B fuel kg", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn ("A", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn ("B", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableHeadersRow ();

    for (size_t leg_index = 0; leg_index < legs_rows; ++leg_index)
    {
      ImGui::TableNextRow ();

      ImGui::TableNextColumn ();
      ImGui::TextColored (missionx::color::color_vec4_grey, "%d", static_cast<int> (leg_index + 1));

      // the leg name of A. When B flew another leg at this place, both names
      ImGui::TableNextColumn ();
      const std::string name_a = (leg_index < vecLegsA.size ()) ? vecLegsA[leg_index].sLegName : "";
      const std::string name_b = (leg_index < vecLegsB.size ()) ? vecLegsB[leg_index].sLegName : "";
      const std::string leg_name_s = (name_a.empty ()) ? name_b : ((name_b.empty () || name_b == name_a) ? name_a : fmt::format ("{} / {}", name_a, name_b));
      ImGui::TextColored (missionx::color::color_vec4_beige, "%s", leg_name_s.c_str ());

      lmbda_number_cell (vecLegsA, leg_index, COLOR_FLIGHT_A, [](const missionx::mx_enrout_stats_strct &in_leg) { return in_leg.fDistanceFlew; });
      lmbda_number_cell (vecLegsB, leg_index, COLOR_FLIGHT_B, [](const missionx::mx_enrout_stats_strct &in_leg) { return in_leg.fDistanceFlew; });
      lmbda_number_cell (vecLegsA, leg_index, COLOR_FLIGHT_A, [](const missionx::mx_enrout_stats_strct &in_leg) { return in_leg.fStartingFuel - in_leg.fEndFuel; });
      lmbda_number_cell (vecLegsB, leg_index, COLOR_FLIGHT_B, [](const missionx::mx_enrout_stats_strct &in_leg) { return in_leg.fStartingFuel - in_leg.fEndFuel; });
      lmbda_status_cell (vecLegsA, leg_index);
      lmbda_status_cell (vecLegsB, leg_index);
    }
    ImGui::EndTable ();
  }
  mxUiReleaseLastFont ();
}


// -------------------------------------------
// v26.10.2 In-flight "stats" tab. Called every frame the mission tab bar is drawn, with the selected state of the "stats" tab.
//          A click on the tab (it was not selected in the previous drawn frame) asks for the stats. Hiding and showing the
//          window does not draw the tab bar, so it does not ask again.
void
ui_stats_screen::on_stats_tab_drawn_cai (const bool in_flag_tab_is_selected)
{
  auto &state = missionx::strct_in_flight_stats;

  // a mission started, stopped or a checkpoint was loaded: drop the request state. The data in the slot stays until a new copy is read.
  // Only a new click counts: a "stats" tab that is already selected (ImGui keeps the selected tab) does not ask for the stats.
  if (state.reset_seq != missionx::data_manager::in_flight_stats_reset_seq)
  {
    state.reset_seq                   = missionx::data_manager::in_flight_stats_reset_seq;
    state.flag_reading                = false;
    state.flag_stats_tab_was_selected = in_flag_tab_is_selected;
    state.message.clear ();
  }

  if (in_flag_tab_is_selected && !state.flag_stats_tab_was_selected)
    request_in_flight_stats_cai ();

  state.flag_stats_tab_was_selected = in_flag_tab_is_selected;
}

// -------------------------------------------
// v26.10.2 Ask for the in-flight stats. The flcPRE action "prepare_in_flight_stats" asks the writer thread for the copy in the next flight loop.
//          Ignored while a request is active (a quick back and forth between the tabs).
void
ui_stats_screen::request_in_flight_stats_cai ()
{
  auto &state = missionx::strct_in_flight_stats;
  if (missionx::data_manager::flag_is_stats_request_is_active)
    return;

  if (missionx::GatherStats::flag_ignore_stats_setup || missionx::data_manager::gather_stats.get_db_ptr () == nullptr)
  {
    state.message = "The flight stats are not written (setup: \"Ignore stats\"), so there are no stats to show.";
    return;
  }

  state.message.clear ();
  missionx::data_manager::flag_is_stats_request_is_active = true;
  missionx::data_manager::postFlcActions.push_back (missionx::mx_flc_pre_command::prepare_in_flight_stats);
}

// -------------------------------------------
// v26.10.2 The lower part of the in-flight "stats" tab.
//          While the request is active: "Gathering stats information...". The copy is made by the writer thread, then read here by the fetch thread.
//          The request ends when the read ends (or the copy failed). The sim paused: no writer runs, the message stays until the sim runs again.
void
ui_stats_screen::draw_in_flight_stats_cai ()
{
  auto &state = missionx::strct_in_flight_stats;
  auto &slot  = state.slot;

  if (missionx::data_manager::flag_is_stats_request_is_active)
  {
    if (!state.flag_reading)
    {
      // waiting for the copy
      const auto copy_state = missionx::data_manager::gather_stats.get_copy_state_cai ();
      if (copy_state == missionx::mx_stats_copy_state::ready && slot.fetch_state != missionx::mxFetchState_enum::fetch_in_process)
      {
        missionx::data_manager::gather_stats.reset_copy_cai (); // the writer does not touch a "ready" copy

        const std::time_t now = std::time (nullptr);
        std::tm           tm_buf{};
        #if defined(_WIN32)
        localtime_s (&tm_buf, &now);
        #else
        localtime_r (&now, &tm_buf);
        #endif
        char time_s[16]{};
        std::strftime (time_s, sizeof (time_s), "%H:%M:%S", &tm_buf);
        state.copy_local_time_s = time_s;

        // read the copy, like a saved flight
        const std::string copy_path = missionx::data_manager::get_in_flight_stats_db_path_cai ();
        slot.file_path              = copy_path;
        slot.row                    = missionx::mx_stats_history_row ();
        slot.view_data              = missionx::mx_stats_view_data (); // no fetch runs on this slot now
        slot.vecGraphElevX_pct.clear ();
        slot.vecGraphAirX_pct.clear ();
        slot.path_view.reset (); // new data: new waypoint list
        slot.fetch_msg.clear ();
        slot.fetch_state     = missionx::mxFetchState_enum::fetch_in_process;
        state.slot_reset_seq = state.reset_seq;
        state.flag_reading   = true;
        missionx::data_manager::mFetchFutures.push_back (std::async (std::launch::async, missionx::data_manager::fetch_mission_stats_cai, copy_path, &slot.view_data, &slot.fetch_state, &slot.fetch_msg));
      }
      else if (copy_state == missionx::mx_stats_copy_state::failed)
      {
        state.message = fmt::format ("Could not prepare the stats: {}", missionx::data_manager::gather_stats.get_copy_error_cai ());
        missionx::data_manager::gather_stats.reset_copy_cai ();
        missionx::data_manager::flag_is_stats_request_is_active = false;
      }
    }
    else if (slot.fetch_state != missionx::mxFetchState_enum::fetch_in_process)
    {
      // the read ended: the request is over
      state.flag_reading                                      = false;
      missionx::data_manager::flag_is_stats_request_is_active = false;
    }

    if (missionx::data_manager::flag_is_stats_request_is_active)
    {
      mxUiSetFont(mxconst::get_TEXT_TYPE_TEXT_MED());
      ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", "Gathering stats information...\nWon't work in Pause state.");
      mxUiReleaseLastFont();
      return;
    }
  }

  if (!state.message.empty ())
  {
    ImGui::TextColored (missionx::color::color_vec4_orange, "%s", state.message.c_str ());
    return;
  }

  // nothing read for this flight yet (the data in the slot is from an older flight)
  if (state.slot_reset_seq != state.reset_seq || slot.fetch_state == missionx::mxFetchState_enum::fetch_not_started)
  {
    ImGui::TextColored (missionx::color::color_vec4_grey, "%s", "Click the \"stats\" tab to see the stats of this flight so far.");
    return;
  }

  if (!is_slot_ready_or_draw_status (slot))
    return;

  ImGui::TextColored (missionx::color::color_vec4_grey, "%s", fmt::format ("Stats up to {} (local time). Click another tab and back on \"stats\" to update them.", state.copy_local_time_s).c_str ());

  ImGui::BeginChild ("in_flight_stats", ImVec2 (0.0f, 0.0f), ImGuiChildFlags_None);
  mx_stats_draw_options draw_options;
  draw_options.id_suffix = "in_flight";
  draw_options.path_view = &slot.path_view;
  draw_flight_stats (slot.view_data, draw_options);
  ImGui::EndChild ();
}


} // namespace missionx
