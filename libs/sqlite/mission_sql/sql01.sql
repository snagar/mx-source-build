select * from xp_rw;

update xp_airports
set icao = (
                select t1.val_col
                from xp_ap_metadata t1
                where t1.key_col = 'icao_code'
                and t1.icao != t1.val_col
                and t1.icao_id = xp_airports.icao_id
          )
;



select icao
from  xp_airports v1, xp_ap_metadata v2
where v1.icao = v2.icao
and 'datum_lat' = v2.key_col
;


select * from xp_airports;

select *
from airports_vu
where icao = 'ICJL'
;


select mx_calc_distance(37.1967503,-107.8722528, 45.4731446, -105.4648861, 3440) as distance
from dual;

select * from airports_vu;

select v1.*
from  (
select mx_calc_distance(37.1967503,-107.8722528, v1.ap_lat , v1.ap_lon, 3440) as distance, v1.icao_id, v1.icao, v1.ap_name, v1.ap_lat, v1.ap_lon
from airports_vu v1
where v1.is_oilrig > 0
) as v1
where distance <= 100
order by distance
;


select mx_bearing(37.1967503,-107.8722528, 45.4731446, -105.4648861) as bearing
from dual;

select mx_get_center_between_2_points(37.1967503,-107.8722528, 45.4731446, -105.4648861) as bearing
from dual;


select mx_get_center_between_2_points(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as center_of_runway, t1.rw_no_1 || '-' || t1.rw_no_2 as runway, t1.rw_length_mt
from xp_rw t1 where t1.icao= 'KSEA' order by rw_length_mt desc 
;

select icao, distance_nm, loc_rw, loc_type, frq_mhz, loc_bearing, rw_length_mt, rw_width, ap_elev, ap_name, surf_type_text, bearing_from_to_icao from(
select xp_loc.icao, round(3440 * acos(cos(radians(?1)) * cos(radians(xp_loc.lat)) * cos(radians(?2) - radians(xp_loc.lon)) + sin(radians(?3)) * sin(radians(xp_loc.lat)))) as distance_nm, xp_loc.loc_rw, xp_loc.loc_type, xp_loc.frq_mhz, xp_loc.loc_bearing, xp_rw.rw_length_mt, xp_rw.rw_width, xa.ap_elev, xa.ap_name
, case xp_rw.rw_surf when 1 then 'Asphalt' when 2 then 'Concrete' when 3 then 'Turf or grass' when 4 then 'Dirt' when 5 then 'Gravel' when 12 then 'Dry lakebed' when 13 then 'Water runways' when 14 then 'Snow or ice' when 15 then 'Transparent' else 'other' end as surf_type_text
, mx_bearing_func(?4, ?5, xp_loc.lat, xp_loc.lon) as bearing_from_to_icao
from xp_loc, xp_rw, xp_airports xa
where xp_rw.icao = xp_loc.icao
and (xp_rw.rw_no_1 = xp_loc.loc_rw or xp_rw.rw_no_2 = xp_loc.loc_rw)
and xa.icao = xp_rw.icao)
where 1 = 1   and distance_nm between 50 and 250 and rw_length_mt >= 1000 and rw_width >= 45 and ap_elev >= 0 
;

drop table if exists xp_airports;

create table if not exists xp_airports ( icao text, ap_elev integer, ap_name text, is_custom int, primary key (icao, ap_name) )
;
select * from xp_airports 
where is_custom is not null
order by icao_id
;

select (select feature_version from dual) as feature_version, (select count(icao_id) from xp_airports) as count_xp_airports, (select count(icao_id) from xp_ap_ramps) as count_xp_ramps, (select count(lat) from xp_loc) as count_xp_loc
from dual
;


create index indx_icao_id_n1 on xp_ap_metadata (icao_id);

update xp_airports 
set ap_lat = IFNULL( (select  val_col from xp_ap_metadata where xp_airports.icao_id = xp_ap_metadata.icao_id and xp_ap_metadata.key_col = 'datum_lat' limit 1), xp_airports.ap_lat )
  , ap_lon = IFNULL( (select  val_col from xp_ap_metadata where xp_airports.icao_id = xp_ap_metadata.icao_id and xp_ap_metadata.key_col = 'datum_lon' limit 1), xp_airports.ap_lon )
;






SELECT * FROM xp_airports
  WHERE icao_id IN (
    SELECT icao_id
    FROM (
      SELECT t1.*,
        LAG(icao_id, 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao) pos_in_group,
        mx_distance_nm(t1.ap_lat, t1.ap_lon, LAG(t1.ap_lat, 1, t1.ap_lat) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id), LAG(t1.ap_lon, 1, t1.ap_lon) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id), 3440) AS distance,
        trunc(t1.ap_lat) - LAG(trunc(t1.ap_lat), 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id) AS lat_diff,
        trunc(t1.ap_lon) - LAG(trunc(t1.ap_lon), 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id) AS lon_diff
      FROM xp_airports t1
      WHERE t1.icao IN (
        SELECT icao
        FROM xp_airports t3
        GROUP BY icao
        HAVING count(icao) > 1
        )
      ORDER BY icao,
      icao_id
    ) v1
  WHERE v1.pos_in_group != 0
  AND (v1.distance < 5 OR (lat_diff = 0 AND lon_diff = 0) ) 
  )
;

select "EGTF" as icao, mx_is_plane_in_rw_area( 51.348055556, -0.558611111, 51.34608733, -0.56364863, 51.34990632, -0.55394151, 27.13) as "is_plane_on_runway"
from dual
;

select xp_rw.icao, mx_is_plane_in_rw_area(51.348055556, -0.558611111, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon, xp_rw.rw_width) as in_rw_area, xp_rw.*
from xp_rw
where in_rw_area > 0;

--  32.011534,  34.870985 LLBG, 51.348055556, -0.558611111 EGTF 
select xp_rw.icao, mx_is_plane_in_rw_area               ( -19.25502, 146.76624, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon, xp_rw.rw_width) as in_rw_area
                 , mx_get_shortest_distance_to_rw_vector( -19.25502, 146.76624, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon) as distance_to_center_line
                 , xp_rw.*
from xp_rw
where 1 = 1 
and xp_rw.icao = 'YBTL'
--and in_rw_area = 1
order by distance_to_center_line
;

select elev, flap_ratio * 100 as flap_ratio, local_date_days, local_time_sec, case when airspeed < 0.0 then 0.0 else airspeed end as airspeed , groundspeed, faxil_gear, roll, activity from stats order by line_id
;

-- query if plane is in airport boundary. 
select xp_airports.icao, xp_airports.ap_name, xp_airports.ap_lat, xp_airports.ap_lon, xp_airports.ap_elev, mx_bearing (47.4391861, -122.31519317, xp_airports.ap_lat, xp_airports.ap_lon) as heading,  mx_is_plane_in_airport_boundary(47.4391861, -122.31519317, xp_airports.boundary) as is_plane_in_boundary, xp_airports.boundary, length(xp_airports.boundary)
from xp_airports
where 1 = 1 
--and xp_airports.icao = 'KSEA'
--and is_plane_in_boundary = 1
and length(xp_airports.boundary) > 0
order by length(xp_airports.boundary) desc
;

select xp_airports.icao, xp_airports.ap_name, xp_airports.ap_lat, xp_airports.ap_lon, xp_airports.ap_elev , mx_bearing(61.1678119, -150.042433574, xp_airports.ap_lat, xp_airports.ap_lon) as heading , mx_is_plane_in_airport_boundary(61.1678119, -150.042433574, xp_airports.boundary) as is_plane_in_boundary, xp_airports.boundary  
from xp_airports where 1 = 1 
and is_plane_in_boundary = 1
and xp_airports.boundary is not null

;


select xp_airports.icao, xp_airports.ap_name, xp_airports.ap_lat, xp_airports.ap_lon, xp_airports.ap_elev , mx_bearing(61.1678119, -150.042433574, xp_airports.ap_lat, xp_airports.ap_lon) as heading , mx_is_plane_in_airport_boundary(61.1678119, -150.042433574, xp_airports.boundary) as is_plane_in_boundary  from xp_airports where is_plane_in_boundary = 1
;

select * from xp_airports;

select * from xp_airports where 1 = 1
and boundary is not null
and icao='YBTL'
--order by icao
;

select ap_type, count(1) from xp_airports 
where 1 = 1
and boundary is null
group by ap_type
;



select max(line_id) as last_id from stats;



-- Fetch time passes between activities. First row will always be 0
-- The query will convert days into seconds in order to have correct delta result between "current - previous" fields
select v1.*, case 
                  when v1.local_date_days >= v1.prev_date_days then ( v1.local_date_days_in_seconds + v1.local_time_sec ) - ( v1.prev_local_date_days_in_seconds + v1.prev_local_time_sec) -- should pick 99.9% of cases
                  when v1.local_date_days < v1.prev_date_days  then ( (prev_local_date_days_in_seconds + 86400) + v1.local_time_sec ) - (v1.prev_local_date_days_in_seconds + v1.prev_local_time_sec) -- transition a year, very rare
             else 0 end time_passed
from (
select line_id, local_date_days, local_date_days*86400 as local_date_days_in_seconds, local_time_sec, lat, lon, elev as elev_mt, gforce_normal, gforce_axil, vvi_fpm_pilot, activity, lag(local_date_days) over (order by line_id) as prev_date_days, (lag(local_date_days) over (order by line_id) * 86400)  as prev_local_date_days_in_seconds, lag (local_time_sec) over (order by line_id) as prev_local_time_sec 
from stats
where (stats.activity != "" and stats.activity is not null)
) v1
;

-- Q1 fetch plane position during activities
select line_id, plane_lat, plane_lon, elev_mt, activity
           , case 
                  when v1.local_date_days >= v1.prev_date_days then ( v1.local_date_days_in_seconds + v1.local_time_sec ) - ( v1.prev_local_date_days_in_seconds + v1.prev_local_time_sec) -- should pick 99.9% of cases
                  when v1.local_date_days < v1.prev_date_days  then ( (prev_local_date_days_in_seconds + 86400) + v1.local_time_sec ) - (v1.prev_local_date_days_in_seconds + v1.prev_local_time_sec) -- transition a year, very rare
             else 0 end time_passed
