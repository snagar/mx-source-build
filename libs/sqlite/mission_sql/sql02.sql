
select * from xp_airports;

-- attach example
attach 'f:\X-Plane11\Resources\plugins\missionx\db\airports_xp.sqlite.db' as sourceDB;
attach 'd:\xp11clean\Resources\plugins\missionx\db\airports_xp.sqlite.db' as targetDB;
INSERT INTO targetDB.xp_airports SELECT * FROM sourceDB.xp_airports;

update xp_airports
set ap_lat = trunc((select lat from (select ROW_NUMBER() OVER (partition by t1.icao) as row, t1.lat from xp_helipads t1 where t1.icao = 'EY01') where row = 1)),
    ap_lon = trunc((select lon from (select ROW_NUMBER() OVER (partition by t1.icao) as row, t1.lon from xp_helipads t1 where t1.icao = 'EY01') where row = 1))
where xp_airports.icao = 'EY01' and xp_airports.ap_lat is null
;

select t1.lat from xp_helipads t1 where t1.icao = 'EY01' and ROW_NUMBER() = 1 group by t1.icao -- having ROW_NUMBER() over (PARTITION BY t1.icao) = 1
;


CREATE UNIQUE INDEX icao_name_n1 on xp_airports (icao, ap_name);
CREATE INDEX icao_helipad_n1 on xp_helipads (icao);
CREATE INDEX icao_rw_n1 on xp_airports (icao, ap_name);

select * from xp_ap_ramps t1 where t1.icao = 'KSEA';

-- delete from xp_airports where icao_id > 10;

select * from xp_airports out
where out.icao in 
(
select icao
from xp_airports t1
group by icao
having count(icao) > 1
)
;

-- PLANE Point: <point lat="-19.25418870" long="146.77017290" elev_ft="17.95" 	template=""  loc_desc="" type=""   heading_psi="193.68" groundElev="17.9494" ft replace_lat="-19.25418870" replace_long="146.77017290" />
-- Min distance 5.0
-- max_distance 12.0
-- exclude bearing
select * from xp_airports;

select icao, ap_name, mx_distance_nm ( ap_lat, ap_lon, -19.25418870, 146.77017290, 3440) as dist_nm, mx_bearing (ap_lat, ap_lon, -19.25418870, 146.77017290) as bearing from xp_airports 
;

select * from
(
    select icao_id, icao, ap_name, ap_lat, ap_lon, mx_distance_nm ( ap_lat, ap_lon, -19.25418870, 146.77017290, 3440) as dist_nm, mx_bearing (ap_lat, ap_lon, -19.25418870, 146.77017290) as bearing from xp_airports 
) v1
where 1 = 1
and v1.dist_nm between 3.0 and 30.0
and bearing between 5 and 351
order by dist_nm
;

select * from 
( select icao_id, icao, ap_name, ap_lat, ap_lon
       , mx_distance_nm ( ap_lat, ap_lon,-19.2684955600, 146.8660278000, 3440) as dist_nm
       , mx_bearing (ap_lat, ap_lon, -19.2684955600, 146.8660278000) as bearing 
  from xp_airports  ) 
where 1 = 1  
and dist_nm between 1.5 and 40
;


select * from 
( select icao_id, icao, ap_name, ap_lat, ap_lon
       , mx_distance_nm ( ap_lat, ap_lon,-19.6499195000, 146.3011627000, 3440) as dist_nm
       , mx_bearing (ap_lat, ap_lon, -19.6499195000, 146.3011627000) as bearing 
  from xp_airports  ) 
where 1 = 1  
and dist_nm between 1.5 and 120 
and (bearing between 0 and 65 or bearing between 75 and 360)
;

select * from 
( select icao_id, icao, ap_name, ap_lat, ap_lon, mx_distance_nm ( ap_lat, ap_lon,-19.1918487500, 146.4940186000, 3440) as dist_nm, mx_bearing (ap_lat, ap_lon, -19.1918487500, 146.4940186000) as bearing from xp_airports  ) 
where 1 = 1  and dist_nm between 1.5 and 55 and ( bearing < 265 or bearing > 275 )
;


----- VIEW that holds most of airport information

