--
-- Default configuration parameters
--

INSERT INTO config (name,value) VALUES ('SyncInterval','60');
INSERT INTO config (name,value) VALUES ('NewNodePollingInterval','60');
INSERT INTO config (name,value) VALUES ('DiscoveryPollingInterval','900');
INSERT INTO config (name,value) VALUES ('StatusPollingInterval','30');
INSERT INTO config (name,value) VALUES ('ResolveNodeNames','0');
INSERT INTO config (name,value) VALUES ('NumberOfEventProcessors','1');


--
-- Default users
--

INSERT INTO users (id,name,password) VALUES (0,'admin','');
INSERT INTO users (id,name,password) VALUES (1,'guest','');