from (
select line_id, local_date_days, local_date_days*86400 as local_date_days_in_seconds, local_time_sec, lat as plane_lat, lon as plane_lon, elev as elev_mt, gforce_normal, gforce_axil, vvi_fpm_pilot, activity, lag(local_date_days) over (order by line_id) as prev_date_days, (lag(local_date_days) over (order by line_id) * 86400)  as prev_local_date_days_in_seconds, lag (local_time_sec) over (order by line_id) as prev_local_time_sec 
from stats
where (stats.activity != "" and stats.activity is not null)
) v1
; -- -20.046856, 146.267674

-- Q2 fetch Airport the plane was during the activities in Q1
select icao_id, icao, mx_is_plane_in_airport_boundary(-20.046856, 146.267674, boundary) as plane_in_boundary, length(boundary), boundary 
from xp_airports
where 1 = 1 
--and xp_airports.boundary is not null
--and plane_in_boundary = 1
and icao = 'YCHT'
--order by length(boundary)
; -- KSEA, 37803

-- Q3 fetch distance from center of rw, if plane landed 
--select xp_rw.*, mx_is_plane_in_rw_area(47.434847, -122.307952, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon, xp_rw.rw_width) as is_plane_on_rw,  mx_get_shortest_distance_to_rw_vector ( 47.434847, -122.307952, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon) as distance_from_center
select xp_rw.rw_no_1 || '-' || xp_rw.rw_no_2 as runway,  mx_get_shortest_distance_to_rw_vector ( 46.990420999999998, -123.43265800000000, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon) as distance_from_center
                                                                       , mx_is_plane_in_rw_area( 46.990420999999998, -123.43265800000000, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon, xp_rw.rw_width) as is_plane_on_rw
      , icao, icao_id
from xp_rw
where 1 = 1 
and xp_rw.icao_id = 5484
and is_plane_on_rw = 1
;



-- Q2 + Q3
--select icao_id, icao, mx_is_plane_in_airport_boundary(47.434847, -122.307952, boundary) as plane_in_boundary, length(boundary), xp_rw.*, mx_is_plane_in_rw_area(47.434847, -122.307952, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon, xp_rw.rw_width) as is_plane_on_rw,  mx_get_shortest_distance_to_rw_vector ( 47.434847, -122.307952, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon) as distance_from_center
select xp_rw.icao_id, xp_rw.icao,   mx_get_shortest_distance_to_rw_vector ( 47.434847, -122.307952, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon) as distance_from_center, mx_is_plane_in_airport_boundary(47.434847, -122.307952, boundary) as plane_in_boundary, length(boundary), xp_rw.*, mx_is_plane_in_rw_area(47.434847, -122.307952, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon, xp_rw.rw_width) as is_plane_on_rw
from xp_airports, xp_rw
where xp_airports.boundary is not null
and plane_in_boundary = 1
and xp_airports.icao_id = xp_rw.icao_id
and is_plane_on_rw = 1
;


-- stmt2
select icao_id, icao, mx_is_plane_in_airport_boundary(47.446158, -122.308015, boundary) as plane_in_boundary
from 
(
select icao_id, icao, length(boundary), boundary 
from xp_airports
where 1 = 1
and ( trunc(xp_airports.ap_lat) between trunc(47.446158 - 1.0) and  trunc(47.446158 + 1.0) )
and ( trunc(xp_airports.ap_lon) between trunc(-122.308015-1.0) and  trunc(-122.308015+1.0) )
) v1
where plane_in_boundary = 1
and boundary is not null
;



SELECT line_id,
       vvi_fpm_pilot,
       vvi_ind_fpm,
       vh_ind,
--       flap_ratio,
       local_date_days,
       local_time_sec,
       lat,
       lon,
       elev,
       airspeed,
       groundspeed,
       faxil_gear,
--       brakes_L,
--       brakes_R,
       gforce_normal,
       gforce_axil,
       AoA,
       pitch,
       Qrad,
       Q,
       agl,       
       roll,
--       heading_mag,
--       heading_no_mag,
       activity,
       onground_any,
       onground_all
  FROM stats
order by line_id  
;

--CREATE VIEW stats_summary AS
  SELECT min(t1.vvi_fpm_pilot) AS min_vvi_fpm_pilot,
         max(t1.vvi_fpm_pilot) AS max_vvi_fpm_pilot,
--         min(t1.vh_ind_fpm) AS min_vh_ind_fpm,
 --        max(t1.vh_ind_fpm) AS max_vh_ind_fpm,
         min(t1.airspeed) AS min_airspeed,
         max(t1.airspeed) AS max_airspeed,
         min(t1.groundspeed) AS min_groundspeed,
         max(t1.groundspeed) AS max_groundspeed,
         min(t1.vh_ind) AS min_vh_ind,
         max(t1.vh_ind) AS max_vh_ind,
         min(t1.gforce_normal) AS min_gforce_normal,
         max(t1.gforce_normal) AS max_gforce_normal,
         min(t1.gforce_axil) AS min_gforce_axil,
         max(t1.gforce_axil) AS max_gforce_axil,
         min(t1.aoa) AS min_aoa,
         max(t1.aoa) AS max_aoa,
         min(t1.pitch) AS min_pitch,
         max(t1.gforce_axil) AS max_pitch,
         min(t1.roll) AS min_roll,
         max(t1.roll) AS max_roll
    FROM stats t1;

;

select icao, ap_name, ap_lat, ap_lon, ap_elev, boundary 
      , mx_bearing( -20.013188, 146.483738, ap_lat, ap_lon ) as heading
      , mx_is_plane_in_airport_boundary( -20.013188, 146.483738, boundary ) as is_plane_in_boundary
      , icao_id
from 
(
select xp_airports.icao_id, xp_airports.icao, xp_airports.ap_name, xp_airports.ap_lat, xp_airports.ap_lon, xp_airports.ap_elev, xp_airports.boundary
from xp_airports
where xp_airports.boundary is not null
and ( trunc(xp_airports.ap_lat) between trunc( -20.013188 - 1.0) and  trunc( -20.013188 + 1.0) )
and ( trunc(xp_airports.ap_lon) between trunc( 146.483738 - 1.0) and  trunc( 146.483738 + 1.0) )
) v1
where is_plane_in_boundary = 1
;    


select * from xp_airports 
where xp_airports.icao = 'YMCS'
; -- -20.013188, 146.483738



-- Q3 
select xp_rw.rw_no_1 || '-' || xp_rw.rw_no_2 as runway,  mx_get_shortest_distance_to_rw_vector ( -20.01116157, 146.49190380, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon) as distance_from_center
                                                                       , mx_is_plane_in_rw_area( -20.01116157, 146.49190380, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon, xp_rw.rw_width) as is_plane_on_rw
      , icao, icao_id
from xp_rw
where 1 = 1 
and xp_rw.icao_id = 36202
and is_plane_on_rw = 1
;
-- -20.01116157, 146.49190380

select line_id, vvi_fpm_pilot, local_time_sec, case when  time_passed_sec is NULL then 0 else time_passed_sec end as time_spent_in_activity, lat, lon, elev, faxil_gear, gforce_normal, pitch, roll, activity, onground_any, onground_all, agl
from
(
SELECT        
       line_id, vvi_fpm_pilot, flap_ratio, local_date_days,
       local_time_sec,
       --lead(local_time_sec) over (order by line_id) as lead_time_sec,
       lead(local_time_sec) over (order by line_id) - local_time_sec as time_passed_sec,
       lat, lon, elev, airspeed, groundspeed, vh_ind, faxil_gear, brakes_L, brakes_R,
       gforce_normal, gforce_axil, AoA, pitch, roll, heading_mag,  heading_no_mag, 
       activity, onground_any, onground_all, agl
  FROM stats
)
;


select line_id, plane_lat, plane_lon, activity
           , case 
                  when v1.local_date_days >= v1.prev_date_days then ( v1.local_date_days_in_seconds + v1.local_time_sec ) - ( v1.prev_local_date_days_in_seconds + v1.prev_local_time_sec) -- should pick 99.9% of cases
                  when v1.local_date_days < v1.prev_date_days  then ( (prev_local_date_days_in_seconds + 86400) + v1.local_time_sec ) - (v1.prev_local_date_days_in_seconds + v1.prev_local_time_sec) -- transition a year, very rare
             else 0 end time_passed
from (
select line_id, local_date_days, local_date_days*86400 as local_date_days_in_seconds, local_time_sec, lat as plane_lat, lon as plane_lon, elev as elev_mt, gforce_normal, gforce_axil, vvi_fpm_pilot, activity, lag(local_date_days) over (order by line_id) as prev_date_days, (lag(local_date_days) over (order by line_id) * 86400)  as prev_local_date_days_in_seconds, lag (local_time_sec) over (order by line_id) as prev_local_time_sec 
from stats
where (stats.activity != "" and stats.activity is not null)
) v1
;

-- p.lat: -18.758044000000002,146.58370400000001
select icao, ap_name, ap_lat, ap_lon, ap_elev, boundary 
      , mx_bearing( -18.758044000000002,146.58370400000001, ap_lat, ap_lon ) as heading
      , mx_is_plane_in_airport_boundary( -18.758044000000002,146.58370400000001, boundary ) as is_plane_in_boundary
      , icao_id
from 
(
  select xp_airports.icao_id, xp_airports.icao, xp_airports.ap_name, xp_airports.ap_lat, xp_airports.ap_lon, xp_airports.ap_elev, xp_airports.boundary
  from xp_airports
  where xp_airports.boundary is not null
  and ( trunc(xp_airports.ap_lat) between trunc( -18.758044000000002 - 1.0) and  trunc( -18.758044000000002 + 1.0) )
  and ( trunc(xp_airports.ap_lon) between trunc( 146.58370400000001 - 1.0) and  trunc( 146.58370400000001 + 1.0) )
) v1
where is_plane_in_boundary = 1
;


-- p.lat: -19.242014000000001, 146.77243000000001
select icao, ap_name, ap_lat, ap_lon, ap_elev
      , mx_bearing( -19.242014000000001, 146.77243000000001, ap_lat, ap_lon ) as heading
      , mx_is_plane_in_airport_boundary( -19.242014000000001, 146.77243000000001, boundary ) as is_plane_in_boundary
      , icao_id, boundary 
from 
(
  select xp_airports.icao_id, xp_airports.icao, xp_airports.ap_name, xp_airports.ap_lat, xp_airports.ap_lon, xp_airports.ap_elev, xp_airports.boundary
  from xp_airports
  where xp_airports.boundary is not null
  and ( trunc(xp_airports.ap_lat) between trunc( -19.242014000000001 - 1.0) and  trunc( -19.242014000000001 + 1.0) )
  and ( trunc(xp_airports.ap_lon) between trunc( 146.77243000000001 - 1.0) and  trunc( 146.77243000000001 + 1.0) )
) v1
where is_plane_in_boundary = 1
;


