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
		10, 0, 1,
		'TEST: Source name = %n  ID = %i  IP Address = %a  TimeStamp = %t',
		'Test message'
	);
