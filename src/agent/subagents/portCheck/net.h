/* $Id: net.h,v 1.1.1.1 2005-01-18 18:38:54 alk Exp $ */

#ifndef __NET__H__
#define __NET__H__

enum
{
	PROTOCOL_UDP,
	PROTOCOL_TCP
};

int NetConnectTCP(char *, unsigned short);
int NetRead(int, char *, int);
int NetWrite(int, char *, int);
void NetClose(int);

#endif // __NET__H__

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
