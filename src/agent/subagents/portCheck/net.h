/* $Id: net.h,v 1.3 2006-03-15 12:00:10 alk Exp $ */

#ifndef __NET__H__
#define __NET__H__

enum
{
	PROTOCOL_UDP,
	PROTOCOL_TCP
};

int NetConnectTCP(char *, DWORD, unsigned short);
bool NetCanRead(int, int);
int NetRead(int, char *, int);
int NetWrite(int, char *, int);
void NetClose(int);

#endif // __NET__H__

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.2  2005/01/28 02:50:32  alk
added support for CMD_CHECK_NETWORK_SERVICE
suported:
	ssh: host/port req.
	pop3: host/port/request string req. request string format: "login:password"

Revision 1.1.1.1  2005/01/18 18:38:54  alk
Initial import

implemented:
	ServiceCheck.POP3(host, user, password) - connect to host:110 and try to login


*/
