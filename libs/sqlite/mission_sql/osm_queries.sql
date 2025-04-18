select *
from way_street_node_data t1
where 1 = 1
--and t1.lat between 47.374300 and  48.039838
--and t1.lon between -121.823910 and -120.804074
and t1.lat between 47.041820 and  48.397229
and t1.lon between -122.354284 and -120.321501
order by t1.id, t1.node_id
;

select * --distinct key_attrib
from ways_meta t1
where 1 = 1

order by 1
;

explain query plan
;
select count (1) --t2.id
from way_tag_data t2
where 1 = 1
and t2.key_attrib = 'highway'
;
--and t1.lat between 47.041820 and  48.397229
--and t1.lon between -122.354284 and -120.321501

-- Step 1, get group of IDs that holds lat/lon in area provided
explain query plan
select distinct t2.id
from way_street_node_data t1, way_tag_data t2
where 1 = 1
and t2.key_attrib = 'highway'
and t2.val_attrib in ('primary', 'secondary', 'tertiary', 'residential', 'service', 'living_street') 
and t1.id = t2.id
and t1.lat between 47.374300 and  48.039838
and t1.lon between -121.823910 and -120.804074
;


select round (t1.lat), count(1)
from way_street_node_data t1
group by round(t1.lat)
;

select  SQRT( SQUARE ( ( (47.711211) - (t1.lat) ) ) + SQUARE( ( (-121.340004) - (t1.lon) ) * COS(47.711211) ) ) as distance_2_points, t1.lat, t1.lon 
from way_street_node_data t1
where 1 = 1
and t1.lat between 47.041820 and  48.397229
and t1.lon between -122.354284 and -120.321501
-- check distance
--and SQRT(SQUARE((47.711211-48.039838))+ SQUARE(((-121.340004)-(-121.36129))*COS(47.711211)) 
-- select  SQRT( SQUARE ( ( (47.711211) - (48.039838) )) + SQUARE( ( (-121.340004) - (-121.36129) ) * COS(47.711211) )  
;

create table if not exists bounds
as
select min (t1.lat) as min_lat, max(t1.lat) as max_lat, min(t1.lon) as min_lon, max(t1.lon) as max_lon
from way_street_node_data t1
;

select min_lat, max_lat, min_lon, max_lon from bounds limit 1;
select min_lat, max_lat, min_lon, max_lon from bounds where min_lat <= 25.630151378432409 and max_lat >= 26.529543498000248 
;
                                                      and min_lon <= -80.647232024170378 and max_lon >= -79.645889942994216
;

select min_lat, max_lat, min_lon, max_lon, 
      case when min_lat <= 25.63015137 then 1 else 0 end as min_lat_test,
      case when max_lat >= 26.52954349 then 1 else 0 end as max_lat_test,
      case when min_lon <= -80.6472320 then 1 else 0 end as min_lon_test,
      case when max_lon >= -79.6458899 then 1 else 0 end as max_lon_test      
from bounds limit 1
;      
/*SQRT(SQUARE((TO_LAT-FROM_LAT)*110)+
     SQUARE((TO_LONG-FROM_LONG)*COS(TO_LAT)*111)) */



SELECT * FROM pragma_index_info('lat_idx');

-- step 2.1: Fetch all coordinate rows, order by node_id
select t2.id, t2.key_attrib, t2.val_attrib
from way_tag_data t2
where t2.id = 119112517
;
-- step 2.2: Fetch all meta data
select t1.node_id, t1.lat, t1.lon
from way_street_node_data t1
where t1.id = 119112517
order by t1.node_id
;

select t1.node_id, t1.lat, t1.lon from way_street_node_data t1 where t1.id = ?1 order by t1.node_id
;

create index id_way_street_idx on way_street_node_data(id);
-- create index lat_idx on way_street_node_data(lat);
-- create index lon_idx on way_street_node_data(lon);

create index id_way_tag_idx on way_tag_data(id);
create index key_attrib_idx on way_tag_data(key_attrib);
----------------------


select t2.id, t2.key_attrib, t2.val_attrib, t1.node_id, t1.lat, t1.lon
from way_street_node_data t1, way_tag_data t2
where t2.id = 119112517
and t1.id = t2.id
--and t2.key_attrib = 'highway'
--and t2.val_attrib in ('primary', 'secondary', 'tertiary', 'residential', 'service', 'living_street') 
;



-- Full information on lat/lon
--select distinct t2.val_attrib
select t2.*, t1.node_id, t1.lat, t1.lon
from way_street_node_data t1, way_tag_data t2
where 1 = 1
and t1.lat between 47.374300 and  48.039838
and t1.lon between -121.823910 and -120.804074
and t1.id = t2.id
and t2.key_attrib = 'highway'
and t2.val_attrib in ('primary', 'secondary', 'tertiary', 'residential', 'service', 'living_street') 
--and t2.val_attrib not in ('track', 'path') 
;








select t2.key_attrib, count (t2.key_attrib)
from  way_tag_data t2
group by t2.key_attrib
order by 1
;

-- abandoned%, agricultural, amenity%, area, basin, beach, bicycle:lanes, bicycle_road, brewery, bridge%, cabins, camp_site, cargo, club, construction, description,dock, factory, ferry, finshing, golf, grass, highway, hiking, hunting, icao, healthcare, lake, landmark, monastery:type, 
-- natural, observatory:type, park, pickup, picnic, pipeline, place, playground, police, port, public_transit, public_transport, pumping_station, railway, railway_1, ruins, runway, shooting, station, street, structure, target, tourism, tunnel,
-- water, waterway, wood
-- highway, waterway, pickup, school, demolished, motorboat, subway, swimming_pool, shooting, structure, park, storage, climbing, site, 


select *
from way_tag_data t1
where t1.id = 4635525
;

select t1.*
from way_tag_data t1
where t1.id in
(
 select t2.id
 from way_tag_data t2
 where t2.key_attrib like 'highway%'
 )
;

select *
from way_street_node_data t2
where t2.id = 4635525
order by t2.lat
;

select id, ''''||id||''', ', id||', '
from 
(
    select id
    from way_tag_data t1
    except 
    select id
    from way_street_node_data
)
;

select t1.*, t2.*
from way_tag_data t1, way_street_node_data t2
where t1.id = t2.id
and t1.key_attrib = 'highway'
--and t1.val_attrib = 'road'
;

-- find all nodes that have same lat/lon but not same id
select *
from way_tag_data wtd1, way_street_node_data t1,way_street_node_data t2
where 1 = 1
and t1.lat = t2.lat
and t1.lon = t2.lon
and t1.id != t2.id
and wtd1.id = t1.id
and wtd1.key_attrib = 'highway'
--and wtd1.val_attrib = 'road'
;

select *
from xx_intersections t1
where t1.val_attrib in ( 'abandoned' )
;

select distinct '''' || t1.val_attrib || ''', '
from xx_intersections t1
where t1.val_attrib not in ('footway', 'cycleway', 'motorway', 'unclassified', 'razed', 'proposed', 'stairs', 'corridor', 'trunc_link', 'stairs', 'steps','construction')

;

drop table xx_intersections; -- living_street, motorway_link, razed, proposed, stairs, bridleway, bridleway

-- create intersection table
create table if not exists xx_intersections
as
select distinct wtd1.*, t1.lat, t1.lon --, t2.id as t2_id
from way_tag_data wtd1, way_street_node_data t1,way_street_node_data t2
where 1 = 1
and t1.lat = t2.lat
and t1.lon = t2.lon
and t1.id != t2.id
and wtd1.id = t1.id
and (wtd1.key_attrib = 'highway'
--and wtd1.val_attrib not in ('footway', 'cycleway', 'motorway', 'unclassified', 'razed', 'proposed', 'stairs', 'corridor', 'trunc_link', 'stairs', 'steps','construction')
and wtd1.val_attrib in ('primary', 'tertiary', 'service', 'secondary', 'track', 'residential', 'path', 'living_street', 'trunk', 'motorway_link', 'tertiary_link', 'trunk_link', 'road', 'secondary_link', 'pedestrian', 'primary_link', 'abandoned', 'planned', 'raceway', 'bridleway')
)
or (
wtd1.val_attrib in ('bridge', 'park')
)
;

-- create intersection vu, if you don't want to use the table xx_intersections
create view vu_intersections
as
select wtd1.*, t1.lat, t1.lon, t2.id as t2_id
from way_tag_data wtd1, way_street_node_data t1,way_street_node_data t2
where 1 = 1
and t1.lat = t2.lat
and t1.lon = t2.lon
and t1.id != t2.id
and wtd1.id = t1.id
and wtd1.key_attrib = 'highway'
;


select /*distinct*/ wtd1.*, t1.lat, t1.lon
from way_tag_data wtd1, way_street_node_data t1,way_street_node_data t2
where 1 = 1
and t1.lat = t2.lat
and t1.lon = t2.lon
and t1.id != t2.id
and wtd1.id = t1.id
and wtd1.key_attrib = 'area'
--and wtd1.val_attrib = 'bridge'
;

select distinct t1.key_attrib, t1.key_attrib ||'='||t1.val_attrib , '='||t1.val_attrib 
from way_tag_data t1
where 1 = 1
and  t1.key_attrib = "highway"
and t1.val_attrib in ('primary', 'tertiary', 'service', 'secondary', 'track', 'residential', 'path', 'living_street', 'trunk', 'motorway_link', 'tertiary_link', 'trunk_link', 'road', 'secondary_link', 'pedestrian', 'primary_link', 'abandoned', 'planned', 'raceway', 'bridleway')
order by 1
;


/***
osmfilter {file} --keep="highway=primary
***/

select distinct t1.key_attrib, count(1)
from way_tag_data t1
--where t1.id = 4635525;
--where t1.id
--and t1.key_attrib like 'bridge'
--and t1.val_attrib = 'road'
group by t1.key_attrib
order by 1
;


/*
and t1.id in (
                select id
                from  way_tag_data t2
                where key_attrib in ('agricultural', 'area', 'basin', 'beach', 'bicycle:lanes', 'bicycle_road', 'brewery', 'cabins', 'camp_site', 'cargo', 'club', 'construction', 'description','dock', 'factory', 'ferry', 'finshing', 'golf', 'grass', 'highway', 'hiking', 'hunting', 'icao', 'healthcare', 'lake', 'landmark', 'monastery:type', 'natural', 'observatory:type', 'park', 'pickup', 'picnic', 'pipeline', 'place', 'playground', 'police', 'port', 'public_transit', 'public_transport', 'pumping_station', 'railway', 'railway_1', 'ruins', 'runway', 'shooting', 'station', 'street', 'structure', 'target', 'tourism', 'tunnel', 'water', 'waterway', 'wood')
                union all 
                select id
                from  way_tag_data t2
                where key_attrib like 'abandoned%'
                or key_attrib like 'amenity%' 
                or key_attrib like 'bridge%'
                )
;
*/
begin transaction;

delete from way_tag_data
where id in
(
    select distinct id
    from way_tag_data
    except 
    select distinct id
    from way_street_node_data
)    
;

select *
from way_tag_data
where 1 = 1
and  id = 608003310
--and lower(val_attrib) = 'wa53'
;

select *
from way_street_node_data
where id = 25054082
;


rollback;

-- end transaction;


select id
from way_street_node_data
except 
select id
from way_tag_data;

select count(distinct id)
from way_street_node_data;

select *
from way_tag_data t1
where t1.id in (
select id
from way_tag_data
where val_attrib like '%helip%'
)
;

select *
from way_tag_data 
where val_attrib like 'Harborview Medical Center'
;





select t2.*
from way_street_node_data t1, way_tag_data t2 
where 1 = 1 and t1.id = t2.id 
and t1.lat between 41.973817300000000 and 47.015354752826021 
and t1.lon between -122.92225571664785 and -121.69122485779864
and key_attrib='aeroway' 
and val_attrib='helipad' and 
t2.id in ( select id from way_tag_data where key_attrib='faa' and val_attrib is not null ) 