SELECT t1.icao_id,
       t1.icao,
       t1.ap_elev,
       t1.ap_name,
       t1.is_custom,
       t1.ap_lat,
       t1.ap_lon,
       (select count(1) from xp_helipads v1 where t1.icao_id = v1.icao_id) as helipads,
       (select count(1) from xp_ap_ramps v1 where t1.icao_id = v1.icao_id and for_planes = 'helos') as ramp_helos,
       (select count(1) from xp_ap_ramps v1 where t1.icao_id = v1.icao_id and for_planes != 'helos') as ramp_planes       
  FROM xp_airports t1
;


select icao_id, count(1) as ramp_planes from xp_ap_ramps where for_planes like '%props%' group by icao_id
;

select * from xp_ap_ramps t1 where t1.for_planes like '%props%' and location_type = 'gate' 
;
-- drop view if exists airports_vu

WITH helipads_view as (select icao_id, count(1) as helipad_counter from xp_helipads group by icao_id),
     heli_ramps_view as (select icao_id, count(1) as ramp_helis from xp_ap_ramps where for_planes like '%helos%' or lower(ramp_uq_name) like '%heli%' group by icao_id),
     plane_ramps_view as (select icao_id, count(1) as ramp_planes from xp_ap_ramps where (for_planes <> 'helos' or for_planes is null) and lower(ramp_uq_name) not like '%heli%' and lower(ramp_uq_name) not like '%hold short%' group by icao_id),
     props_ramps_view as (select icao_id, count(1) as ramp_planes from xp_ap_ramps where for_planes like '%props%' group by icao_id),
     turboprop_ramps_view as (select icao_id, count(1) as ramp_planes from xp_ap_ramps where for_planes like '%turboprops%' group by icao_id),
     jets_heavy_ramps_view as (select icao_id, count(1) as ramp_planes from xp_ap_ramps where for_planes like '%jets%' or for_planes like '%heavy%' group by icao_id),
     rw_hard_vu as (select icao_id, count(1) as rw_hard from xp_rw where rw_surf in (1,2) group by icao_id),
     rw_dirt_gravel_vu as (select icao_id, count(1) as rw_dirt_n_gravel from xp_rw where rw_surf in (4,5) group by icao_id),
     rw_grass_vu as (select icao_id, count(1) as rw_grass from xp_rw where rw_surf = 3 group by icao_id),     
     rw_water_vu as (select icao_id, count(1) as rw_water from xp_rw where rw_surf = 13 group by icao_id)     
SELECT t1.icao_id,
       t1.icao,
       t1.ap_elev,
       t1.ap_name,
       t1.ap_type,
       t1.is_custom,
       t1.ap_lat,
       t1.ap_lon
       ,IFNULL((select helipad_counter from helipads_view v1 where t1.icao_id = v1.icao_id), 0) as helipads
       ,IFNULL((select ramp_helis from heli_ramps_view v1 where t1.icao_id = v1.icao_id ), 0) as ramp_helos
       ,IFNULL((select ramp_planes from plane_ramps_view v1 where t1.icao_id = v1.icao_id ), 0) as ramp_planes       
       ,IFNULL((select ramp_planes from props_ramps_view v1 where t1.icao_id = v1.icao_id ), 0) as ramp_props       
       ,IFNULL((select ramp_planes from turboprop_ramps_view v1 where t1.icao_id = v1.icao_id ), 0) as ramp_turboprops       
       ,IFNULL((select ramp_planes from jets_heavy_ramps_view v1 where t1.icao_id = v1.icao_id ), 0) as ramp_jet_heavy       
       ,IFNULL((select rw_hard from rw_hard_vu v1 where t1.icao_id = v1.icao_id ), 0) as rw_hard           
       ,IFNULL((select rw_dirt_n_gravel from rw_dirt_gravel_vu v1 where t1.icao_id = v1.icao_id ), 0) as rw_dirt_gravel           
       ,IFNULL((select rw_grass from rw_grass_vu v1 where t1.icao_id = v1.icao_id ), 0) as rw_grass           
       ,IFNULL((select rw_water from rw_water_vu v1 where t1.icao_id = v1.icao_id ), 0) as rw_water           
  FROM xp_airports t1
;

select * from airports_vu;


-- KSEA lat:  47.438100, lon:  -122.312958
select *
from (
select v1.*, mx_distance_nm ( ap_lat, ap_lon,47.438100, -122.312958 , 3440) as dist_nm, mx_bearing (ap_lat, ap_lon, 47.438100, -122.312958) as bearing from airports_vu v1
) mv1
where mv1.dist_nm between 5 and 25.0
and helipads + ramp_helos > 0 -- filter airports that have helipads
order by dist_nm
;



