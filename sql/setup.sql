--
-- Default configuration parameters
--

INSERT INTO config (name,value) VALUES ('SyncInterval','60');
INSERT INTO config (name,value) VALUES ('NewNodePollingInterval','60');
INSERT INTO config (name,value) VALUES ('DiscoveryPollingInterval','900');
INSERT INTO config (name,value) VALUES ('StatusPollingInterval','30');
INSERT INTO config (name,value) VALUES ('ConfigurationPollingInterval','3600');
INSERT INTO config (name,value) VALUES ('ResolveNodeNames','0');
INSERT INTO config (name,value) VALUES ('NumberOfEventProcessors','1');
INSERT INTO config (name,value) VALUES ('ClientListenerPort','4701');
INSERT INTO config (name,value) VALUES ('NumberOfDataCollectors','10');
INSERT INTO config (name,value) VALUES ('IDataTableCreationCommand','CREATE TABLE idata_%d (item_id integer not null,timestamp integer,value varchar(255))');
INSERT INTO config (name,value) VALUES ('RunNetworkDiscovery','1');
INSERT INTO config (name,value) VALUES ('EnableAdminInterface','1');
INSERT INTO config (name,value) VALUES ('EnableAccessControl','1');
INSERT INTO config (name,value) VALUES ('EnableEventsAccessControl','0');


--
-- Default users
--

INSERT INTO users (id,name,password) VALUES (0,'admin','');
INSERT INTO users (id,name,password) VALUES (1,'guest','');
