#include <nms_common.h>
#include <nms_agent.h>
#include <netxms-regex.h>

#include "main.h"
#include "net.h"

LONG H_CheckHTTP(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	char szHost[1024];
	TCHAR szPort[1024];
	char szURI[1024];
	char szHeader[1024];
	char szMatch[1024];
	TCHAR szTimeout[64];
	unsigned short nPort;

	AgentGetParameterArgA(pszParam, 1, szHost, sizeof(szHost));
	AgentGetParameterArg(pszParam, 2, szPort, sizeof(szPort));
	AgentGetParameterArgA(pszParam, 3, szURI, sizeof(szURI));
	AgentGetParameterArgA(pszParam, 4, szHeader, sizeof(szHeader));
	AgentGetParameterArgA(pszParam, 5, szMatch, sizeof(szMatch));
   AgentGetParameterArg(pszParam, 6, szTimeout, sizeof(szTimeout));

	if (szHost[0] == 0 || szPort[0] == 0 || szURI[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nPort = (unsigned short)_tcstoul(szPort, NULL, 10);
	if (nPort == 0)
	{
		nPort = 80;
	}

	UINT32 dwTimeout = _tcstoul(szTimeout, NULL, 0);
	ret_int(pValue, CheckHTTP(szHost, 0, nPort, szURI, szHeader, szMatch, dwTimeout));
	return nRet;
}

int CheckHTTP(char *szAddr, UINT32 dwAddr, short nPort, char *szURI,
		char *szHost, char *szMatch, UINT32 dwTimeout)
{
	int nBytes, nRet = 0;
	SOCKET nSd;
	regex_t preg;

	if (szMatch[0] == 0)
	{
		strcpy(szMatch, "^HTTP/1.[01] 200 .*");
	}

	if (tre_regcomp(&preg, szMatch, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
	{
		return PC_ERR_BAD_PARAMS;
	}

	nSd = NetConnectTCP(szAddr, dwAddr, nPort, dwTimeout);
	if (nSd != INVALID_SOCKET)
	{
		char szTmp[4096];
		char szHostHeader[4096];

		nRet = PC_ERR_HANDSHAKE;

		snprintf(szHostHeader, sizeof(szHostHeader), "Host: %s:%u\r\n",
				szHost[0] != 0 ? szHost : szAddr, nPort);

		snprintf(szTmp, sizeof(szTmp),
				"GET %s HTTP/1.1\r\nConnection: close\r\nAccept: */*\r\n%s\r\n",
				szURI, szHostHeader);

		if (NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0)
		{
#define READ_TIMEOUT 5000
#define CHUNK_SIZE 10240
			char *buff = (char *)malloc(CHUNK_SIZE);
			int offset = 0;
			int buffSize = CHUNK_SIZE;

			while(NetCanRead(nSd, READ_TIMEOUT))
			{
				nBytes = NetRead(nSd, buff + offset, buffSize - offset);
				if (nBytes > 0) {
					offset += nBytes;
					if (buffSize - offset < (CHUNK_SIZE / 2)) {
						char *tmp = (char *)realloc(buff, buffSize + CHUNK_SIZE);
						if (tmp != NULL) {
							buffSize += CHUNK_SIZE;
							buff = tmp;
						}
						else {
							safe_free(buff);
							buffSize = 0;
						}
					}
				}
				else {
					break;
				}
			}

			if (buff != NULL && offset > 0) {
				buff[offset] = 0;

				if (tre_regexec(&preg, buff, 0, NULL, 0) == 0)
				{
					nRet = PC_ERR_NONE;
				}

				safe_free(buff);
			}
		}
		NetClose(nSd);
	}
	else
	{
		nRet = PC_ERR_CONNECT;
	}

	regfree(&preg);

	return nRet;
}
