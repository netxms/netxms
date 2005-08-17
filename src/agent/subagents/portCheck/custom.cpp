/* $Id: custom.cpp,v 1.2 2005-08-17 12:09:23 victor Exp $ */

#include <nms_common.h>
#include <nms_agent.h>

#include "main.h"
#include "net.h"

int CheckCustom(char *szAddr, DWORD dwAddr, short nPort, char *szRequest,
		char *szResponse)
{
	int nRet = 0;
	int nSd;

	nSd = NetConnectTCP(szAddr, dwAddr, nPort);
	if (nSd > 0)
	{
		nRet = PC_ERR_NONE;

		NetClose(nSd);
	}
	else
	{
		nRet = PC_ERR_CONNECT;
	}

	return nRet;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.1  2005/02/08 18:32:56  alk
+ simple "custom" checker added


*/