select xp_rw.rw_no_1 || '-' || xp_rw.rw_no_2 as runway
, mx_get_shortest_distance_to_rw_vector ( -19.242014000000001, 146.77243000000001, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon) as distance_from_center
, mx_is_plane_in_rw_area( -19.242014000000001, 146.77243000000001, xp_rw.rw_no_1_lat, xp_rw.rw_no_1_lon, xp_rw.rw_no_2_lat, xp_rw.rw_no_2_lon, xp_rw.rw_width) as is_plane_on_rw
from xp_rw
where is_plane_on_rw = 1 
;


select mx_get_center_between_2_points(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as center_of_runway, t1.rw_no_1 || '-' || t1.rw_no_2 as name, t1.rw_length_mt from xp_rw t1 where t1.icao= 'YBTL' order by rw_length_mt desc limit 1
;

select *
from xp_rw t1 where t1.icao= 'LS11'
;

select t1.rw_no_1_lat || ',' || rw_no_1_lon as start_pos
       ,'RW:' || t1.rw_no_1 as name
       , mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as heading
       , mx_get_point_based_on_bearing_and_length_in_meters (t1.rw_no_1_lat, rw_no_1_lon, 90.0, t1.rw_width * 0.5) -- find center of runway width
       , t1.rw_length_mt 
from xp_rw t1 
where t1.icao= 'LS11' order by rw_length_mt desc limit 1;

select icao_id, icao, ap_elev_ft, ap_name, ap_type, ap_lat, ap_lon , mx_calc_distance ( ap_lat, ap_lon,32.153488159180, -91.843742370605, 3440) as dist_nm, 0 as bearing , helipads, ramp_helos, ramp_planes, ramp_props, ramp_turboprops, ramp_jet_heavy, rw_hard, rw_dirt_gravel, rw_grass , rw_water, is_custom from airports_vu where 1 = 1 and icao = 'LS11' order by dist_nm
;

select mx_get_point_based_on_bearing_and_length_in_meters (t1.rw_no_1_lat, rw_no_1_lon, 90.0, t1.rw_width * 0.5) as start_pos, t1.rw_no_1 as name, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as heading, t1.rw_length_mt, t1.rw_no_1_lat || ',' || rw_no_1_lon from xp_rw t1 where t1.icao= 'LS11' order by rw_length_mt desc limit 1


-- lat="32.15683746" long="-91.84336090"
-- 32.15683982,-91.84336185

;
select mx_get_point_based_on_bearing_and_length_in_meters (t1.rw_no_1_lat, rw_no_1_lon, 90.0, t1.rw_width * 0.5) as start_pos, t1.rw_no_1 as name, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as heading, t1.rw_length_mt, t1.rw_no_1_lat, rw_no_1_lon from xp_rw t1 where t1.icao= 'LS11' order by rw_length_mt desc limit 1
;
select mx_get_point_based_on_bearing_and_length_in_meters (t1.rw_no_1_lat, rw_no_1_lon, 90.0, t1.rw_width * 0.5) as start_pos,t1.rw_no_1_lat, rw_no_1_lon, t1.rw_no_1 as name, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as heading, t1.rw_length_mt from xp_rw t1 where t1.icao= 'LS11' order by rw_length_mt desc limit 1
;
-- 32.15683982,-91.84336185


select mx_get_point_based_on_bearing_and_length_in_meters (t1.rw_no_1_lat, rw_no_1_lon, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) + 90.0, t1.rw_width * 0.5 ) as start_pos, t1.rw_no_1 as name, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as heading, t1.rw_length_mt from xp_rw t1 where t1.icao= 'YPAM' order by rw_length_mt desc limit 1
-- -18.75106663,146.57820144
;

select * from xp_rw t1 
where t1.icao= 'YPAM' order by rw_length_mt desc limit 1;

select mx_get_point_based_on_bearing_and_length_in_meters (t1.rw_no_1_lat, rw_no_1_lon, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) + 90.0, t1.rw_width) as start_pos
     , t1.rw_no_1 as name, mx_bearing(t1.rw_no_1_lat, rw_no_1_lon, rw_no_2_lat, rw_no_2_lon) as heading, t1.rw_length_mt 
from xp_rw t1 where t1.icao= 'YPAM' order by rw_length_mt desc limit 1
;
---18.75111538,146.57804795


select * from ramps_vu v1
where v1.icao = 'LLBG'
;

-- find oil rigs
select t1.icao_id, t1.icao, t1.name, t1.lat, t1.lon, t1.length, t1.width, t2.key_col, t2.val_col
from xp_helipads t1, xp_ap_metadata t2
where 1 = 1
and t1.icao_id = t2.icao_id
and t2.key_col = 'is_oilrig'
and t2.val_col = '1'
;

select * from xp_helipads t1
where t1.icao = '4LA1'
;

select t1.icao_id, t1.icao, count(t1.icao_id) as is_oilrig 
from xp_helipads t1,  xp_ap_metadata t2 
where t1.icao_id = t2.icao_id 
and t2.key_col = 'is_oilrig' 
and t2.val_col = '1'  
group by t1.icao_id, t1.icao
;

select t1.icao_id, t1.icao, count(t1.icao_id) as is_oilrig  from xp_helipads t1,  xp_ap_metadata t2 where t1.icao_id = t2.icao_id  and t2.key_col = 'is_oilrig' and t2.val_col = '1'  group by t1.icao_id, t1.icao
;

select *
from airports_vu v1
where v1.is_oilrig = 1
;

select t1.icao_id, t1.icao, t1.val_col, t2.ap_name
from xp_ap_metadata t1, xp_airports t2
where key_col = 'icao_code'
and t1.icao != val_col
and t1.icao_id = t2.icao_id
;


with missing_icao_vu as (
select t1.icao_id, t1.icao, t1.val_col
from xp_ap_metadata t1
where t1.key_col = 'icao_code'
and t1.icao != val_col
)
select *
from missing_icao_vu
;


--
--
select * from xp_ap_metadata;

select t2.val_col, t1.*
from xp_airports t1, xp_ap_metadata t2
where  t2.key_col = 'icao_code'
and t1.icao_id = t2.icao_id
and t2.val_col != t1.icao
;


-----
-----
select * from xp_ap_metadata;

-----
-----
;


-- begin transaction;
-- 
-- update xp_airports
-- set icao = (
--                 select t1.val_col
--                 from xp_ap_metadata t1
--                 where t1.key_col = 'icao_code'
--                 and t1.icao != t1.val_col
--                 and t1.icao_id = xp_airports.icao_id
--           )
-- where 1 = 1
-- and xp_airports.icao != (select val_col
--                          from  xp_ap_metadata t2
--                          where t2.key_col = 'icao_code'
--                          and t2.icao != t2.val_col
--                          and t2.icao_id = xp_airports.icao_id
--                          )           
-- ;


select t2.val_col, t1.*
from xp_airports t1, xp_ap_metadata t2
where  t2.key_col = 'icao_code'
and t1.icao_id = t2.icao_id
and t2.val_col != t1.icao
;

rollback;

select *
from xp_airports t1
where lower(t1.ap_name) like 'akuta%'
;

-- closest oil rig
select distance, icao_id, icao, ap_name, ap_lat, ap_lon
from  (
select mx_calc_distance(-19.25377419, 146.7707937, v1.ap_lat , v1.ap_lon, 3440) as distance, v1.icao_id, v1.icao, v1.ap_name, v1.ap_lat, v1.ap_lon
from airports_vu v1
where v1.is_oilrig > 0
)
where 1 = 1
ORDER BY RANDOM() limit 1
;


select 'Airport: "' || v2.icao || '" is located: ' ||  mx_calc_distance(v2.ap_lat, v2.ap_lon, v1.ap_lat , v1.ap_lon, 3440) || 'nm from: "' || v1.icao || '"' as desc_detail, mx_calc_distance(v2.ap_lat, v2.ap_lon, v1.ap_lat , v1.ap_lon, 3440) as distance, v2.icao_id, v2.icao, v2.ap_name, v2.ap_lat, v2.ap_lon
      , v1.icao_id, v1.icao, v1.ap_name, v1.ap_lat, v1.ap_lon
from airports_vu v1, airports_vu v2
where 1 = 1 
and (v2.is_oilrig = 0 and v1.is_oilrig > 0 and v1.icao_id != v2.icao_id)
and mx_calc_distance(v2.ap_lat, v2.ap_lon, v1.ap_lat , v1.ap_lon, 3440) between 20.0 and 80.0
order by v1.icao_id, distance
limit 10
;

-- <point lat="47.4363788" long="-122.30398304" elev_ft="358.2503568" />
-- lat="61.391811371" long="60.885505676"
select distance, icao_id, icao, ap_name, ap_lat, ap_lon
from (
select mx_calc_distance(v2.ap_lat, v2.ap_lon, 55.530833, 5.004167, 3440) as distance, v2.icao_id, v2.icao, v2.ap_name, v2.ap_lat, v2.ap_lon      
from airports_vu oilRigVu, airports_vu v2
where 1 = 1 
and (v2.is_oilrig = 0 and (v2.ramp_helos + v2.helipads > 0) and oilRigVu.is_oilrig > 0 and oilRigVu.icao_id != v2.icao_id)
and mx_calc_distance(v2.ap_lat, v2.ap_lon, 55.530833, 5.004167, 3440) between 5.0 and 120.0
order by oilRigVu.icao_id, distance
limit 30
)
--order by random() limit 1 
;

select * from
(
select  mx_calc_distance(29.260498047, -89.354774475, t1.ap_lat, t1.ap_lon, 3440) as distance, t1.*
from xp_airports t1
)
order by distance
;





select icao_id, icao, ap_elev_ft, ap_name, ap_type, ap_lat, ap_lon 
     , mx_calc_distance ( ap_lat, ap_lon,61.391609191895, 5.760071277618, 3440) as dist_nm, 0 as bearing , helipads, ramp_helos, ramp_planes, ramp_props
     , ramp_turboprops, ramp_jet_heavy, rw_hard, rw_dirt_gravel, rw_grass , rw_water, is_custom 
from airports_vu 
where 1 = 1 
and icao = 'ENBL' 
order by dist_nm
;

