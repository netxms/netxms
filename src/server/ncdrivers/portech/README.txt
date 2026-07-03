ABOUT

This driver can be used to send SMS via Portech MV-37x VoIP GSM gateways.
It was tested on MV-372, but should work on other models as well.


CONFIGURATION

Configuration string for the driver should be in form

option=value;option=value;...

Valid options are:

host - host address or DNS name.
login - login name. Default is "admin".
password - password. Default is "admin".
mode - SMS sending mode. Vaid values are PDU or TEXT. Default is PDU.


EXAMPLE

To configure driver for working with device at address 10.0.0.1, login admin,
password test, set server configuration variable SMSDrvConfig to

host=10.0.0.1;login=admin;password=test;mode=PDU
