/* Removes report-only defects so that the final check is clean. */
UPDATE nxdbmgr_check_assert SET id=0 WHERE id=0;

/* Task 3: report-only references */
UPDATE nodes SET zone_guid=0 WHERE id=990008;
UPDATE subnets SET zone_guid=0 WHERE id=990007;
DELETE FROM dci_delete_list WHERE node_id=999232;
DELETE FROM business_service_checks WHERE id IN (999233,999234,999235);
DELETE FROM cond_dci_map WHERE condition_id=990012;
DELETE FROM scheduled_tasks WHERE id=999238;
DELETE FROM dashboard_template_instances WHERE instance_object_id=999239;

/* Task 5: ghost properties and duplicate IDs are report only */
DELETE FROM object_properties WHERE object_id IN (999500,999501,999502);
DELETE FROM nodes WHERE id IN (999501,999502);
DELETE FROM object_containers WHERE id IN (999501,999502);
DELETE FROM container_members WHERE object_id IN (999501,999502);
/* the first forced run created data tables for the duplicate node rows, drop them with the nodes */
DROP TABLE idata_999501;
DROP TABLE tdata_999501;
DROP TABLE idata_999502;
DROP TABLE tdata_999502;

/* Task 6: cycle, shared GUID and duplicate subnets are report only */
DELETE FROM container_members WHERE container_id IN (999600,999601);
DELETE FROM object_containers WHERE id IN (999600,999601,999610,999611);
DELETE FROM subnets WHERE id IN (999620,999621);
DELETE FROM object_properties WHERE object_id IN (999600,999601,999610,999611,999620,999621);

/* Task 7: report-only bindings */
DELETE FROM items WHERE item_id IN (999701,999703,999704,999707);

/* Task 6 follow-up: overlapping cycles, self-membership, diamond */
DELETE FROM container_members WHERE container_id IN (999630,999631,999632,999640,999650,999651,999652);
DELETE FROM object_containers WHERE id IN (999630,999631,999632,999640,999650,999651,999652,999653);
DELETE FROM object_properties WHERE object_id IN (999630,999631,999632,999640,999650,999651,999652,999653);

/* Task 10: channel references are report only */
DELETE FROM actions WHERE action_id=999910;

/* Task 9: restore default event 35 and the condition that could not be repaired without it */
INSERT INTO event_cfg (event_code,event_name,guid,severity,flags,message,description,tags) VALUES (35,'SYS_CONDITION_DEACTIVATED','926d15d2-9761-4bb6-a1ce-64175303796f',0,1,'Condition "%2" deactivated','Default event for condition deactivation.',NULL);
UPDATE conditions SET deactivation_event=35 WHERE id=990012;

/* Task 11: last-entry references are report only */
DELETE FROM policy_source_list WHERE object_id IN (999951,999953,999959,999960);
DELETE FROM policy_event_list WHERE event_code IN (999955,999961);
DELETE FROM policy_event_list WHERE event_code=999963;
DELETE FROM policy_source_list WHERE object_id=999967;
