#ifndef dataref_const_h_
#define dataref_const_h_

//#include "base_xp_include.h" // v3.303.14 removed and use more granular headers

#include <string>
#include "XPLMDataAccess.h"

namespace missionx
{


class dataref_const
{
public:
  ~dataref_const(void){};

  dataref_const(void) {
    dref_xplane_version_i = XPLMFindDataRef("sim/version/xplane_internal_version"); // v3.0.241.5 xplane version number as integer

    dref_xplane_using_modern_driver_b = XPLMFindDataRef("sim/graphics/view/using_modern_driver"); // v3.0.241.5 - Only for XP11, deprecated in XP12

    dref_acf_m_empty_weight = XPLMFindDataRef("sim/aircraft/weight/acf_m_empty"); // plane empty weight // v3.0.213.3
    dref_acf_m_max_weight   = XPLMFindDataRef("sim/aircraft/weight/acf_m_max");   // plane max weight (payload + fuel) // v3.0.213.3

    dref_lat_d             = XPLMFindDataRef("sim/flightmodel/position/latitude");    // latitude
    dref_lon_d             = XPLMFindDataRef("sim/flightmodel/position/longitude");   // longitude
    dref_elev_d            = XPLMFindDataRef("sim/flightmodel/position/elevation");   // elevation in Meters
    dref_faxil_gear_f      = XPLMFindDataRef("sim/flightmodel/forces/faxil_gear");    // faxil_gear
    dref_fnrml_gear_f      = XPLMFindDataRef("sim/flightmodel/forces/fnrml_gear");    // sim/flightmodel/forces/fnrml_gear forces on gear
    dref_g_nrml_f          = XPLMFindDataRef("sim/flightmodel/forces/g_nrml");        // Total g-forces on the plane as a multiple, downward
    dref_fnrml_total_f       = XPLMFindDataRef("sim/flightmodel/forces/fnrml_total");   // total forces on plane - newtons
    dref_brake_Left_add_f  = XPLMFindDataRef("sim/flightmodel/controls/l_brake_add"); // l_brake_add - add Left brake
    dref_brake_Right_add_f = XPLMFindDataRef("sim/flightmodel/controls/r_brake_add"); // r_brake_add - add Right brake

    dref_frame_rate_period = XPLMFindDataRef("sim/operation/misc/frame_rate_period"); // v3.0.207.1 // frame_rate_period - fps

    dref_m_fuel_f_arr                    = XPLMFindDataRef("sim/flightmodel/weight/m_fuel"); // weight for each 9 tanks
    dref_m_fuel1_f                       = XPLMFindDataRef("sim/flightmodel/weight/m_fuel1");
    dref_m_fuel2_f                       = XPLMFindDataRef("sim/flightmodel/weight/m_fuel2");
    dref_m_fuel3_f                       = XPLMFindDataRef("sim/flightmodel/weight/m_fuel3");
    dref_m_fuel_total_f                  = XPLMFindDataRef("sim/flightmodel/weight/m_fuel_total");         // weight in Kg
    dref_m_total_f                       = XPLMFindDataRef("sim/flightmodel/weight/m_total");              // Current total payload + fuel weight in Kg
    dref_groundspeed_f                   = XPLMFindDataRef("sim/flightmodel2/position/groundspeed");       // Groundspeed meters/sec
    indicated_airspeed_f                 = XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed"); // indicated_airspeed Kias
    dref_local_vz_f                      = XPLMFindDataRef("sim/flightmodel/position/local_vz");           // Velocity of aircraft in its own coordinate system meters/sec
    dref_local_vx_f                      = XPLMFindDataRef("sim/flightmodel/position/local_vx");           // Velocity of aircraft in its own coordinate system meters/sec
    dref_q                               = XPLMFindDataRef("sim/flightmodel/position/q");                  // quaternion - array of [4]
    dref_acf_h_eqlbm                     = XPLMFindDataRef("sim/aircraft/gear/acf_h_eqlbm");               // equilibrium - height of plane body from ground // calc plane location when moving it
    dref_local_date_days_i               = XPLMFindDataRef("sim/time/local_date_days");                    // local_date_days - Day in year
    dref_local_time_sec_f                = XPLMFindDataRef("sim/time/local_time_sec");                     // local_time_sec - how many milliseconds from midnight. When value between   greater 66600 or less 18000 it will mark as night
    dref_local_time_hours_i              = XPLMFindDataRef("sim/cockpit2/clock_timer/local_time_hours");   // local_time_sec - how many milliseconds from midnight. When value between   greater 66600 or less 18000 it will mark as night
    dref_zulu_time_sec_f                 = XPLMFindDataRef("sim/time/zulu_time_sec");                      // zulu_time_sec - how many milliseconds from ZULU midnight. When value between   greater 66600 or less 18000 it will mark as night

    dref_local_x_d = XPLMFindDataRef("sim/flightmodel/position/local_x"); // local_x
    dref_local_y_d = XPLMFindDataRef("sim/flightmodel/position/local_y"); // local_y
    dref_local_z_d = XPLMFindDataRef("sim/flightmodel/position/local_z"); // local_z

    dref_pitch_f            = XPLMFindDataRef("sim/flightmodel/position/true_theta"); // The pitch of the aircraft relative to the earth precisely below the aircraft
    dref_roll_f             = XPLMFindDataRef("sim/flightmodel/position/true_phi");   // The roll of the aircraft relative to the earth precisely below the aircraft
    dref_heading_true_psi_f = XPLMFindDataRef("sim/flightmodel/position/true_psi");   // The heading of the aircraft relative to the earth precisely below the aircraft - true degrees north, always
    dref_heading_mag_psi_f  = XPLMFindDataRef("sim/flightmodel/position/mag_psi");    // The real magnetic heading of the aircraft - the old magpsi dataref was FUBAR
    dref_heading_psi_f      = XPLMFindDataRef("sim/flightmodel/position/psi");        // The true heading of the aircraft in degrees from the Z axis - OpenGL coordinates

    dref_total_running_time_sec_f = XPLMFindDataRef("sim/time/total_running_time_sec");     // Total time the sim has been up
    dref_vh_ind_f                 = XPLMFindDataRef("sim/flightmodel/position/vh_ind");     // VVI - vertical velocity in meters per second
    dref_gforce_normal_f          = XPLMFindDataRef("sim/flightmodel2/misc/gforce_normal"); //
    dref_gforce_axil_f            = XPLMFindDataRef("sim/flightmodel2/misc/gforce_axil");   //
    AoA_f                         = XPLMFindDataRef("sim/flightmodel2/position/alpha");     // Angle of Attack degrees
    dref_pause                    = XPLMFindDataRef("sim/time/paused");                     // is Sim in pause
    dref_is_in_replay             = XPLMFindDataRef("sim/time/is_in_replay");               // v3.303.14 is Sim in Replay mode



    dref_render_window_width  = XPLMFindDataRef("sim/graphics/view/window_width");  // v3.0.138
    dref_render_window_height = XPLMFindDataRef("sim/graphics/view/window_height"); // v3.0.138


    dref_camera_view_x_f = XPLMFindDataRef("sim/graphics/view/view_x"); // v3.0.219.3
    dref_camera_view_y_f = XPLMFindDataRef("sim/graphics/view/view_y"); // v3.0.219.3
    dref_camera_view_z_f = XPLMFindDataRef("sim/graphics/view/view_z"); // v3.0.219.3


    dref_vvi_fpm_pilot_f          = XPLMFindDataRef("sim/cockpit2/gauges/indicators/vvi_fpm_pilot"); // v3.0.255.1

    dref_vh_ind_fpm_f          = XPLMFindDataRef("sim/flightmodel/position/vh_ind_fpm");  // v3.303.8.3 VVI (vertical velocity in feet per second)
    dref_vh_ind_fpm2_f         = XPLMFindDataRef("sim/flightmodel/position/vh_ind_fpm2"); // v3.303.8.3 VVI (vertical velocity in feet per second)
    dref_Qrad_f                 = XPLMFindDataRef("sim/flightmodel/position/Qrad");        // v3.303.8.3 The pitch rotation rates (relative to the flight)
    dref_Q_f                    = XPLMFindDataRef("sim/flightmodel/position/Q");           // v3.303.8.3 The pitch rotation rates (relative to the flight)
    dref_y_agl_f                = XPLMFindDataRef("sim/flightmodel/position/y_agl");       // v3.303.8.3 Above Ground Level in meters
    dref_onground_any_i         = XPLMFindDataRef("sim/flightmodel/failures/onground_any"); // v3.303.8.3 User Aircraft is on the ground when this is set to 1
    dref_onground_all_i         = XPLMFindDataRef("sim/flightmodel/failures/onground_all"); // v3.303.8.3 I think it means "true" when all wheels are on ground.


    dref_override_planepath_i_arr = XPLMFindDataRef("sim/operation/override/override_planepath");    // v3.0.301 B4

    if ( XPLMGetDatai( dref_xplane_version_i) < 120000) // missionx::XP12_VERSION_NO
      dref_flap_ratio_f = XPLMFindDataRef("sim/cockpit2/controls/flap_ratio");
    else 
      dref_flap_ratio_f = XPLMFindDataRef("sim/cockpit2/controls/flap_system_deploy_ratio"); // v3.0.303.8 xp12 compatibility
      
    
    dref_projection_matrix_3d_arr = XPLMFindDataRef("sim/graphics/view/projection_matrix_3d"); // v3.305.4
    dref_outside_air_temp_degc    = XPLMFindDataRef("sim/cockpit2/temperature/outside_air_temp_degc"); // v24.03.1

    dref_acf_m_station_max_kgs_f_arr = XPLMFindDataRef("sim/aircraft/weight/acf_m_station_max"); // v24.12.2
    dref_m_stations_kgs_f            = XPLMFindDataRef("sim/flightmodel/weight/m_stations"); // v24.12.2


    dref_cg_offset_z_mac_f = XPLMFindDataRef ("sim/flightmodel2/misc/cg_offset_z_mac"); // v25.03.3 Center of gravity - %
    dref_cg_offset_z_f     = XPLMFindDataRef ("sim/flightmodel2/misc/cg_offset_z"); // v25.03.3 Center of gravity - meters
    dref_CG_indicator_f    = XPLMFindDataRef ("sim/cockpit2/gauges/indicators/CG_indicator"); // v25.03.3 Center of gravity - meters
  }