select count(1)
from xp_ap_ramps t1
where t1.for_planes != 'helos'
and t1.for_planes like '%hel%'
;

SELECT *
  FROM (
         SELECT icao_id,
                icao,
                ap_name,
                ap_lat,
                ap_lon,
                mx_distance_nm(ap_lat, ap_lon, -19.35930443, 146.6625519, 3440) AS dist_nm,
                mx_bearing(ap_lat, ap_lon, -19.35930443, 146.6625519) AS bearing
           FROM airports_vu
       )
 WHERE 1 = 1 AND 
       dist_nm BETWEEN 1.5 AND 40
 ORDER BY dist_nm;
;





  SELECT icao_id, icao, ap_name
    FROM (
           SELECT t1.*,
                  LAG(icao_id, 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao) pos_in_group,
                  mx_distance_nm(t1.ap_lat, t1.ap_lon, LAG(t1.ap_lat, 1, t1.ap_lat) OVER (PARTITION BY t1.icao ORDER BY t1.icao,
                                 t1.icao_id), LAG(t1.ap_lon, 1, t1.ap_lon) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id), 3440) AS distance,
                  trunc(t1.ap_lat) - LAG(trunc(t1.ap_lat), 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id) as lat_diff,
                  trunc(t1.ap_lon) - LAG(trunc(t1.ap_lon), 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id) as lon_diff
             FROM xp_airports t1
            WHERE t1.icao IN (
                    SELECT icao
                      FROM xp_airports t3
                     GROUP BY icao
                    HAVING count(icao) > 1
                  )
            ORDER BY icao,
                     icao_id
         )
         v1
   WHERE v1.pos_in_group != 0
   --Less than 5 nautical miles will be looked as duplicate airport that we can remove
   -- or they are in same major coordinates (40, -74)
   and (v1.distance < 5 or (lat_diff = 0 and lon_diff = 0) )
;


-- delete all duplicate ICAOs based on the distance between duplicate ICAOs
DELETE FROM xp_airports
      WHERE icao_id IN (
  SELECT icao_id
    FROM (
           SELECT t1.*,
                  LAG(icao_id, 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao) pos_in_group,
                  -- Gather info regarding the distance between Two ICAOs
                  mx_distance_nm(t1.ap_lat, t1.ap_lon, LAG(t1.ap_lat, 1, t1.ap_lat) OVER (PARTITION BY t1.icao ORDER BY t1.icao,
                                 t1.icao_id), LAG(t1.ap_lon, 1, t1.ap_lon) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id), 3440) AS distance,
                  -- info regarding major cooridinate of current ICAO vs the duplicated one. If the ICAO is the first one than we return same LAT/LON values only truncated (without decimals)
                  trunc(t1.ap_lat) - LAG(trunc(t1.ap_lat), 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id) as lat_diff,
                  trunc(t1.ap_lon) - LAG(trunc(t1.ap_lon), 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao, t1.icao_id) as lon_diff
             FROM xp_airports t1
            WHERE t1.icao IN (
                    SELECT icao
                      FROM xp_airports t3
                     GROUP BY icao
                    HAVING count(icao) > 1
                  )
            ORDER BY icao,
                     icao_id
         )
         v1
   WHERE v1.pos_in_group != 0
   --Less than 5 nautical miles will be looked as duplicated airport that we can remove
   -- or they are in same major coordinates (example: 40, -74)
   and (v1.distance < 5 or (lat_diff = 0 and lon_diff = 0) )
)
;



  SELECT icao_id, icao
    FROM (
           SELECT t1.*,
                  LAG(icao_id, 1, 0) OVER (PARTITION BY t1.icao ORDER BY t1.icao) pos_in_group,
                  mx_distance_nm(t1.ap_lat, t1.ap_lon, LAG(t1.ap_lat, 1, t1.ap_lat) OVER (PARTITION BY t1.icao ORDER BY t1.icao,
                                 t1.icao_id), LAG(t1.ap_lon, 1, t1.ap_lon) OVER (PARTITION BY t1.icao ORDER BY t1.icao,
                                 t1.icao_id), 3440) AS distance
             FROM xp_airports t1
            WHERE t1.icao IN (
                    SELECT icao
                      FROM xp_airports t3
                     GROUP BY icao
                    HAVING count(icao) > 1
                  )
            ORDER BY icao,
                     icao_id
         )
         v1
   WHERE v1.pos_in_group != 0
   and v1.distance < 5 --Less than 5 nautical miles will be looked as duplicate airport that we can remove