-- missionx: No START airport was found for Oil Rig: EKHA ([H] Halfdan A Helideck), location: 55.530833, 5.004167
select distance, icao_id, icao, ap_name, ap_lat, ap_lon, oilrig_icao, oilrig_icao_id
from (
select mx_calc_distance(v2.ap_lat, v2.ap_lon, 55.530833, 5.004167, 3440) as distance, v2.icao_id, v2.icao, v2.ap_name, v2.ap_lat, v2.ap_lon, oilRigVu.icao as oilrig_icao, oilRigVu.icao_id as oilrig_icao_id    
from airports_vu oilRigVu, airports_vu v2
where 1 = 1 
and oilRigVu.icao_id = 83 -- = '02CA'
and (v2.is_oilrig = 0 and oilRigVu.is_oilrig > 0 and oilRigVu.icao_id != v2.icao_id)
and mx_calc_distance(v2.ap_lat, v2.ap_lon, 55.530833, 5.004167, 3440) between 5.0 and 120.0
order by oilRigVu.icao_id, distance
limit 30
)
-- where distance between 5 and 120
--order by random() limit 1 
;

 -- filter oilrigs with airports near them
 
select distance, icao_id, icao, ap_name, ap_lat, ap_lon
from  (
select mx_calc_distance(-19.25377419, 146.7707937, oilRigVu.ap_lat , oilRigVu.ap_lon, 3440) as distance, oilRigVu.icao_id, oilRigVu.icao, oilRigVu.ap_name, oilRigVu.ap_lat, oilRigVu.ap_lon
from airports_vu oilRigVu
where oilRigVu.is_oilrig > 0
)
where 1 = 1
--ORDER BY RANDOM() limit 1
;


with oilrigs_vu as (
select oilRigVu.icao_id, oilRigVu.icao, oilRigVu.ap_name, oilRigVu.ap_lat, oilRigVu.ap_lon, trunc(oilRigVu.ap_lat) as ap_lat_trunc, trunc(oilRigVu.ap_lon) as ap_lon_trunc
from airports_vu oilRigVu
where oilRigVu.is_oilrig > 0
), 
airports_with_helipads_vu as (
select av.icao_id, av.icao, av.ap_name, av.ap_lat, av.ap_lon, trunc(av.ap_lat) as ap_lat_trunc, trunc(av.ap_lon) as ap_lon_trunc
from airports_vu av
where av.is_oilrig = 0
-- and av.ramp_helos + av.helipads > 0
)
select  mx_calc_distance(ov.ap_lat, ov.ap_lon, awh.ap_lat , awh.ap_lon, 3440) as distance
        , ov.icao_id as oilrig_icao_id, ov.icao as oilrig_icao, ov.ap_lat as oilrig_lat, ov.ap_lon as oilrig_lon, ov.ap_name as oilrig_name
        , awh.icao_id, awh.icao, awh.ap_lat, awh.ap_lon, awh.ap_name
        , trunc(ov.ap_lat), trunc(ov.ap_lon)
from oilrigs_vu ov, airports_with_helipads_vu awh
where 1 = 1
and ov.icao_id != awh.icao_id
and ( awh.ap_lat_trunc between ov.ap_lat_trunc - 2 and ov.ap_lat_trunc + 2
      and awh.ap_lon_trunc between ov.ap_lon_trunc - 2 and ov.ap_lon_trunc + 2 )
--and mx_calc_distance(ov.ap_lat, ov.ap_lon, awh.ap_lat , awh.ap_lon, 3440) between 5.0 and 120.0
--and ov.icao='EKSI'
;



with oilrigs_vu as (
select oilRigVu.icao_id, oilRigVu.icao, oilRigVu.ap_name, oilRigVu.ap_lat, oilRigVu.ap_lon, trunc(oilRigVu.ap_lat) as ap_lat_trunc, trunc(oilRigVu.ap_lon) as ap_lon_trunc
from airports_vu oilRigVu
where oilRigVu.is_oilrig > 0
), 
airports_with_helipads_vu as (
select av.icao_id, av.icao, av.ap_name, av.ap_lat, av.ap_lon, trunc(av.ap_lat) as ap_lat_trunc, trunc(av.ap_lon) as ap_lon_trunc
from airports_vu av
where av.is_oilrig = 0
and av.ramp_helos + av.helipads > 0
)
select  mx_calc_distance(ov.ap_lat, ov.ap_lon, awh.ap_lat , awh.ap_lon, 3440) as distance
        , ov.icao_id as oilrig_icao_id, ov.icao as oilrig_icao, ov.ap_name as oilrig_name, ov.ap_lat as oilrig_lat, ov.ap_lon as oilrig_lon
        , awh.icao_id as start_icao_id, awh.icao as start_icao, awh.ap_name as start_ap_name, awh.ap_lat as start_lat, awh.ap_lon as start_lon
from oilrigs_vu ov, airports_with_helipads_vu awh
where 1 = 1
and ov.icao_id != awh.icao_id
and ( awh.ap_lat_trunc between ov.ap_lat_trunc - 2 and ov.ap_lat_trunc + 2
      and awh.ap_lon_trunc between ov.ap_lon_trunc - 2 and ov.ap_lon_trunc + 2 )
order by RANDOM() limit 1
;



select * from ramps_vu where 1 = 1 and icao_id = 36787 
;
select * from ramps_vu where 1 = 1 and icao_id = 36062 
;

update stats
set activity = NULL
where activity == "";

select * from stats;
-- search and order all landing activities
select stats.line_id, stats.activity, stats.faxil_gear, stats.gforce_axil, stats.gforce_normal
     , stats.Q, stats.Qrad, stats.AoA, stats.pitch, stats.vh_ind_fpm, stats.onground_any, stats.onground_all, stats.agl, stats.airspeed, stats.groundspeed, stats.vh_ind
from stats 
where 1 = 1 
--and activity is not null
order by line_id
;


CREATE TABLE t0(x INTEGER PRIMARY KEY, y TEXT);
INSERT INTO t0 VALUES (1, 'aaa'), (2, 'ccc'), (3, 'bbb');

SELECT x, y, row_number() OVER (ORDER BY y) AS row_number FROM t0 ORDER BY x;



select *
from stats
order by stats.line_id --stats.local_time_sec
;
-- provide last ~30 sec from last "landing" activity
with last_landing_activity as (
    select max(stats.line_id) as last_landing_line_id
    from stats
    where stats.activity = 'landing'   
)
select t1.*
from stats t1, last_landing_activity v1
where t1.line_id between v1.last_landing_line_id - 30 and v1.last_landing_line_id + 10
;


-- Flare stats: at the landing stage
with last_landing_activity as (
    select max(stats.line_id) as last_landing_line_id
    from stats
    where stats.activity = 'landing'   
)
select land_stats.aoa, abs(land_stats.Q) as pitch_rate, abs(land_stats.Qrad), land_stats.Q as orig_pitch_rate_val, land_stats.Qrad, land_stats.faxil_gear, land_stats.gforce_axil, land_stats.gforce_normal
from 
(
    select t1.*, v1.last_landing_line_id, (select stats.vh_ind_fpm from stats where stats.line_id = v1.last_landing_line_id ) as vv_touchdown_fpm
    from stats t1, last_landing_activity v1
--    where t1.line_id between v1.last_landing_line_id - 2 and v1.last_landing_line_id -- ~6 sec
    where t1.line_id = v1.last_landing_line_id -- at landing
) land_stats
;

-- stats from last ~30 sec from last "landing" activity
with last_landing_activity as (
    select max(stats.line_id) as last_landing_line_id, stats.vh_ind_fpm as landing_vh_ind_fpm
    from stats
    where stats.activity = 'landing'   
)
select avg(land_stats.vh_ind_fpm) as average_fpm, landing_vh_ind_fpm, avg(land_stats.vh_ind_fpm)/60 as average_vertical_velocity_per_second
      , landing_vh_ind_fpm/60 as vv_touchdown_fps, sum(delta_agl)
      , avg(land_stats.agl), land_stats.agl - (avg(land_stats.agl)/2)       
from 
(
    select t1.*, v1.last_landing_line_id, v1.landing_vh_ind_fpm, ifnull( lag( t1.agl, 1) over (order by t1.line_id), agl) - agl,0 as delta_agl
    from stats t1, last_landing_activity v1
    where t1.line_id between v1.last_landing_line_id - 30 and v1.last_landing_line_id
    -- and trunc(t1.agl) <= 15
) land_stats
;


-- Find average agl in the last 30 seconds
with last_landing_activity as (
    select line_id as landing_line_id, vh_ind_fpm as landing_vh_ind_fpm, flap_ratio, lat, lon, faxil_gear, onground_any, onground_all, gforce_normal, Q, Qrad
    from stats
    where stats.line_id = 
                        ( select max(stats.line_id)
                          from stats
                          where stats.activity = 'landing' )
)
select avg(delta_agl) avg_delta_agl, avg(agl) avg_agl, avg( vh_ind_fpm) as avg_vh_ind_fpm     
from 
(
    select agl, ifnull(lag(agl,1) over (order by t1.line_id) - agl, 0.0) as delta_agl
         , vh_ind_fpm 
    from stats t1, last_landing_activity v1
    where 1 = 1
    and t1.line_id between v1.landing_line_id - 30 and v1.landing_line_id
    -- and trunc(t1.agl) <= 15
) land_stats
--where delta_agl > 0.0
;
    

with last_landing_activity as (
    select line_id as landing_line_id, vh_ind_fpm as landing_vh_ind_fpm, flap_ratio, lat, lon, faxil_gear, onground_any, onground_all, gforce_normal, Q, Qrad
    from stats
    where stats.line_id = 
                        ( select max(stats.line_id)
                          from stats
                          where stats.activity = 'landing' )
),
main_window as (
select t1.line_id
        , agl, ifnull(lag(agl,1) over (order by t1.line_id) - agl, 0.0) as delta_agl
        , vh_ind_fpm, ifnull(lag(vh_ind_fpm,1) over (order by t1.line_id) - vh_ind_fpm, 0.0) as delta_vh_ind_fpm
    from stats t1, last_landing_activity v1
    where 1 = 1
    and t1.line_id between v1.landing_line_id - 35 and v1.landing_line_id
)
select avg(delta_agl) avg_delta_agl, avg(agl) avg_agl, avg( vh_ind_fpm) as avg_vh_ind_fpm, avg(delta_vh_ind_fpm) as avg_delta_vh_ind_fpm     
from 
(
    select t1.*, v1.*
    from main_window t1, last_landing_activity v1
    where 1 = 1
    and t1.line_id between v1.landing_line_id - 30 and v1.landing_line_id
    -- and trunc(t1.agl) <= 15
) land_stats
--where delta_agl > 0.0
;