  XPLMDataRef dref_xplane_version_i             ;        // v3.0.241.5 xplane version number as integer
                                                
  XPLMDataRef dref_xplane_using_modern_driver_b ; // v3.0.241.5

  XPLMDataRef dref_acf_m_empty_weight ; // plane empty weight // v3.0.213.3
  XPLMDataRef dref_acf_m_max_weight   ; // plane max weight (payload + fuel) // v3.0.213.3

  XPLMDataRef dref_lat_d             ; // latitude
  XPLMDataRef dref_lon_d             ; // longitude
  XPLMDataRef dref_elev_d            ; // elevation
  XPLMDataRef dref_faxil_gear_f      ; // faxil_gear
  XPLMDataRef dref_fnrml_gear_f      ; // sim/flightmodel/forces/fnrml_gear
  XPLMDataRef dref_g_nrml_f          ; // sim/flightmodel/forces/g_nrml
  XPLMDataRef dref_fnrml_total_f       ; // sim/flightmodel/forces/fnrml_total
  XPLMDataRef dref_brake_Left_add_f  ; // l_brake_add - add Left brake
  XPLMDataRef dref_brake_Right_add_f ; // r_brake_add - add Right brake

  XPLMDataRef dref_frame_rate_period ; // v3.0.207.1 // frame_rate_period - fps

