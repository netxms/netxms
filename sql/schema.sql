--
-- System configuration table
--

create table CONFIG
(
	name varchar(64),
	value varchar(255)
);


--
-- Nodes information
--

create table NODES
(
	id integer autoincrement,
	name varchar(64),
	status integer,
	primary_ip integer,
	isSNMP bool,
	isNativeAgent bool,
	isRouter bool,
	isBridge bool
);


--
-- Subnets
--

create table SUBNETS
(
	id integer autoincrement,
	name varchar(64),
	status integer,
	ip_addr integer,
	ip_netmask integer
);


--
-- Nodes' interfaces
--

create table INTERFACES
(
	id integer autoincrement,
	node_id integer not null,
	name varchar(64),
	ip_addr integer,
	ip_netmask integer,
	status integer,
	type integer
);


--
-- Nodes to subnets mapping
--

create table NSMAP
(
	node_id integer,
	subnet_id integer
);


--
-- Data collection items
--

create table ITEMS
(
	id integer autoincrement,
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
	val_float ??,
	val_string varchar(255)
);


--
-- Events configuration
--

create table EVENTS
(
	id integer autoincrement,
	severity integer,
	message varchar(255),		-- Message template
	retention_time integer,
	description blob
);


--
-- Event log
--

create table ELOG
(
	event_id integer,
	timestamp integer,
	source integer,			-- Source node ID
	severity integer,
	isProcessed bool,
	message varchar(255)
);


--
-- Actions on events
--

create table ACTIONS
(
	id integer autoincrement,
	type integer,
	command varchar(255)
);


--
-- Node templates
--
	