-- Using ROWS windowing technique
with last_landing_activity as (
    select line_id as landing_line_id, vh_ind_fpm as landing_vh_ind_fpm, flap_ratio, lat, lon, faxil_gear, onground_any, onground_all, gforce_normal, Q, Qrad
    from stats
    where stats.activity = 'landing'
    order by line_id
    limit 1
),
main_window as (
select t1.line_id
        , agl, ifnull(lag(agl,1) over (order by t1.line_id) - agl, 0.0) as delta_agl
        , vh_ind_fpm, ifnull(lag(vh_ind_fpm,1) over (order by t1.line_id) - vh_ind_fpm, 0.0) as delta_vh_ind_fpm
    from stats t1, last_landing_activity v1
    where 1 = 1
    and t1.line_id between v1.landing_line_id - 35 and v1.landing_line_id
)
select avg(delta_agl) avg_delta_agl, avg(agl) avg_agl, avg( vh_ind_fpm) as avg_vh_ind_fpm, avg(delta_vh_ind_fpm) as avg_delta_vh_ind_fpm     
from 
(
    select t1.*, v1.*
    from main_window t1, last_landing_activity v1
    where 1 = 1
    and t1.line_id between v1.landing_line_id - 30 and v1.landing_line_id
    -- and trunc(t1.agl) <= 15
) land_stats
--where delta_agl > 0.0
;




SELECT line_id, vvi_fpm_pilot, flap_ratio, local_date_days, local_time_sec, lat, lon
       , elev, lag(elev,1) over (order by line_id) - elev as delta_elev, activity
       , vh_ind, faxil_gear, onground_any, onground_all, vh_ind_fpm, gforce_normal
       , agl, ifnull(lag(agl,1) over (order by stats.line_id) - agl, 0.0) as delta_agl
       , Qrad, Q       --,gforce_axil, AoA, pitch, roll, heading_mag, heading_no_mag
       -- , airspeed, groundspeed, brakes_L, brakes_R
  FROM stats
;


select elev, flap_ratio * 100 as flap_ratio, local_date_days, local_time_sec, case when airspeed < 0.0 then 0.0 else airspeed end as airspeed , groundspeed, faxil_gear, roll, IFNULL(activity,'') from stats order by line_id

;

select delta_nm, sum(delta_nm) over (order by 1)
from (
select mx_calc_distance(stats.lat,stats.lon, IFNULL(lag(stats.lat, 1) over (order by stats.line_id), stats.lat) , IFNULL(lag(stats.lon, 1) over (order by stats.line_id), stats.lon), 3440) as delta_nm
from stats
)
;



select avg(stats.agl), avg(stats.gforce_normal)
from stats
where stats.line_id between 321-15 and 321;



select t1.icao
from xp_airports t1
where t1.icao_id = xp_ap_metadata.icao_id
and t1.icao != xp_ap_metadata.icao
;                


begin transaction;

update xp_ap_metadata
set icao = (
            select t1.icao
            from xp_airports t1
            where t1.icao_id = xp_ap_metadata.icao_id
            and t1.icao != xp_ap_metadata.icao
          )
where  xp_ap_metadata.icao not in (select t1.icao
                                   from  xp_airports t1        
                                   where t1.icao_id = xp_ap_metadata.icao_id)  
;    


-- select ifnull(t1.icao, t2.icao)
select distinct ifnull(t1.icao, 'n/a') as icao
from xp_airports t1, xp_ap_metadata t2
where t1.icao_id = t2.icao_id
and t1.icao != t2.icao
;

update xp_ap_metadata
set icao = (
            select ifnull(t1.icao, xp_ap_metadata.icao)
            from xp_airports t1
            where t1.icao_id = xp_ap_metadata.icao_id
            and t1.icao != xp_ap_metadata.icao
          )
where  xp_ap_metadata.icao_id in (          
select distinct icao_id
from   xp_ap_metadata
where  xp_ap_metadata.icao not in (select t1.icao
                                   from  xp_airports t1        
                                   where t1.icao_id = xp_ap_metadata.icao_id)  
)                                   
;            





select *
from   xp_ap_metadata t1
where t1.icao is NULL
;

end transaction;       




select *
from xp_ap_metadata 
where 1 = 1 
and xp_ap_metadata.icao is NULL
--and icao_id = 30037
;

select *
from xp_airports t1
where t1.icao_id = 30037
;

--update xp_ap_metadata
--set icao = 'n/a'
--where icao_id = 30037;  -- Faro, LPFR

select *
from  xp_ap_metadata
where icao = 'LPFR'
;



select icao_id
from xp_ap_metadata
EXCEPT
select icao_id
from xp_airports
;

begin transaction;

delete from xp_ap_metadata
where icao_id in (
select icao_id
from xp_ap_metadata
EXCEPT
select icao_id
from xp_airports
)
;

end transaction; 



select *
from xp_loc t1
where t1.max_reception_range=0;

select loc_type, count(loc_type)
from xp_loc t1
group by t1.loc_type
order by 1
;

select *
from xp_loc t1
where t1.loc_type in ( 'DME', 'VOR', 'NDB' )
-- and bias != 0
order by loc_type
;

select *
from (
select FORMAT ("%-*s %-*s %i (%s) ", 6,t3.ident,15,t3.name, t3.frq_mhz, t3.loc_type ) as loc_data
,  mx_calc_distance(t3.lat, t3.lon, 47.449888889,  -122.311777778, 3440) as distance
, t3.loc_type
from xp_loc t3
) v1
where distance <= 20
and v1.loc_type in ('DME', 'VOR', 'NDB' )
order by v1.distance
;

select *
from xp_airports
where icao = 'KSEA'
;

select *
from xp_loc
where icao = 'UHBB'
;


-- missionx (245): PLANE: <point lat="-19.25373583" long="146.77101810" elev_ft="14.03" 	template=""  loc_desc="" type=""   heading_psi="287.00" groundElev="14.0313" ft replace_lat="-19.25373583" replace_long="146.77101810" />

select icao, distance_nm, loc_rw, loc_type, frq_mhz, loc_bearing, rw_length_mt, rw_width, ap_elev,ap_elev_ft, ap_name, surf_type_text, bearing_from_to_icao from(
  select xp_loc.icao, round(3440 * acos(cos(radians(-19.25373583)) * cos(radians(xp_loc.lat)) * cos(radians(146.77101810) - radians(xp_loc.lon)) + sin(radians(-19.25373583)) * sin(radians(xp_loc.lat)))) as distance_nm
        , xp_loc.loc_rw, xp_loc.loc_type, xp_loc.frq_mhz, xp_loc.loc_bearing, xp_rw.rw_length_mt, xp_rw.rw_width, xa.ap_elev, xp_loc.ap_elev_ft, xa.ap_name
        , case xp_rw.rw_surf when 1 then 'Asphalt' when 2 then 'Concrete' when 3 then 'Turf or grass' when 4 then 'Dirt' when 5 then 'Gravel' when 12 then 'Dry lakebed' when 13 then 'Water runways' when 14 then 'Snow or ice' when 15 then 'Transparent' else 'other' end as surf_type_text
        , mx_bearing(-19.25373583, 146.77101810, xp_loc.lat, xp_loc.lon) as bearing_from_to_icao
  from xp_loc, xp_rw, xp_airports xa
  where xp_rw.icao = xp_loc.icao
  and (xp_rw.rw_no_1 = xp_loc.loc_rw or xp_rw.rw_no_2 = xp_loc.loc_rw)
  and xa.icao = xp_rw.icao
  )
where 1 = 1
 and distance_nm between 3412 and 4550 and rw_length_mt >= 1000 and rw_width >= 45 and ap_elev >= 0 and lower(loc_type) in ( 'gls','lp','lpv' )
 ;
 
-- display duplicate airports ICAO
select *
from xp_airports
where icao in (
    select icao
    from xp_airports
    group by icao
    having count(icao) > 1
)
;

    SELECT *
    FROM (
      SELECT icao_id, icao,
        LAG(icao_id, 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao) pos_in_group,
        mx_calc_distance(t1.ap_lat, t1.ap_lon, LAG(t1.ap_lat, 1, t1.ap_lat) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id), LAG(t1.ap_lon, 1, t1.ap_lon) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id), 3440) AS distance,
        trunc(t1.ap_lat) - LAG(trunc(t1.ap_lat), 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id) AS lat_diff,
        trunc(t1.ap_lon) - LAG(trunc(t1.ap_lon), 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id) AS lon_diff,
        t1.*
      FROM xp_airports t1
      WHERE t1.icao IN (
        SELECT icao
        FROM xp_airports t3
        GROUP BY icao
        HAVING count(icao) > 1
        )
      ORDER BY icao,
      icao_id
    ) v1
WHERE v1.pos_in_group != 0    
AND (v1.distance < 2.4 OR (lat_diff = 0 AND lon_diff = 0) ) 
;


begin transaction;



DELETE FROM xp_airports
  WHERE icao_id IN (
    SELECT icao_id
    FROM (
      SELECT icao_id, icao,
        LAG(icao_id, 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao) pos_in_group,
        mx_calc_distance(t1.ap_lat, t1.ap_lon, LAG(t1.ap_lat, 1, t1.ap_lat) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id), LAG(t1.ap_lon, 1, t1.ap_lon) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id), 3440) AS distance,
        trunc(t1.ap_lat) - LAG(trunc(t1.ap_lat), 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id) AS lat_diff,
        trunc(t1.ap_lon) - LAG(trunc(t1.ap_lon), 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id) AS lon_diff,
        t1.*
      FROM xp_airports t1
      WHERE t1.icao IN (
        SELECT icao
        FROM xp_airports t3
        GROUP BY icao
        HAVING count(icao) > 1
        )
      ORDER BY icao,
      icao_id
    ) v1
WHERE v1.pos_in_group != 0    
AND (v1.distance < 2.4 OR (lat_diff = 0 AND lon_diff = 0) ) 
)  
;

rollback;

end transaction;
  


select t1.icao_id, FORMAT("%6.2f", mx_calc_distance(t1.ap_lat, t1.ap_lon, 27.687233,  86.731914, 3440)) as format_distance, FORMAT("Airport: %s (%s), lat/lon: %3.8f/%3.8f (%ift)", t1.ap_name, t1.icao, ap_lat, ap_lon, ap_elev, ap_name) as data, mx_calc_distance(t1.ap_lat, t1.ap_lon, 27.687233,  86.731914, 3440) as distance
from xp_airports t1
where 1 = 1
and icao = 'VNLK'
order by distance
;



