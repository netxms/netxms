--
-- System configuration table
--

CREATE TABLE Config
(
	name varchar(64) not null,
	value varchar(255),
	PRIMARY KEY(name)
);


--
-- Users
--

CREATE TABLE Users
(
	id integer not null,
	name varchar(64) not null,
	password varchar(64),
	PRIMARY KEY(id)
);


--
-- Nodes to be added
--

CREATE TABLE NewNodes
(
	id integer not null,
	ip_addr integer not null,
	ip_netmask integer not null,
	discovery_flags integer not null
);


--
-- Nodes information
--

CREATE TABLE Nodes
(
	id integer not null,
	name varchar(64),
	status integer,
	is_deleted integer not null,
	primary_ip integer,
	is_snmp integer,
	is_agent integer,
	is_bridge integer,
	is_router integer,
	is_local_mgmt integer,
	snmp_version integer,
	community varchar(32),
	snmp_oid varchar(255),
	discovery_flags integer,
	auth_method integer,
	secret varchar(64),
	agent_port integer,
	status_poll_type integer,
	PRIMARY KEY(id)
);


--
-- Subnets
--

CREATE TABLE Subnets
(
	id integer not null,
	name varchar(64),
	status integer,
	is_deleted integer not null,
	ip_addr integer,
	ip_netmask integer,
	PRIMARY KEY(id)
);


--
-- Nodes' interfaces
--

CREATE TABLE Interfaces
(
	id integer not null,
	name varchar(64),
	status integer,
        is_deleted integer,
	node_id integer not null,
	ip_addr integer,
	ip_netmask integer,
	if_type integer,
	if_index integer,
	PRIMARY KEY(id),
	KEY(node_id)
);


--
-- Nodes to subnets mapping
--

CREATE TABLE nsmap
(
	subnet_id integer not null,
	node_id integer not null,
	KEY (subnet_id)
);


--
-- Data collection items
--

CREATE TABLE Items
(
	id integer not null,
	node_id integer not null,
	name varchar(255),
	description varchar(255),
	source integer,			-- 0 for internal or 1 for native agent or 2 for SNMP
	datatype integer,
	polling_interval integer,
	retention_time integer,
	PRIMARY KEY(id),
	KEY(node_id)
);


--
-- Events configuration
--

CREATE TABLE Events
(
	id integer not null,
	severity integer,
	flags integer,
	message varchar(255),		-- Message template
	description blob,
	PRIMARY KEY(id)
);


--
-- Event log
--

CREATE TABLE EventLog
(
	event_id integer,
	timestamp integer,
	source integer,			-- Source object ID
	severity integer,
	message varchar(255),
	KEY(event_id),
	KEY(timestamp)
);


--
-- Actions on events
--

CREATE TABLE ACTIONS
(
	id integer not null,
	type integer,
	command varchar(255),
	PRIMARY KEY(id)
);


--
-- Node groups
--

CREATE TABLE NodeGroups
(
	id integer not null,
	name varchar(255),
	PRIMARY KEY(id)
);


--
-- Event groups
--

CREATE TABLE EventGroups
(
	id integer not null,
	name varchar(255),
	PRIMARY KEY(id)
);


--
-- Node group members
--

CREATE TABLE NodeGroupMembers
(
	group_id integer not null,
	node_id integer not null,
	KEY(group_id),
	KEY(node_id)
);


--
-- Event group members
--

CREATE TABLE EventGroupMembers
(
	group_id integer not null,
	event_id integer not null,
	KEY(group_id),
	KEY(event_id)
);


--
-- Event processing policy
--

CREATE TABLE EventPolicy
(
	id integer not null,	-- Rule number
	comments varchar(255),
	PRIMARY KEY(id)
);

CREATE TABLE PolicySourceList
(
	rule_id integer not null,
	source_type integer not null,
	object_id integer not null,
	KEY(rule_id)
);

CREATE TABLE PolicyEventList
(
	rule_id integer not null,
	is_group integer not null,
	event_id integer not null,
	KEY(rule_id)
);

