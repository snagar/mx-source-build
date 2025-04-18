 select *
 from airport_csv 
 group by type;
 
DETACH DATABASE xpdb;
ATTACH DATABASE 'd:\xp11clean\Resources\plugins\missionx\db\airports_xp.sqlite.db' as xpdb;

select * from xpdb.xp_airports t2;

select * from airport_csv t1
where t1.local_code = 'CYZF' or gps_code = 'CYZF' or ident='CYZF'
;
-- LLBG

select * from airport_csv t1, xpdb.xp_airports t2
where 1 =1 
and t1.ident = t2.icao
and  mx_distance_nm ( t2.ap_lat, t2.ap_lon, t1.latitude_deg, t1.longitude_deg, 3440) < 5.0
and t1.ident = '05NJ'
--and t1.local_code = 'CYZF' or gps_code = 'CYZF' or ident='CYZF'
;

select *
from 
(
select t2.*, count(1) over (partition by icao order by icao_id) icao_count
from xpdb.xp_airports t2
) v1
where v1.icao_count > 1
and icao = '05NJ'
order by icao, icao_id
;

select icao, count(icao) from xp_airports t3
group by icao
having count(icao) > 1
;

select *
from 
(
select t1.*
 from xp_airports t1
where t1.icao in (
select icao from xp_airports t3
group by icao
having count(icao) > 1
)
order by icao, icao_id
) v1
;

















select * 
from xpdb.xp_airports t2, 
    (select icao, count(icao) from xpdb.xp_airports 
    group by icao
    having count(icao) > 1
    ) v1
    , airport_csv t1 
where t2.icao = v1.icao
and t1.ident = t2.icao
and mx_distance_nm ( t2.ap_lat, t2.ap_lon, t1.latitude_deg, t1.longitude_deg, 3440) > 5.0
order by icao, icao_id
;

select * from xp_helipads t1
where t1.icao = 'NY99';

select * from xp_airports t1
where icao_id = 17416; -- (40.767571, -73.97255511)

-- 604 17416
update xp_airports
set ap_lat = (select lat from(select ROW_NUMBER() OVER(partition by t1.icao_id) as row, t1.lat from xp_helipads t1 where t1.icao_id = 17416) where row = 1),
    ap_lon = (select lon from(select ROW_NUMBER() OVER(partition by t1.icao_id) as row, t1.lon from xp_helipads t1 where t1.icao_id = 17416) where row = 1)
where xp_airports.icao_id = 17416 
--and xp_airports.ap_lat is null
;



select * from xp_airports t1 
ORDER BY RANDOM() LIMIT 10;


SELECT *
  FROM (
         SELECT icao_id,
                icao,
                ap_name,
                ap_type,
                ap_lat,
                ap_lon,
                mx_distance_nm(ap_lat, ap_lon, -20.047399521, 146.2666016, 3440) AS dist_nm,
                mx_bearing(ap_lat, ap_lon, -20.047399521, 146.2666016) AS bearing,
                helipads,
                ramp_helos,
                ramp_planes,
                rw_hard,
                rw_dirt_gravel,
                rw_grass,
                rw_water
           FROM airports_vu
       )
 WHERE 1 = 1 AND 
       dist_nm BETWEEN 1.5 AND 50 AND 
       ap_type = 1 AND 
       ramp_planes > 0
/* ORDER BY RANDOM() 
 LIMIT 1;*/
;

SELECT *
  FROM (
         SELECT icao_id,
                icao,
                ap_name,
                ap_type,
                ap_lat,
                ap_lon,
                mx_distance_nm(ap_lat, ap_lon, -19.41603279, 146.5417328, 3440) AS dist_nm,
                mx_bearing(ap_lat, ap_lon, -19.41603279, 146.5417328) AS bearing,
                helipads,
                ramp_helos,
                ramp_planes,
                rw_hard,
                rw_dirt_gravel,
                rw_grass,
                rw_water
           FROM airports_vu
       )
 WHERE 1 = 1 AND 
       dist_nm BETWEEN 1.5 AND 56 AND 
       (bearing < 185 OR 
        bearing > 195) AND 
       ap_type = 1 AND 
       ramp_planes > 0
 --ORDER BY RANDOM() 
 --LIMIT 1
 ;



