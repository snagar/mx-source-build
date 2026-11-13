//
// v26.10.2 ui_stats_screen: draws the stats of one flight (one stats database file).
//          Used by the end screen (the live stats file) and by the stats screen (history flights).
//

#ifndef MISSIONX_UI_STATS_SCREEN_H
#define MISSIONX_UI_STATS_SCREEN_H

#include "shared_ui_types.hpp"
#include "../core/mx_img_window.h"

namespace missionx
{

// v26.10.2 the stats layer state (mx_stats_layer_state, strct_stats_layer) is in shared_ui_types.hpp

class ui_stats_screen : public mx_img_window
{
protected:
  void buildInterface () override {};

private:
  // I am window number...
  const int myWinNum;

public:
  ui_stats_screen (const int left, const int top, const int right, const int bot, const XPLMWindowDecoration decoration, const XPLMWindowLayer layer, const int &in_win_num)
    : mx_img_window (left, top, right, bot, decoration, layer)
    , myWinNum (in_win_num) {};

  // ----- How to draw one flight -----
  struct mx_stats_draw_options
  {
    std::string id_suffix;                    // ImGui ID stack: unique for each flight drawn in the same window ("end_screen", "flight_a", "flight_b")
    float       column_width{ 0.0f };         // width of the summary columns. 0 = the content width of the current window
    float       fallback_max_weight_kg{ 0.0f }; // landing weight percent, when the plane is not in plane_meta (older files)
    ImVec2      graph_child_size{ 0.0f, 315.0f };

    // the legs to list instead of in_view_data.vecLegs. The end screen passes the legs in memory when the leg_stats table is empty. nullptr = use vecLegs
    const std::vector<missionx::mx_enrout_stats_strct> *fallback_legs{ nullptr };

    bool show_summary{ true };
    bool show_legs{ true };
    bool show_landing_analysis{ true };
    bool show_graph{ true };

    // v26.10.2 the flight path plots (2D/3D, picked by waypoint) replace the full flight graph. nullptr = the full flight graph (draw_graph)
    missionx::mx_flight_path_view_state *path_view{ nullptr };
    ImVec4                               path_line_color{ 0.0f, 1.0f, 1.0f, 1.0f }; // aqua
    float                                path_plot_height{ 250.0f }; // the small plot. The focus view uses the screen height
  };

  // ----- The stats layer (home screen "Stats" button) -----
  // ----- Compare screen helpers -----
  static constexpr ImVec4 COLOR_FLIGHT_A{ 0.0f, 1.0f, 1.0f, 1.0f }; // aqua
  static constexpr ImVec4 COLOR_FLIGHT_B{ 1.0f, 0.6f, 0.2f, 1.0f }; // orange

  enum class mx_better_rule : uint8_t
  {
    none = 0, // no "better" value (distance, time)
    higher,   // the higher value is better (scores)
    lower     // the lower value is better (touchdown rate, bounces)
  };

  // the values of one flight that the compare table shows
  struct mx_compare_values
  {
    double total_score{ 0.0 };
    double pitch_score{ 0.0 };
    double roll_score{ 0.0 };
    double g_score{ 0.0 };
    double max_bank{ 0.0 };
    double max_g{ 0.0 };

    int    landings_count{ 0 };
    int    graded_landings_count{ 0 }; // a crashed landing is not graded
    int    bounces_count{ 0 };
    double sum_landing_score{ 0.0 };
    double best_landing_score{ 0.0 };
    double softest_td_fpm{ -1.0 }; // -1 = no touchdown
    double hardest_td_fpm{ 0.0 };

    int    legs_count{ 0 };
    int    legs_completed_count{ 0 };
    double distance_nm{ 0.0 };
    double fuel_used_kg{ 0.0 };

    [[nodiscard]] double get_average_landing_score_cai () const { return (graded_landings_count > 0) ? sum_landing_score / graded_landings_count : 0.0; }
  };

