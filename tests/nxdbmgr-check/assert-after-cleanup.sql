/* State after the final clean run. IDs 201-299. */

/* 201: baseline graph still intact */
INSERT INTO nxdbmgr_check_assert (id) SELECT CASE WHEN (SELECT count(*) FROM container_members WHERE container_id=990001 AND object_id=990002)=1 AND (SELECT count(*) FROM nodes WHERE id=990002)=1 THEN 201 ELSE 0 END FROM nxdbmgr_check_assert WHERE id=0;

/* 202: task 3 baseline objects intact after cleanup */
INSERT INTO nxdbmgr_check_assert (id) SELECT CASE WHEN (SELECT count(*) FROM nodes WHERE id=990008 AND zone_guid=0)=1 AND (SELECT count(*) FROM access_points WHERE id=990003)=1 AND (SELECT count(*) FROM business_services WHERE id=990020)=1 AND (SELECT count(*) FROM conditions WHERE id=990012)=1 AND (SELECT count(*) FROM dashboards WHERE id=990014)=1 AND (SELECT count(*) FROM dc_tables WHERE item_id=990011)=1 THEN 202 ELSE 0 END FROM nxdbmgr_check_assert WHERE id=0;

/* 203: task 4 baseline rule intact after cleanup */
INSERT INTO nxdbmgr_check_assert (id) SELECT CASE WHEN (SELECT count(*) FROM event_policy WHERE chain_id=0 AND rule_id=990010)=1 AND (SELECT count(*) FROM policy_source_list WHERE rule_id=990010)=1 THEN 203 ELSE 0 END FROM nxdbmgr_check_assert WHERE id=0;

/* 204: task 5 sensor kept its lost properties, duplicates and ghost gone */
INSERT INTO nxdbmgr_check_assert (id) SELECT CASE WHEN (SELECT count(*) FROM sensors WHERE id=999503)=1 AND (SELECT count(*) FROM object_properties WHERE object_id=999503)=1 AND (SELECT count(*) FROM object_properties WHERE object_id IN (999500,999501,999502))=0 THEN 204 ELSE 0 END FROM nxdbmgr_check_assert WHERE id=0;

/* 205: task 6 negative subnets and zone intact, injected objects gone */
INSERT INTO nxdbmgr_check_assert (id) SELECT CASE WHEN (SELECT count(*) FROM subnets WHERE id IN (990030,990031))=2 AND (SELECT count(*) FROM zones WHERE id=990032)=1 AND (SELECT count(*) FROM object_properties WHERE object_id IN (999600,999601,999610,999611,999620,999621))=0 THEN 205 ELSE 0 END FROM nxdbmgr_check_assert WHERE id=0;

/* 206: task 7 valid bindings survive the clean run */
INSERT INTO nxdbmgr_check_assert (id) SELECT CASE WHEN (SELECT count(*) FROM items WHERE item_id IN (990043,990045,990046,990047,990048) AND template_id<>0)=5 AND (SELECT count(*) FROM items WHERE item_id IN (999701,999703,999704,999707))=0 THEN 206 ELSE 0 END FROM nxdbmgr_check_assert WHERE id=0;

/* 207: task 8 valid peers survive the clean run */
INSERT INTO nxdbmgr_check_assert (id) SELECT CASE WHEN (SELECT count(*) FROM interfaces WHERE id=990050 AND peer_node_id=990003)=1 AND (SELECT count(*) FROM nodes WHERE id=990002 AND path_check_node_id=990008)=1 THEN 207 ELSE 0 END FROM nxdbmgr_check_assert WHERE id=0;

/* 208: task 11 valid references survive the clean run */
INSERT INTO nxdbmgr_check_assert (id) SELECT CASE WHEN (SELECT count(*) FROM policy_source_list WHERE rule_id IN (990070,990072,990076,990079,990080) AND object_id=990002)=5 AND (SELECT count(*) FROM policy_event_list WHERE rule_id IN (990074,990082,990083) AND event_code=17)=3 THEN 208 ELSE 0 END FROM nxdbmgr_check_assert WHERE id=0;
/* 209: built-in references survive the clean run */
INSERT INTO nxdbmgr_check_assert (id) SELECT CASE WHEN (SELECT count(*) FROM acl WHERE object_id IN (3,4) AND user_id=1)=2 AND (SELECT count(*) FROM container_members WHERE container_id=3 AND object_id=990041)=1 AND (SELECT count(*) FROM policy_source_list WHERE object_id IN (1,3,4))=3 THEN 209 ELSE 0 END FROM nxdbmgr_check_assert WHERE id=0;
