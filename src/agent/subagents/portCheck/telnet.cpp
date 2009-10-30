/* $Id$ */

#include <nms_common.h>
#include <nms_agent.h>

#include "main.h"
#include "net.h"

LONG H_CheckTelnet(const char *pszParam, const char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	char szHost[256];
	char szPort[256];
	unsigned short nPort;

	AgentGetParameterArg(pszParam, 1, szHost, sizeof(szHost));
	AgentGetParameterArg(pszParam, 2, szPort, sizeof(szPort));

	if (szHost[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nPort = (unsigned short)atoi(szPort);
	if (nPort == 0)
	{
		nPort = 23;
	}

	ret_int(pValue, CheckTelnet(szHost, 0, nPort, NULL, NULL));
	return nRet;
}

int CheckTelnet(char *szAddr, DWORD dwAddr, short nPort, char *szUser, char *szPass)
{
	int nRet = 0;
	SOCKET nSd;

	nSd = NetConnectTCP(szAddr, dwAddr, nPort);
	if (nSd != INVALID_SOCKET)
	{
		unsigned char szBuff[512];

		nRet = PC_ERR_HANDSHAKE;
		while (NetCanRead(nSd, 1000) && nRet == PC_ERR_HANDSHAKE) // 1sec
		{
			int size = NetRead(nSd, (char *)szBuff, sizeof(szBuff));
			unsigned char out[4];

			memset(out, 0, sizeof(out));
			for (int i = 0; i < size; i++)
			{
				if (szBuff[i] == 0xFF) // IAC
				{
					out[0] = 0xFF;
					continue;
				}
				if (out[0] == 0xFF && (szBuff[i] == 251 || szBuff[i] == 252))
				{
					out[1] = 254;
					continue;
				}
				if (out[0] == 0xFF && (szBuff[i] == 253 || szBuff[i] == 254))
				{
					out[1] = 252;
					continue;
				}

				if (out[0] == 0xFF && out[1] != 0)
				{
					out[2] = szBuff[i];

					// send reject
					NetWrite(nSd, (char *)out, 3);

					memset(out, 0, sizeof(out));
					continue;
				}

				// end of handshake, get out from here
				nRet = PC_ERR_NONE;
				break;
			}
		}

		NetClose(nSd);
	}
	else
	{
		nRet = PC_ERR_CONNECT;
	}

	return nRet;
}