select t1.*
from xp_airports t1
;

select t2.rw_no_1 || '/' || t2.rw_no_2 as rw_key, FORMAT ("Runway: %-*s Length: %d meters", 10, t2.rw_no_1 || '/' || t2.rw_no_2, t2.rw_length_mt) as rw_data, mx_calc_distance(t2.rw_no_1_lat, t2.rw_no_1_lon, 27.687233,  86.731914, 3440) as distance
from xp_rw t2
where 1 = 1
and icao_id = 27453
order by distance
;

-- display duplicate airports ICAO
select *
from xp_airports
where icao in (
    select icao
    from xp_airports
    group by icao
    having count(icao) > 1
)
;

select *
from xp_loc t3
;

select *
from (
select FORMAT ("Localizer rw: %-*s ,freq: %i, Type: %s Region: %-*s, lat/lon: %3.0f/%3.0f ", 4, t3.loc_rw, t3.frq_mhz, t3.loc_type, 3, icao_region_code, t3.lat, t3.lon ) as loc_data, mx_calc_distance(t3.lat, t3.lon, 27.687233,  86.731914, 3440) as distance, t3.loc_rw
from xp_loc t3
where icao = 'KSEA'
) v1
order by v1.distance, v1.loc_rw
;





-- 47.6201008,-122.3491711|47.6208995,-122.3491668|47.6209024,-122.3503686
-- 47.439185601259751, -122.31519271112450
select icao, ap_name, ap_lat, ap_lon, ap_elev 
      , mx_bearing( 47.439185601259751, -122.31519271112450, ap_lat, ap_lon ) as heading
      , mx_is_plane_in_airport_boundary( 47.439185601259751, -122.31519271112450, boundary ) as is_plane_in_boundary
      , icao_id, boundary
from 
(
  select xp_airports.icao_id, xp_airports.icao, xp_airports.ap_name, xp_airports.ap_lat, xp_airports.ap_lon, xp_airports.ap_elev, xp_airports.boundary
  from xp_airports
  where xp_airports.boundary is not null
  and ( trunc(xp_airports.ap_lat) between trunc( 47.43918560125 - 1.0) and  trunc( 47.43918560125 + 1.0) )
  and ( trunc(xp_airports.ap_lon) between trunc( -122.31519271 - 1.0) and  trunc( -122.31519271 + 1.0) )
) v1
where is_plane_in_boundary = 1
;

select *
from xp_airports
where 1 =1 
and icao like 'CYYJ'
--and lower(ap_name) like '%lagua%'
;



select frq_data, frq
from (
select FORMAT ("%-*.3f %s", 10, xaf.frq/1000.0, xaf.frq_desc ) as frq_data
, frq
from xp_ap_frq xaf
where icao like 'CYYJ'
) v1
--order by v1.distance
;
select FORMAT ("%-*.3f %s", 10, xaf.frq/1000.0, xaf.frq_desc ) as frq_data
, frq
from xp_ap_frq xaf
where icao_id = 13738
;

select ident, loc_data, distance, loc_type
from (
select t3.ident
, FORMAT ("%.2f (%i/%s) %s", case WHEN length(t3.frq_mhz) = 5 THEN t3.frq_mhz/100.0 ELSE t3.frq_mhz END 
, mx_bearing(48.647163889, -123.425741667, t3.lat, t3.lon ), t3.loc_type, t3.name ) as loc_data
, mx_calc_distance(t3.lat, t3.lon, 48.647163889, -123.425741667, 3440) as distance
, t3.loc_type
, t3.frq_mhz
from xp_loc t3
) v1
where distance <= 20
and v1.loc_type in ('DME', 'VOR', 'NDB' )
order by v1.distance
;

select ident, loc_data, distance, loc_type
from (
select t3.ident
, FORMAT ("%.2f (%i/%s) %s", case WHEN length(t3.frq_mhz) = 5 THEN t3.frq_mhz/100.0 ELSE t3.frq_mhz END 
                           , mx_bearing(48.647163889, -123.425741667, t3.lat, t3.lon )
                           , t3.loc_type
                           , t3.name ) as loc_data
, mx_calc_distance(t3.lat, t3.lon, 48.647163889, -123.425741667, 3440) as distance
, t3.loc_type
, t3.frq_mhz
from xp_loc t3
) v1
where distance <= 20
and v1.loc_type in ('DME', 'VOR', 'NDB' )
order by v1.distance
;


-- base
select t3.ident
     , case WHEN length(t3.frq_mhz) = 5 THEN t3.frq_mhz/100.0 ELSE t3.frq_mhz END as formatedFreq
     , mx_bearing(48.647163889, -123.425741667, t3.lat, t3.lon ) as bearing_nav_to_ap
     , t3.loc_type
     , t3.name
     , mx_calc_distance(t3.lat, t3.lon, 48.647163889, -123.425741667, 3440) as distance
     , t3.frq_mhz
from xp_loc t3   
where distance <= 20  
order by ident, frq_mhz, name
;


with vor_dme as
(
select v1.ident, v1.name, v1.frq_mhz, 'VOR/DME' as vor_dme_nav, v1.lat, v1.lon
from 
(
select t3.ident
     , case WHEN length(t3.frq_mhz) = 5 THEN t3.frq_mhz/100.0 ELSE t3.frq_mhz END as formatedFreq
     , mx_bearing(48.647163889, -123.425741667, t3.lat, t3.lon ) as bearing_nav_to_ap
     , t3.loc_type
     , lag (t3.loc_type) over (order by ident, frq_mhz, loc_type) as prev_type 
     , t3.name
     , mx_calc_distance(t3.lat, t3.lon, 48.647163889, -123.425741667, 3440) as distance
     , t3.frq_mhz
     , t3.ident || ',' || t3.name || ',' || t3.frq_mhz as identNameFrq
     , lag (t3.ident || ',' || t3.name || ',' || t3.frq_mhz) over (order by ident, frq_mhz, loc_type) as prevIdentNameFrq
     , t3.lat
     , t3.lon
from xp_loc t3   
where distance <= 20  
order by ident, frq_mhz, name
) v1
where 1 = 1
--and v1.loc_type in ('DME', 'VOR')
--and (v1.loc_type in ('DME', 'VOR') or v1.prev_type in ('DME', 'VOR') )
and v1.identNameFrq = v1.prevIdentNameFrq
and ( (v1.loc_type = 'VOR' and v1.prev_type='DME') or (v1.loc_type = 'DME' and v1.prev_type='VOR') )

)
select t1.ident
     , case WHEN length(t1.frq_mhz) = 5 THEN t1.frq_mhz/100.0 ELSE t1.frq_mhz END as formatedFreq
     , mx_bearing(48.647163889, -123.425741667, t1.lat, t1.lon ) as bearing_nav_to_ap
     , t1.loc_type
     , t1.name
     , mx_calc_distance(t1.lat, t1.lon, 48.647163889, -123.425741667, 3440) as distance
     , t1.frq_mhz
     , v1.ident as vor_dme_nav
     , ( t1.ident || t1.name || t1.frq_mhz) as uqLoc
     , ( v1.ident || v1.name || v1.frq_mhz) as uqVodDme
from xp_loc t1, vor_dme v1  
where distance <= 20  
and ( IFNULL(( t1.ident || t1.name || t1.frq_mhz), t1.ident) != ( v1.ident || v1.name || v1.frq_mhz) )
order by t1.ident, t1.frq_mhz, t1.name
;



select t3.ident
, FORMAT ("%.2f (%i/%s) %s", case WHEN length(t3.frq_mhz) = 5 THEN t3.frq_mhz/100.0 ELSE t3.frq_mhz END 
                           , mx_bearing({}, {}, t3.lat, t3.lon )
                           , t3.loc_type, t3.name ) as loc_data
, mx_calc_distance(t3.lat, t3.lon, {}, {}, 3440) as distance
, t3.loc_type
, t3.frq_mhz
from xp_loc t3
;

-- Optimize Query
WITH vor_dme AS (
  SELECT v1.ident, v1.loc_data, distance, 'VOR/DME' AS loc_type, v1.frq_mhz, identNameFrq, prevIdentNameFrq
    FROM (
           SELECT t3.ident,
                  FORMAT("%.2f (%i,%s) %s", CASE WHEN length(t3.frq_mhz) = 5 THEN t3.frq_mhz / 100.0 ELSE t3.frq_mhz END
                                          , mx_bearing(47.449888889, -122.311777778, t3.lat, t3.lon)
                                          , 'VOR/DME'/* constant */
                                          , t3.name) AS loc_data,
                  mx_calc_distance(t3.lat, t3.lon, 47.449888889, -122.311777778, 3440) AS distance,
                  t3.loc_type,
                  t3.frq_mhz,
                  IFNULL( (t3.ident || t3.name || t3.frq_mhz), t3.ident) AS identNameFrq,
                  lag(IFNULL( (t3.ident || t3.name || t3.frq_mhz), t3.ident)) OVER (ORDER BY t3.ident, t3.frq_mhz, t3.loc_type) AS prevIdentNameFrq,
                  lag(t3.loc_type) OVER (ORDER BY ident, frq_mhz, loc_type) AS prev_type
             FROM xp_loc t3
            WHERE distance <= 20.0
            and t3.loc_type in ('VOR', 'DME')
            ORDER BY ident, frq_mhz, name
         )
         v1
   WHERE 1 = 1  
     AND v1.identNameFrq = v1.prevIdentNameFrq  
     AND ( (v1.loc_type = 'VOR' AND v1.prev_type = 'DME') OR 
           (v1.loc_type = 'DME' AND v1.prev_type = 'VOR') ) 
)
SELECT v1.ident, v1.loc_data, v1.distance
  FROM vor_dme v1
