--
-- System configuration table
--

CREATE TABLE CONFIG
(
	name varchar(64) not null,
	value varchar(255),
	PRIMARY KEY(name)
);


--
-- Nodes information
--

CREATE TABLE NODES
(
	id integer not null,
	name varchar(64),
	status integer,
	primary_ip integer,
	isSNMP bool,
	isNativeAgent bool,
	isRouter bool,
	isBridge bool,
	PRIMARY KEY(id)
);


--
-- New nodes list
--

CREATE TABLE NEWNODES
(
	ip integer not null
);


--
-- Subnets
--

CREATE TABLE SUBNETS
(
	id integer not null,
	name varchar(64),
	status integer,
	ip_addr integer,
	ip_netmask integer,
	PRIMARY KEY(id)
);


--
-- Nodes' interfaces
--

create table INTERFACES
(
	id integer not null,
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

create table EVENTS
(
	id integer not null,
	severity integer,
	message varchar(255),		-- Message template
	retention_time integer,
	description blob
);


--
-- Event log
--

CREATE TABLE ELOG
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
	