  XPLMDataRef dref_m_fuel_f_arr     ;// weight for each 9 tanks
  XPLMDataRef dref_m_fuel1_f        ;
  XPLMDataRef dref_m_fuel2_f        ;
  XPLMDataRef dref_m_fuel3_f        ;
  XPLMDataRef dref_m_fuel_total_f   ; // weight in Kg
  XPLMDataRef dref_m_total_f        ; // Current payload + Fuel weight in kg
  XPLMDataRef dref_groundspeed_f    ;  // Groundspeed meters/sec
  XPLMDataRef indicated_airspeed_f  ; // indicated_airspeed Kias
  XPLMDataRef dref_local_vz_f       ; // Velocity of aircraft in its own coordinate system meters/sec
  XPLMDataRef dref_local_vx_f       ; // Velocity of aircraft in its own coordinate system meters/sec
  XPLMDataRef dref_q                ;              // quaternion - array of [4]
  XPLMDataRef dref_acf_h_eqlbm      ;              // equilibrium - height of plane body from ground // calc plane location when moving it
  // dataref for step statistics
  XPLMDataRef dref_local_date_days_i ; // local_date_days - Day in year
  XPLMDataRef dref_local_time_sec_f  ; // local_time_sec - how many milliseconds from midnight. When value between   greater 66600 or less 18000 it will mark as night
  XPLMDataRef dref_local_time_hours_i; // local_time_sec - how many milliseconds from midnight. When value between   greater 66600 or less 18000 it will mark as night
  XPLMDataRef dref_zulu_time_sec_f   ; // zulu_time_sec - how many milliseconds from ZULU midnight. When value between   greater 66600 or less 18000 it will mark as night

