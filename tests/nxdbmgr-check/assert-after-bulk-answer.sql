/* State after the interactive bulk answer run. IDs 301-399. */

/* 301: "All" to the link prompt linked node 990002 to subnet 990007, and the run was committed */
INSERT INTO nxdbmgr_check_assert (id) SELECT CASE WHEN (SELECT count(*) FROM nsmap WHERE subnet_id=990007 AND node_id=990002)=1 THEN 301 ELSE 0 END FROM nxdbmgr_check_assert WHERE id=0;

/* 302: "All" to the link prompt did not answer the delete prompt for node 999990 */
INSERT INTO nxdbmgr_check_assert (id) SELECT CASE WHEN (SELECT count(*) FROM nodes WHERE id=999990)=1 AND (SELECT count(*) FROM object_properties WHERE object_id=999990)=1 THEN 302 ELSE 0 END FROM nxdbmgr_check_assert WHERE id=0;
