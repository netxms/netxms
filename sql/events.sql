--
-- System-defined events
--

INSERT INTO events (id,severity,flags,message,description) VALUES
	(
		1, 0, 1,
		'Node added',
		'Generated when new node object added to the database.\n'
		'Parameters:\n'
		'   No message-specific parameters'
	);
INSERT INTO events (id,severity,flags,message,description) VALUES
	(
		2, 0, 1,
		'Subnet added',
		'Generated when new subnet object added to the database.\n'
		'Parameters:\n'
		'   No message-specific parameters'
	);
INSERT INTO events (id,severity,flags,message,description) VALUES
	(
		3, 0, 1,
		'Interface %2 added (IP Addr: %3/%4, IfIndex: %5)',
		'Generated when new interface object added to the database.\n'
		'Please note that source of event is node, not an interface itself.\n'
		'Parameters:\n'
		'   1) Interface object ID\n'
		'   2) Interface name\n'
		'   3) Interface IP address\n'
		'   4) Interface netmask\n'
		'   5) Interface index'
	);
INSERT INTO events (id,severity,flags,message,description) VALUES
	(
		4, 0, 1,
		'Interface %2 changed state to UP (IP Addr: %3/%4, IfIndex: %5)',
		'Generated when interface goes up.\n'
		'Please note that source of event is node, not an interface itself.\n'
		'Parameters:\n'
		'   1) Interface object ID\n'
		'   2) Interface name\n'
		'   3) Interface IP address\n'
		'   4) Interface netmask\n'
		'   5) Interface index'
	);
INSERT INTO events (id,severity,flags,message,description) VALUES
	(
		5, 2, 1,
		'Interface %2 changed state to DOWN (IP Addr: %3/%4, IfIndex: %5)',
		'Generated when interface goes down.\n'
		'Please note that source of event is node, not an interface itself.\n'
		'Parameters:\n'
		'   1) Interface object ID\n'
		'   2) Interface name\n'
		'   3) Interface IP address\n'
		'   4) Interface netmask\n'
		'   5) Interface index'
	);
INSERT INTO events (id,severity,flags,message,description) VALUES
	(
		6, 0, 1,
		'Node status changed to NORMAL',
		'Generated when node status changed to normal.\n'
		'Parameters:\n'
		'   1) Previous node status'
	);
INSERT INTO events (id,severity,flags,message,description) VALUES
	(
		7, 0, 1,
		'Node status changed to INFO',
		'Generated when node status changed to informational.\n'
		'Parameters:\n'
		'   1) Previous node status'
	);
INSERT INTO events (id,severity,flags,message,description) VALUES
	(
		8, 0, 1,
		'Node status changed to WARNING',
		'Generated when node status changed to warning.\n'
		'Parameters:\n'
		'   1) Previous node status'
	);
INSERT INTO events (id,severity,flags,message,description) VALUES
	(
		9, 0, 1,
		'Node status changed to ERROR',
		'Generated when node status changed to error.\n'
		'Parameters:\n'
		'   1) Previous node status'
	);
INSERT INTO events (id,severity,flags,message,description) VALUES
	(
		10, 0, 1,
		'Node status changed to CRITICAL',
		'Generated when node status changed to critical.\n'
		'Parameters:\n'
		'   1) Previous node status'
	);
INSERT INTO events (id,severity,flags,message,description) VALUES
	(
		11, 0, 1,
		'Node status changed to UNKNOWN',
		'Generated when node status changed to unknown.\n'
		'Parameters:\n'
		'   1) Previous node status'
	);
INSERT INTO events (id,severity,flags,message,description) VALUES
	(
		12, 0, 1,
		'Node status changed to UNMANAGED',
		'Generated when node status changed to unmanaged.\n'
		'Parameters:\n'
		'   1) Previous node status'
	);