  // ----- The stats layer (home screen "Stats" button). The state is missionx::strct_stats_layer (shared_ui_types.hpp) -----
  static void on_layer_opened (); // called when the home screen opens the layer: back to the list, read the folder again
  static void draw_stats_layer (); // the list of saved flights, one flight, or two flights compared
  static void draw_history_list_screen (); // explanation, buttons and the table
  static void draw_history_table (); // one row for each saved flight
  static std::pair<std::string, ImVec4> get_result_text_and_color (const std::string &in_outcome, bool in_crashed, bool in_flag_on_selected_row); // "Success" in green...
  static void load_flight_slot (int in_slot_index, const std::string &in_file_path); // read a flight into slot A (0) or B (1) in the background
  static void open_one_flight (const std::string &in_file_path); // slot A, one-flight screen
  static void open_two_flights (const std::string &in_file_path_a, const std::string &in_file_path_b); // slots A and B, compare screen
  static void add_back_to_list_button ();
  static void draw_flight_header (const missionx::mx_stats_flight_slot &in_slot, const std::string &in_label, const ImVec4 &in_label_color); // date, plane, title, result
  static bool is_slot_ready_or_draw_status (const missionx::mx_stats_flight_slot &in_slot); // false: draws "Reading..." or the fetch message
  static void draw_one_flight_screen (); // header and the stats of one flight
  static mx_compare_values get_compare_values (const missionx::mx_stats_view_data &in_view_data);
  static void draw_compare_screen (); // headers, compare table, graph, landings and legs side by side
  static void draw_compare_table (const missionx::mx_stats_flight_slot &in_slot_a, const missionx::mx_stats_flight_slot &in_slot_b); // the better value is highlighted
  static void draw_compare_graph (missionx::mx_stats_flight_slot &inout_slot_a, missionx::mx_stats_flight_slot &inout_slot_b); // both flights, X = % of the flight
  static void draw_landings_stacked (const missionx::mx_stats_flight_slot &in_slot_a, const missionx::mx_stats_flight_slot &in_slot_b); // landing 1 of A, landing 1 of B, landing 2 of A...
  static void draw_landings_side_by_side (const missionx::mx_stats_flight_slot &in_slot_a, const missionx::mx_stats_flight_slot &in_slot_b); // two columns, scroll linked
  static void draw_compare_legs_table (const missionx::mx_stats_flight_slot &in_slot_a, const missionx::mx_stats_flight_slot &in_slot_b); // one row for each leg, A and B values next to each other

  // ----- Draw functions: read only their parameters -----
  static void draw_flight_stats (const missionx::mx_stats_view_data &in_view_data, const mx_stats_draw_options &in_draw_options); // all the parts below, top to bottom
  static void draw_summary (const missionx::mx_stats_view_data &in_view_data, float column_width); // time flew, min/max, scores, runway list
  static void draw_legs (const std::vector<missionx::mx_enrout_stats_strct> &in_vec_legs); // the legs, newest first
  static void draw_landing_analysis (const missionx::mx_stats_view_data &in_view_data, float in_fallback_max_weight_kg, bool in_flag_natural_width_table = false); // all landings: grade, touchdown, height steps, notes
  static void draw_one_landing (const missionx::mx_stats_view_data &in_view_data, int td_line_id, const missionx::mx_landing_analysis &landing, float in_fallback_max_weight_kg, bool in_flag_natural_width_table); // one landing. The caller sets the font
  static void draw_graph (const missionx::mx_stats_view_data &in_view_data, const ImVec2 &in_child_size); // elevation and airspeed

  // ----- v26.10.2 Flight path plots (2D/3D): one part of the flight, picked by waypoint and range -----
  // one path in a plot. Offsets move it (merged compare plots)
  struct mx_path_layer
  {
    const missionx::mx_flight_path_cache_entry *entry{ nullptr };
    ImVec4                                      color{ 0.0f, 1.0f, 1.0f, 1.0f };
    std::string                                 label;
    double                                      east_offset_nm{ 0.0 };
    double                                      north_offset_nm{ 0.0 };
    double                                      alt_offset_ft{ 0.0 };
  };

