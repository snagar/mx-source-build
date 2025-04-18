-- xp_airports
drop table if exists xp_airports;

-- codes: 1, 16, 17
create table if not exists xp_airports
(
icao text primary key not null,
ap_elev integer,
ap_name text
)
;

--insert into xp_airports (icao, ap_elev, ap_name)
--values ('test', 25, 'test ap');

select * from xp_airports;

--insert into xp_airports (icao, ap_elev, ap_name) values ('00C', 6684, 'Animas Air Park');

-- delete from xp_airports;
commit;

 -- xp_ap_metadata: airports metadata code 1302
drop table if exists xp_ap_metadata;
create table if not exists xp_ap_metadata
(
icao text NOT NULL,
key_col text NOT NULL,
val_col text NULL
)
;


 -- xp_ap_ramps: x-plane airports ramps code 1300
drop table if exists xp_ap_ramps;

create table if not exists xp_ap_ramps
(
icao text NOT NULL,
ramp_lat real NOT NULL,
ramp_lon real NOT NULL,
ramp_heading_true NULL,
location_type text NULL, -- gate”, “hangar”, “misc” or “tie-down”
for_planes text NULL,
ramp_uq_name text NULL
)
;

 -- xp_rw: x-plane runways codes 100,101
drop table if exists xp_rw;

create table if not exists xp_rw
(
icao text NOT NULL,
rw_width   real NULL,
rw_surf    integer NULL,
rw_sholder integer NULL,
rw_smooth  real NULL,
rw_no_1    text NOT NULL,
rw_no_1_lat real not null,
rw_no_1_lon real not null,
rw_no_1_disp_hold real null,
rw_no_2    text NOT NULL,
rw_no_2_lat real not null,
rw_no_2_lon real not null,
rw_no_2_disp_hold real null
);


drop table if exists xp_helipads;

create table if not exists xp_helipads
(
icao text NOT NULL,
name text NULL,
lat    real NOT NULL,
lon    real NOT NULL,
length real NULL,
width  real NULL
);

drop table if exists xp_loc;

create table if not exists xp_loc 
( 
lat real NOT NULL, 
lon real NOT NULL, 
ap_elev_ft integer, 
frq_mhz integer, 
max_reception_range integer, 
loc_bearing real, 
ident text, 
icao text, 
icao_region_code text, 
loc_rw text, 
loc_type text );

select *
from xp_loc t1
where t1.icao = 'LLBG'
;