UNION ALL
SELECT *
FROM (
  
SELECT t1.ident
       , FORMAT("%.2f (%i,%s) %s", CASE WHEN length(t1.frq_mhz) = 5 THEN t1.frq_mhz / 100.0 ELSE t1.frq_mhz END, mx_bearing(47.449888889, -122.311777778, t1.lat, t1.lon), t1.loc_type, t1.name) AS loc_data
       , mx_calc_distance(t1.lat, t1.lon, 47.449888889, -122.311777778, 3440) as distance
  FROM xp_loc t1
 WHERE mx_calc_distance (t1.lat, t1.lon, 47.449888889, -122.311777778, 3440) <= 20.0
   AND t1.loc_type IN ('VOR', 'DME')
   AND IFNULL( (t1.ident || t1.name || t1.frq_mhz), t1.ident) not in (select identNameFrq 
                                                                        from vor_dme)
UNION ALL
SELECT *
  FROM (
         SELECT t1.ident,
                FORMAT("%i (%i,%s) %s", t1.frq_mhz
                                      , mx_bearing(47.449888889, -122.311777778, t1.lat, t1.lon)
                                      , t1.loc_type
                                      , t1.name) AS loc_data
                , mx_calc_distance(t1.lat, t1.lon, 47.449888889, -122.311777778, 3440) as distance
           FROM xp_loc t1
          WHERE mx_calc_distance(t1.lat, t1.lon, 47.449888889, -122.311777778, 3440) <= 20.0  
            AND t1.loc_type = 'NDB'
        )
ORDER BY distance      
)
;



-- lat/lon:47.447502136,-122.307899475

select icao, round(distance_nm) as distance_nm, loc_rw, loc_type, frq_mhz, loc_bearing, rw_length_mt, rw_width, ap_elev, ap_name, surf_type_text, bearing_from_to_icao
from (
select xp_loc.icao
      , mx_calc_distance (47.447502, -122.3079, xp_loc.lat, xp_loc.lon, 3440) as distance_nm
      
      , xp_loc.loc_rw, xp_loc.loc_type, xp_loc.frq_mhz, xp_loc.loc_bearing, xp_rw.rw_length_mt, xp_rw.rw_width, xa.ap_elev, xa.ap_name
      , case xp_rw.rw_surf when 1 then 'Asphalt' when 2 then 'Concrete' when 3 then 'Turf or grass' when 4 then 'Dirt' when 5 then 'Gravel' when 12 then 'Dry lakebed' when 13 then 'Water runways' when 14 then 'Snow or ice' when 15 then 'Transparent' else 'other' end as surf_type_text
      , mx_bearing(47.447502, -122.3079, xp_loc.lat, xp_loc.lon) as bearing_from_to_icao
from xp_loc, xp_rw, xp_airports xa
where xp_rw.icao = xp_loc.icao
and (xp_rw.rw_no_1 = xp_loc.loc_rw or xp_rw.rw_no_2 = xp_loc.loc_rw)
and xa.icao = xp_rw.icao
)
where 1 = 1
  and rw_length_mt >= 1000 and rw_width >= 45 and ap_elev >= 0  and distance_nm between 50 and 250 LIMIT 250 
;

select icao, ap_name, ap_lat, ap_lon, ap_elev
      , mx_bearing( 47.4381635, -122.30388505, ap_lat, ap_lon ) as heading
      , mx_is_plane_in_airport_boundary( 47.4381635, -122.30388505, boundary ) as is_plane_in_boundary
      , icao_id, boundary
from
(
  select xp_airports.icao_id, xp_airports.icao, xp_airports.ap_name, xp_airports.ap_lat, xp_airports.ap_lon, xp_airports.ap_elev, xp_airports.boundary
  from xp_airports
  where xp_airports.boundary is not null
  and ( trunc(xp_airports.ap_lat) between trunc( 47.4381635 - 1.0) and  trunc( 47.4381635 + 1.0) )
  and ( trunc(xp_airports.ap_lon) between trunc( -122.30388505 - 1.0) and  trunc( -122.30388505 + 1.0) )
) v1
where is_plane_in_boundary = 1;


select icao, round(distance_nm) as distance_nm, loc_rw, loc_type, frq_mhz, loc_bearing, rw_length_mt, rw_width, ap_elev, ap_name, surf_type_text, bearing_from_to_icao
from (
select xp_loc.icao
      , mx_calc_distance (47.447502136, -122.307899475, xp_loc.lat, xp_loc.lon, 3440) as distance_nm
      , xp_loc.loc_rw, xp_loc.loc_type, xp_loc.frq_mhz, xp_loc.loc_bearing, xp_rw.rw_length_mt, xp_rw.rw_width, xa.ap_elev, xa.ap_name
      , case xp_rw.rw_surf when 1 then 'Asphalt' when 2 then 'Concrete' when 3 then 'Turf or grass' when 4 then 'Dirt' when 5 then 'Gravel' when 12 then 'Dry lakebed' when 13 then 'Water runways' when 14 then 'Snow or ice' when 15 then 'Transparent' else 'other' end as surf_type_text
      , mx_bearing(47.447502136, -122.307899475, xp_loc.lat, xp_loc.lon) as bearing_from_to_icao
from xp_loc, xp_rw, xp_airports xa
where xp_rw.icao = xp_loc.icao
and (xp_rw.rw_no_1 = xp_loc.loc_rw or xp_rw.rw_no_2 = xp_loc.loc_rw)
and xa.icao = xp_rw.icao
)
where 1 = 1
  and rw_length_mt >= 1000 and rw_width >= 45 and ap_elev >= 0  and distance_nm between 50 and 250 LIMIT 250 
;


select t1.icao_id
, FORMAT("%s (%s), coord: %3.4f/%3.4f (%ift) ", t1.ap_name, t1.icao, ap_lat, ap_lon, ap_elev) as data
, mx_calc_distance(t1.ap_lat, t1.ap_lon, 47.4381635,  -122.30388505, 3440) as distance, t1.ap_lat, t1.ap_lon
from xp_airports t1
where 1 = 1
and icao = 'CYVR' order by distance
; 


select t2.rw_no_1 || '/' || t2.rw_no_2 as rw_key
, FORMAT ("%-*s Length: %d meters", 10, t2.rw_no_1 || '/' || t2.rw_no_2, t2.rw_length_mt) as rw_data
, mx_calc_distance(t2.rw_no_1_lat, t2.rw_no_1_lon, 47.4381635, -122.30388505, 3440) as distance
from xp_rw t2
where 1 = 1
and icao_id = '2447'
;

WITH vor_dme AS (
  SELECT v1.ident, v1.loc_data, distance, 'VOR/DME' AS loc_type, v1.frq_mhz, identNameFrq, prevIdentNameFrq
    FROM (
           SELECT t3.ident,
                  FORMAT("%.2f (%i,%s) %s", CASE WHEN length(t3.frq_mhz) = 5 THEN t3.frq_mhz / 100.0 ELSE t3.frq_mhz END
                                          , mx_bearing(47.1377, -122.476, t3.lat, t3.lon)
                                          , 'VOR/DME'/* constant */
                                          , t3.name) AS loc_data,
                  mx_calc_distance(t3.lat, t3.lon, 47.1377, -122.476, 3440) AS distance,
                  t3.loc_type,
                  t3.frq_mhz,
                  IFNULL( (t3.ident || t3.name || t3.frq_mhz), t3.ident) AS identNameFrq,
                  lag(IFNULL( (t3.ident || t3.name || t3.frq_mhz), t3.ident)) OVER (ORDER BY t3.ident, t3.frq_mhz, t3.loc_type) AS prevIdentNameFrq,
                  lag(t3.loc_type) OVER (ORDER BY ident, frq_mhz, loc_type) AS prev_type
             FROM xp_loc t3
            WHERE distance <= 20
            and t3.loc_type in ('VOR', 'DME')
            ORDER BY distance, ident, frq_mhz, name
         )
         v1
   WHERE 1 = 1
     AND v1.identNameFrq = v1.prevIdentNameFrq
     AND ( (v1.loc_type = 'VOR' AND v1.prev_type = 'DME') OR
           (v1.loc_type = 'DME' AND v1.prev_type = 'VOR') )
)
SELECT v1.ident, v1.loc_data, v1.distance
  FROM vor_dme v1
UNION ALL
SELECT *
FROM (
    SELECT t1.ident
           , FORMAT("%.2f (%i,%s) %s", CASE WHEN length(t1.frq_mhz) = 5 THEN t1.frq_mhz / 100.0 ELSE t1.frq_mhz END, mx_bearing(47.1377, -122.476, t1.lat, t1.lon), t1.loc_type, t1.name) AS loc_data
           , mx_calc_distance(t1.lat, t1.lon, 47.1377, -122.476, 3440) as distance
      FROM xp_loc t1
     WHERE mx_calc_distance (t1.lat, t1.lon, 47.1377, -122.476, 3440) <= 20
       AND t1.loc_type IN ('VOR', 'DME')
       AND IFNULL( (t1.ident || t1.name || t1.frq_mhz), t1.ident) not in (select identNameFrq
                                                                            from vor_dme)
    UNION ALL
    SELECT t1.ident,
          FORMAT("%i (%i,%s) %s", t1.frq_mhz
                                , mx_bearing(47.1377, -122.476, t1.lat, t1.lon)
                                , t1.loc_type
                                , t1.name) AS loc_data
          , mx_calc_distance(t1.lat, t1.lon, 47.1377, -122.476, 3440) as distance
     FROM xp_loc t1
    WHERE mx_calc_distance(t1.lat, t1.lon, 47.1377, -122.476, 3440) <= 20
      AND t1.loc_type = 'NDB'
ORDER BY distance
)
;
 




select FORMAT ("%-*.3f %s", 10, xaf.frq/1000.0, xaf.frq_desc ) as frq_data
from xp_ap_frq xaf
where icao_id = 25459
;


select loc_data, distance, loc_rw
from (
select FORMAT ("%-*s , freq: %5.3f, Type: %s, Region: %-*s, lat/lon: %3.0f/%3.0f ", 4, t3.loc_rw, t3.frq_mhz*0.01, t3.loc_type, 3, icao_region_code, t3.lat, t3.lon ) as loc_data
,  mx_calc_distance(t3.lat, t3.lon, 47.4381635, -122.30388505, 3440) as distance
, t3.loc_rw
from xp_loc t3
where icao = 'KSFO'
and ( t3.loc_type like ('ILS%' ) or t3.loc_type like ('LOC%' ) )
--and t3.loc_type not in ('DME', 'VOR', 'NDB' )
) v1
Union ALL
select loc_data, distance, loc_rw
from (
select FORMAT ("%-*s , freq: %i, Type: %s, Region: %-*s, lat/lon: %3.0f/%3.0f ", 4, t3.loc_rw, t3.frq_mhz, t3.loc_type, 3, icao_region_code, t3.lat, t3.lon ) as loc_data
,  mx_calc_distance(t3.lat, t3.lon, 47.4381635, -122.30388505, 3440) as distance
, t3.loc_rw
from xp_loc t3
where icao = 'KSFO'
and ( t3.loc_type not like ('ILS%' ) and t3.loc_type not like ('LOC%' ) )
and t3.loc_type not in ('DME', 'VOR', 'NDB' )
) 
order by loc_rw