  static void sync_flight_path_sections (const missionx::mx_stats_view_data &in_view_data, missionx::mx_flight_path_view_state &inout_state); // build the waypoint list once for each flight
  static void normalize_flight_path_range (missionx::mx_flight_path_view_state &inout_state); // only the ranges the picked section offers
  static missionx::mx_flight_path_cache_entry *get_or_fetch_flight_path (const missionx::mx_stats_view_data &in_view_data, missionx::mx_flight_path_view_state &inout_state); // cached, or start the background fetch
  static missionx::mx_flight_path_cache_entry *get_ready_flight_path_or_draw_status (const missionx::mx_stats_view_data &in_view_data, missionx::mx_flight_path_view_state &inout_state, float in_reserved_height, bool *out_flag_reading); // nullptr: drew "Reading..." in a space of in_reserved_height. out_flag_reading: the previous section is returned while the new one is read
  static void draw_flight_path_selector (missionx::mx_flight_path_view_state &inout_state, const std::string &in_label, const ImVec4 &in_label_color, bool in_flag_compact = false, bool in_flag_show_range = true); // waypoint combo and range (radio buttons, or a combo when compact)
  static void draw_path_plot (missionx::mx_flight_path_cache_entry &in_entry, const missionx::mx_path_section &in_section, const ImVec4 &in_line_color, const std::string &in_layer_label, bool in_flag_show_3d, missionx::mx_plot_camera &inout_camera, float in_plot_height, float in_box_scale = 1.0f); // one flight, one plot
  static void draw_camera_buttons (missionx::mx_plot_camera &inout_camera); // zoom, 3D/Top/Side, reset
  static void draw_view_buttons (bool &inout_flag_show_3d, missionx::mx_plot_camera &inout_camera); // 2D/3D and the camera buttons
  static constexpr float SMALL_BOX_SCALE = 1.1f; // v26.10.2 3D box scale: one flight (small plot) and the small merged compare plot
  static constexpr float FOCUS_BOX_SCALE = 1.3f; // v26.10.2 3D box scale: the focus views
  // v26.10.2 3D box proportions (before the box scale above). X = east, Y = north, Z = altitude (up).
  //          X and Y can differ without distorting the ground track: the east/north ranges follow them, so one nm is the same length on both.
  //          Z sets how stretched the altitude looks (smaller = gentler climbs and descents).
  static constexpr double BOX_RATIO_X = 1.2;
  static constexpr double BOX_RATIO_Y = 1.2;
  static constexpr double BOX_RATIO_Z = 0.7;
  static void draw_paths_3d (const std::vector<mx_path_layer> &in_layers, missionx::mx_plot_camera &inout_camera, const char *in_z_axis_label, float in_plot_height, float in_box_scale); // left drag rotates
  static void draw_paths_2d (const std::vector<mx_path_layer> &in_layers, bool in_flag_x_is_minutes, const char *in_x_axis_label, const char *in_y_axis_label, float in_plot_height);
  static bool draw_flight_path (const missionx::mx_stats_view_data &in_view_data, missionx::mx_flight_path_view_state &inout_state, const std::string &in_id_suffix, const ImVec4 &in_line_color, float in_plot_height,
                                const char *in_focus_button_label, float in_box_scale = 1.0f); // true: the focus button was pressed
  static bool draw_flight_path_section (const missionx::mx_stats_view_data &in_view_data, missionx::mx_flight_path_view_state &inout_state, const std::string &in_id_suffix, const ImVec4 &in_line_color,
                                        float in_small_plot_height); // small plot + "Focus plot", or the focus view. true: drew the focus view
  static void draw_compare_flight_paths (missionx::mx_stats_flight_slot &inout_slot_a, missionx::mx_stats_flight_slot &inout_slot_b, bool in_flag_focus_view); // split or merged

  // ----- v26.10.2 In-flight "stats" tab: the stats so far, from tmp_stats.db -----
  static void on_stats_tab_drawn_cai (bool in_flag_tab_is_selected); // call every frame the mission tab bar is drawn. A click on the "stats" tab asks for the stats
  static void request_in_flight_stats_cai (); // set the request flag and queue the flcPRE action. Ignored while a request is active
  static void draw_in_flight_stats_cai (); // "Gathering stats information...", or the stats so far (same parts as the end screen)
};

} // namespace missionx

#endif // MISSIONX_UI_STATS_SCREEN_H