SELECT *
  FROM (
         SELECT icao_id,
                icao,
                ap_name,
                ap_type,
                ap_lat,
                ap_lon,
                mx_distance_nm(ap_lat, ap_lon, -20.014005661, 147.2520294, 3440) AS dist_nm,
                mx_bearing(ap_lat, ap_lon, -20.014005661, 147.2520294) AS bearing,
                helipads,
                ramp_helos,
                ramp_planes,
                rw_hard,
                rw_dirt_gravel,
                rw_grass,
                rw_water
           FROM airports_vu
       )
 WHERE 1 = 1 AND 
       dist_nm BETWEEN 1.5 AND 35 AND 
       (bearing < 215 OR 
        bearing > 225) AND 
       ap_type = 1 AND 
       ramp_planes > 0
-- ORDER BY RANDOM() 
-- LIMIT 1
 ;


select * from airports_vu where icao='YBLP';




SELECT *
  FROM (
         SELECT icao_id,
                icao,
                ap_elev_ft,
                ap_name,
                ap_type,
                ap_lat,
                ap_lon,
                mx_distance_nm(ap_lat, ap_lon, -19.25418854, 146.7701721, 3440) AS dist_nm,
                mx_bearing(ap_lat, ap_lon, -19.25418854, 146.7701721) AS bearing,
                helipads,
                ramp_helos,
                ramp_planes,
                ramp_props,
                ramp_turboprops,
                ramp_jet_heavy,
                rw_hard,
                rw_dirt_gravel,
                rw_grass,
                rw_water
           FROM airports_vu
       )
       as vu
 WHERE 1 = 1 AND 
       dist_nm BETWEEN 5 AND 55 AND 
       ap_type = 1 AND 
       ramp_props > 0 AND 
       0 < (
             SELECT count(1) 
               FROM xp_rw xr
              WHERE xr.rw_surf IN (1, 2) AND 
                    xr.icao_id = vu.icao_id
           )
 ORDER BY RANDOM() 
 LIMIT 10;








drop table products;

CREATE TABLE products (
  item_id              INTEGER     PRIMARY KEY,
  item_type            TEXT (10)   NOT NULL
                                   CONSTRAINT item_type_fk REFERENCES item_types (item_type),
  item_barcode         TEXT (100)  NOT NULL,
  item_weight_kg       REAL (5, 2) NOT NULL
                                   DEFAULT (0) 
                                   CONSTRAINT check_weight_less_10k_n2 CHECK (item_weight_kg < 10000.0),
  min_plane_size       INTEGER (1) DEFAULT (0),
  rearity_pct          INTEGER (2) DEFAULT (0),
  item_display_name    TEXT (100),
  item_desc            TEXT (200),
  dataref_restrictions TEXT (1024),
  comment              TEXT (1024),
  CONSTRAINT uq_type_barcode_n2 UNIQUE (
    item_type,
    item_barcode
  )
);




CREATE TABLE item_types (
  item_type      TEXT (5)  UNIQUE
                           NOT NULL,
  item_type_desc TEXT (20),
  comments       TEXT
);


select * from stats;
select * from stats_copy;

create table stats_copy
as
select * from stats;

delete FROM stats;

insert into stats
select * from stats_copy;



select sum ( active_time_in_sec_in_aday ) as total_mission_time_sec from ( select MAX(local_time_sec) - MIN(local_time_sec) as active_time_in_sec_in_aday, local_date_days, MAX(local_time_sec),MIN(local_time_sec) from stats group by local_date_days) v1
;

select sum ( active_time_in_sec_in_aday ) as total_mission_time_sec from ( select MAX(local_time_sec) - MIN(local_time_sec) as active_time_in_sec_in_aday from stats group by local_date_days) v1
;

select elev, flap_ratio * 100 as flap_ratio, local_date_days, local_time_sec, case when airspeed < 0.0 then 0.0 else airspeed end as airspeed , groundspeed, faxil_gear, roll, activity from stats order by line_id
;

SELECT min(t1.vvi_fpm_pilot) AS min_vvi_fpm_pilot,
		   max(t1.vvi_fpm_pilot) AS max_vvi_fpm_pilot,
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
	  FROM  (select vvi_fpm_pilot, vh_ind, gforce_normal, gforce_axil, aoa, pitch, elev, flap_ratio * 100 as flap_ratio, local_date_days, local_time_sec, case when airspeed < 0.0 then 0.0 else airspeed end as airspeed , groundspeed, faxil_gear, roll, activity from stats order by line_id) t1