;   


WITH -- helipads_view as (select icao_id, count(1) as helipad_counter from xp_helipads group by icao_id),
     heli_ramps_view as (select icao_id, count(1) as ramp_helis from xp_ap_ramps where for_planes like '%helos%' or lower(ramp_uq_name) like '%heli%' group by icao_id),
     plane_ramps_view as (select icao_id, count(1) as ramp_planes from xp_ap_ramps where (for_planes <> 'helos' or for_planes is null) and lower(ramp_uq_name) not like '%heli%' and lower(ramp_uq_name) not like '%hold short%' group by icao_id),
     props_ramps_view as (select icao_id, count(1) as ramp_planes from xp_ap_ramps where for_planes like '%props%' group by icao_id),
     turboprop_ramps_view as (select icao_id, count(1) as ramp_planes from xp_ap_ramps where for_planes like '%turboprops%' group by icao_id),
     jets_heavy_ramps_view as (select icao_id, count(1) as ramp_planes from xp_ap_ramps where for_planes like '%jets%' or for_planes like '%heavy%' group by icao_id)
;
     

SELECT icao_id, icao, ramp_lat as lat, ramp_lon as lon, ramp_heading_true heading, ramp_uq_name as name
     , case when (instr( lower(IFNULL(for_planes,'')), 'helos') > 0) or (instr( lower(IFNULL(ramp_uq_name,'')), 'heli') > 0) then 1  else 0 END as helos
     , case when instr( lower(IFNULL(for_planes,'')), 'props') > 0 or  (instr( lower(IFNULL(ramp_uq_name,'')), 'general') > 0) then 1 else 0 END as props
     , case when instr( lower(IFNULL(for_planes,'')), 'turboprops') > 0 or  (instr( lower(IFNULL(ramp_uq_name,'')), 'apron') > 0) then 1 else 0 END as turboprops
     , case when instr( lower(IFNULL(for_planes,'')), 'heavy') > 0 or (instr( lower(IFNULL(ramp_uq_name,'')), 'jet') > 0) or (instr( lower(IFNULL(ramp_uq_name,'')), 'apron') > 0) then 1 else 0 END as jet_n_heavy
     , 1 as which_table
FROM xp_ap_ramps t1
union all
SELECT icao_id, icao, lat, lon, 0 as heading, name, 1 as helos, 0 as props, 0 as turboprops, 0 as jet_n_heavy, 2 as which_table
FROM xp_helipads t1
ORDER BY icao_id
;

create view ramps_vu as
SELECT icao_id, icao, ramp_lat as lat, ramp_lon as lon, ramp_heading_true heading, ramp_uq_name as name
     -- All the last "OR" logic in each case statements is for compatibility with old scenery files codes and format. We try to guess the ramp type
     , case when (instr( lower(IFNULL(for_planes,'')), 'helos') > 0) or (instr( lower(IFNULL(ramp_uq_name,'')), 'heli') > 0) then 1  else 0 END as helos  
     , case when instr( lower(IFNULL(for_planes,'')), 'props') > 0 or  (instr( lower(IFNULL(ramp_uq_name,'')), 'general') > 0) then 1 else 0 END as props
     , case when instr( lower(IFNULL(for_planes,'')), 'turboprops') > 0 or  (instr( lower(IFNULL(ramp_uq_name,'')), 'apron') > 0) then 1 else 0 END as turboprops
     , case when instr( lower(IFNULL(for_planes,'')), 'heavy') > 0 or (instr( lower(IFNULL(ramp_uq_name,'')), 'jet') > 0) or (instr( lower(IFNULL(ramp_uq_name,'')), 'apron') > 0) then 1 else 0 END as jet_n_heavy
     , 1 as which_table
FROM xp_ap_ramps t1
union
SELECT icao_id, icao, lat, lon, 0 as heading, name, 1 as helos, 0 as props, 0 as turboprops, 0 as jet_n_heavy, 2 as which_table
FROM xp_helipads t1
;



select * from ramps_vu v1
where v1.icao = 'YCSW';

  SELECT icao_id,
         icao,
         lat,
         lon,
         0 AS heading,
         name,
         "helos" AS for_planes,
         1 AS helos,
         0 AS props,
         0 AS turboprops,
         0 AS jet_n_heavy,
         2 AS which_table
    FROM xp_helipads t1
   ORDER BY icao_id;

 