;




select loc_data, distance, loc_type_code, loc_rw
from (
select FORMAT ("%-*s , freq: %5.3f, Type: %s, Region: %-*s, lat/lon: %3.0f/%3.0f ", 4, t3.loc_rw, t3.frq_mhz*0.01, t3.loc_type, 3, icao_region_code, t3.lat, t3.lon ) as loc_data
,  mx_calc_distance(t3.lat, t3.lon, 47.4381635, -122.30388505, 3440) as distance
, t3.loc_rw
    , 1 as loc_type_code
from xp_loc t3
where icao = 'KSFO'
and ( t3.loc_type like ('ILS%' ) or t3.loc_type like ('LOC%' ) )
) v1
UNION ALL
select loc_data, distance, loc_type_code, loc_rw
from (
select FORMAT ("%-*s , freq: %i, Type: %s, Region: %-*s, lat/lon: %3.0f/%3.0f ", 4, t3.loc_rw, t3.frq_mhz, t3.loc_type, 3, icao_region_code, t3.lat, t3.lon ) as loc_data
,  mx_calc_distance(t3.lat, t3.lon, 47.4381635, -122.30388505, 3440) as distance
, t3.loc_rw
, 2 as loc_type_code
from xp_loc t3
where icao = 'KSFO'
and t3.loc_type not like ('ILS%' )
and t3.loc_type not like ('LOC%' )
and t3.loc_type not in ('DME', 'VOR', 'NDB' )
)
order by loc_type_code, loc_rw
;




            select icao, round(distance_nm) as distance_nm, loc_rw, loc_type, frq_mhz, loc_bearing, rw_length_mt, rw_width, ap_elev, ap_name, surf_type_text, bearing_from_to_icao, RANK() over (order by icao,distance_nm,loc_rw,loc_type) as rank
            from (
            select xp_loc.icao
              , mx_calc_distance (37.621147156, -122.375297546, xp_loc.lat, xp_loc.lon, 3440) as distance_nm
              , xp_loc.loc_rw, xp_loc.loc_type, xp_loc.frq_mhz, xp_loc.loc_bearing, xp_rw.rw_length_mt, xp_rw.rw_width, xa.ap_elev, xa.ap_name
              , case xp_rw.rw_surf when 1 then 'Asphalt' when 2 then 'Concrete' when 3 then 'Turf or grass' when 4 then 'Dirt' when 5 then 'Gravel' when 12 then 'Dry lakebed' when 13 then 'Water runways' when 14 then 'Snow or ice' when 15 then 'Transparent' else 'other' end as surf_type_text
              , mx_bearing(37.621147156, -122.375297546, xp_loc.lat, xp_loc.lon) as bearing_from_to_icao
            from xp_loc, xp_rw, xp_airports xa
            where xp_rw.icao = xp_loc.icao
            and (xp_rw.rw_no_1 = xp_loc.loc_rw or xp_rw.rw_no_2 = xp_loc.loc_rw)
            and xa.icao = xp_rw.icao
            )
            where 1 = 1
            and rw_length_mt >= 1000 and rw_width >= 45 and ap_elev >= 0  and distance_nm between 50 and 250 LIMIT 250
;                         





---------------------------------------------
-- Basic info on ICAO
select t1.icao_id
, FORMAT("%s (%s), coord: %3.4f/%3.4f (%ift) ", t1.ap_name, t1.icao, ap_lat, ap_lon, ap_elev) as data
, mx_calc_distance(t1.ap_lat, t1.ap_lon, 37.6210070,  -122.3870101, 3440) as distance, t1.ap_lat, t1.ap_lon
from xp_airports t1
where 1 = 1
and icao = 'KSFO' order by distance
;



-- Runway Info
select t2.rw_no_1 || '/' || t2.rw_no_2 as rw_key
, FORMAT ("%-*s Length: %d meters", 10, t2.rw_no_1 || '/' || t2.rw_no_2, t2.rw_length_mt) as rw_data
, mx_calc_distance(t2.rw_no_1_lat, t2.rw_no_1_lon, 37.6210070, -122.3870101, 3440) as distance
from xp_rw t2
where 1 = 1
and icao_id = '16457'
;

-- missionx T(238): Using fetch_vor_ndb_dme_info_step3 from sql.xml file
-- missionx T(239): Nav Data Query 3:
WITH vor_dme AS (
  SELECT v1.ident, v1.loc_data, distance, 'VOR/DME' AS loc_type, v1.frq_mhz, identNameFrq, prevIdentNameFrq
    FROM (
           SELECT t3.ident,
                  FORMAT("%.2f (%i,%s) %s", CASE WHEN length(t3.frq_mhz) = 5 THEN t3.frq_mhz / 100.0 ELSE t3.frq_mhz END
                                          , mx_bearing(37.6210070, -122.3870101, t3.lat, t3.lon)
                                          , 'VOR/DME'/* constant */
                                          , t3.name) AS loc_data,
                  mx_calc_distance(t3.lat, t3.lon, 16457, {4}, 3440) AS distance,
                  t3.loc_type,
                  t3.frq_mhz,
                  IFNULL( (t3.ident || t3.name || t3.frq_mhz), t3.ident) AS identNameFrq,
                  lag(IFNULL( (t3.ident || t3.name || t3.frq_mhz), t3.ident)) OVER (ORDER BY t3.ident, t3.frq_mhz, t3.loc_type) AS prevIdentNameFrq,
                  lag(t3.loc_type) OVER (ORDER BY ident, frq_mhz, loc_type) AS prev_type
             FROM xp_loc t3
            WHERE distance <= 20
            and t3.loc_type in ('VOR', 'DME')
            ORDER BY distance, ident, frq_mhz, name
         )
         v1
   WHERE 1 = 1
     AND v1.identNameFrq = v1.prevIdentNameFrq
     AND ( (v1.loc_type = 'VOR' AND v1.prev_type = 'DME') OR
           (v1.loc_type = 'DME' AND v1.prev_type = 'VOR') )
)
SELECT v1.ident, v1.loc_data, v1.distance
  FROM vor_dme v1
UNION ALL
SELECT *
FROM (
    SELECT t1.ident
           , FORMAT("%.2f (%i,%s) %s", CASE WHEN length(t1.frq_mhz) = 5 THEN t1.frq_mhz / 100.0 ELSE t1.frq_mhz END, mx_bearing({5}, {6}, t1.lat, t1.lon), t1.loc_type, t1.name) AS loc_data
           , mx_calc_distance(t1.lat, t1.lon, {7}, {8}, 3440) as distance
      FROM xp_loc t1
     WHERE mx_calc_distance (t1.lat, t1.lon, {9}, {10}, 3440) <= 20
       AND t1.loc_type IN ('VOR', 'DME')
       AND IFNULL( (t1.ident || t1.name || t1.frq_mhz), t1.ident) not in (select identNameFrq
                                                                            from vor_dme)
    UNION ALL
    SELECT t1.ident,
          FORMAT("%i (%i,%s) %s", t1.frq_mhz
                                , mx_bearing({11}, {12}, t1.lat, t1.lon)
                                , t1.loc_type
                                , t1.name) AS loc_data
          , mx_calc_distance(t1.lat, t1.lon, {13}, {14}, 3440) as distance
     FROM xp_loc t1
    WHERE mx_calc_distance(t1.lat, t1.lon, {15}, {16}, 3440) <= 20
      AND t1.loc_type = 'NDB'
ORDER BY distance
)
;





-- Output from online C++
select loc_data, distance, loc_type_code, loc_rw
from (
select FORMAT ("%-*s , freq: %5.3f, Type: %s, Region: %-*s, lat/lon: %3.0f/%3.0f ", 4, t3.loc_rw, t3.frq_mhz*0.01, t3.loc_type, 3, icao_region_code, t3.lat, t3.lon ) as loc_data
,  mx_calc_distance(t3.lat, t3.lon, 37.6210070, -122.3870101, 3440) as distance
, t3.loc_rw
, 1 as loc_type_code
from xp_loc t3
where icao = 'KSFO'
and ( t3.loc_type like ('ILS%' ) or t3.loc_type like ('LOC%' ) )
) v1
UNION ALL
select loc_data, distance, loc_type_code, loc_rw
from (
select FORMAT ("%-*s , channel: %i, Type: %s, Region: %-*s, lat/lon: %3.0f/%3.0f ", 4, t3.loc_rw, t3.frq_mhz, t3.loc_type, 3, icao_region_code, t3.lat, t3.lon ) as loc_data
,  mx_calc_distance(t3.lat, t3.lon, 37.6210070, -122.3870101, 3440) as distance
, t3.loc_rw
, 2 as loc_type_code
from xp_loc t3
where icao = 'KSFO'
and t3.loc_type not like ('ILS%' )
and t3.loc_type not like ('LOC%' )
and t3.loc_type not in ('DME', 'VOR', 'NDB' )
)
order by loc_type_code, loc_rw
;

select *
from xp_loc 
where icao = 'KSFO'

;










select loc_data, distance, loc_type_code, loc_rw
from (
select FORMAT ("%-*s , channel: %i/%s, Type: %s, Region: %-*s, lat/lon: %3.0f/%3.0f ", 4, t3.loc_rw, t3.frq_mhz, t3.ident, t3.loc_type, 3, icao_region_code, t3.lat, t3.lon ) as loc_data
,  mx_calc_distance(t3.lat, t3.lon, 37.6210070, -122.3870101, 3440) as distance
, t3.loc_rw
, 1 as loc_type_code
from xp_loc t3
where icao = 'KSFO'
and ( t3.loc_type like ('ILS%' ) or t3.loc_type like ('LOC%' ) )
) v1
UNION ALL
select loc_data, distance, loc_type_code, loc_rw
from (
select FORMAT ("%-*s , channel: %i, Type: %s, Region: %-*s, lat/lon: %3.0f/%3.0f ", 4, t3.loc_rw, t3.frq_mhz, t3.loc_type, 3, icao_region_code, t3.lat, t3.lon ) as loc_data
,  mx_calc_distance(t3.lat, t3.lon, 37.6210070, -122.3870101, 3440) as distance
, t3.loc_rw
, 2 as loc_type_code
from xp_loc t3
where icao = 'KSFO'
and t3.loc_type not like ('ILS%' )
and t3.loc_type not like ('LOC%' )
and t3.loc_type not in ('DME', 'VOR', 'NDB' )
)
order by loc_type_code, loc_rw
