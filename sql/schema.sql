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

create table ITEMS
(
	id integer not null,
	node_id integer,
	name varchar(255),
	description varchar(255),
	datatype integer,
	polling_interval integer,
	retention_time integer
);


--
-- Collected data
--

create table IDATA
(
	item_id integer not null,
	timestamp integer,
	val_integer integer,
	val_string varchar(255)
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
-- Node templates
--
	