/* $Id: pop3.h,v 1.2 2005-01-28 02:50:32 alk Exp $ */

#ifndef __POP3__H__
#define __POP3__H__

LONG H_CheckPOP3(char *, char *, char *);
int CheckPOP3(char *, DWORD, short, char *, char *);

#endif // __POP3__H__

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.1.1.1  2005/01/18 18:38:54  alk
Initial import

implemented:
	ServiceCheck.POP3(host, user, password) - connect to host:110 and try to login


*/