  XPLMDataRef dref_local_x_d ; // local_x
  XPLMDataRef dref_local_y_d ; // local_y
  XPLMDataRef dref_local_z_d ; // local_z

  XPLMDataRef dref_pitch_f            ; // The pitch of the aircraft relative to the earth precisely below the aircraft
  XPLMDataRef dref_roll_f             ; // The roll of the aircraft relative to the earth precisely below the aircraft
  XPLMDataRef dref_heading_true_psi_f ; // The heading of the aircraft relative to the earth precisely below the aircraft - true degrees north, always
  XPLMDataRef dref_heading_mag_psi_f  ; // The real magnetic heading of the aircraft - the old magpsi dataref was FUBAR
  XPLMDataRef dref_heading_psi_f      ; // The true heading of the aircraft in degrees from the Z axis - OpenGL coordinates

  XPLMDataRef dref_total_running_time_sec_f; // Total time the sim has been up
  XPLMDataRef dref_vh_ind_f                ; // VVI - vertical velocity in meters per second
  XPLMDataRef dref_gforce_normal_f         ; //
  XPLMDataRef dref_gforce_axil_f           ; //
  XPLMDataRef AoA_f                        ; // Angle of Attack degrees
  XPLMDataRef dref_pause                   ; // is Sim in pause
  XPLMDataRef dref_is_in_replay            ; // is Sim in Replay Mode

  // v3.0.138
  XPLMDataRef dref_render_window_width  ; // v3.0.138
  XPLMDataRef dref_render_window_height ; // v3.0.138

  // v3.0.219.3
  XPLMDataRef dref_camera_view_x_f; // v3.0.219.3
  XPLMDataRef dref_camera_view_y_f; // v3.0.219.3
  XPLMDataRef dref_camera_view_z_f; // v3.0.219.3

  // v3.0.221.11 CUSTOM datarefs should not be XPLMDataRef const since they won't be available until plugin will finish initialization. Instead we will hold their "path"
  std::string dref_xpshared_target_status_i_str;  // v3.0.221.9  -1: fail, 0: waiting 1: success
  std::string dref_xpshared_target_listen_plugin_available_i_str;// v3.0.221.9  1=listen 0 =no one is listening

  // gather stats
  XPLMDataRef dref_vvi_fpm_pilot_f;

  XPLMDataRef dref_vh_ind_fpm_f;            // v3.303.8.3 VVI (vertical velocity in feet per second)
  XPLMDataRef dref_vh_ind_fpm2_f;           // v3.303.8.3 VVI (vertical velocity in feet per second)
  XPLMDataRef dref_Qrad_f;                   // v3.303.8.3 The pitch rotation rates (relative to the flight)
  XPLMDataRef dref_Q_f;                      // v3.303.8.3 The pitch rotation rates (relative to the flight)
  XPLMDataRef dref_y_agl_f;                  // v3.303.8.3 AGL in meters (Above Ground Level)
  XPLMDataRef dref_onground_any_i;           // v3.303.8.3 User Aircraft is on the ground when this is set to 1
  XPLMDataRef dref_onground_all_i;           // v3.303.8.3 I think it is 1 when all wheels are touching ground


  XPLMDataRef dref_override_planepath_i_arr; // v3.0.301 B4
  XPLMDataRef dref_projection_matrix_3d_arr; // v3.305.4 
  XPLMDataRef dref_outside_air_temp_degc; // v24.03.1 sim/cockpit2/temperature/outside_air_temp_degc 

  // X-Plane 11 specific Datarefs
  XPLMDataRef dref_flap_ratio_f; 


  // X-Plane 12 specific Datarefs
  XPLMDataRef dref_acf_m_station_max_kgs_f_arr; // 24.12.2 acf_m_station_max - N'th payload stations maximum weight, as specified by author.
  XPLMDataRef dref_m_stations_kgs_f; // 24.12.2 Payload Weight per station, if airplane has stations

  XPLMDataRef dref_CG_indicator_f; // v25.03.3 center of gravity cg_offset_z_mac - meter sim/cockpit2/gauges/indicators/CG_indicator
  XPLMDataRef dref_cg_offset_z_mac_f; // v25.03.3 center of gravity cg_offset_z_mac %
  XPLMDataRef dref_cg_offset_z_f; // v25.03.3 center of gravity sim/flightmodel2/misc/cg_offset_z (meter)


  // const XPLMDataRef dref_ {template}
};

} // missionx
#endif // dataref_const_h_
