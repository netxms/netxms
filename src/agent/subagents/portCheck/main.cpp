/* $Id: main.cpp,v 1.9 2005-08-13 16:03:01 victor Exp $ */

#include <nms_common.h>
#include <nms_agent.h>
#include <nxclapi.h>
#include <nxcscpapi.h>

#ifdef _WIN32
#define PORTCHECK_EXPORTABLE __declspec(dllexport) __cdecl
#else
#define PORTCHECK_EXPORTABLE
#endif

#include "main.h"
#include "net.h"

//
// Command handler
//
BOOL CommandHandler(DWORD dwCommand, CSCPMessage *pRequest, CSCPMessage *pResponce)
{
	BOOL bHandled = TRUE;
	WORD wType, wPort;
	DWORD dwAddress;
	char szRequest[1024 * 10];
	char szResponce[1024 * 10];
	DWORD nRet;
	
	if (dwCommand != CMD_CHECK_NETWORK_SERVICE)
	{
		return FALSE;
	}

	wType = pRequest->GetVariableShort(VID_SERVICE_TYPE);
	wPort = pRequest->GetVariableShort(VID_IP_PORT);
	dwAddress = pRequest->GetVariableLong(VID_IP_ADDRESS);
	pRequest->GetVariableStr(VID_SERVICE_REQUEST, szRequest, sizeof(szRequest));
	pRequest->GetVariableStr(VID_SERVICE_RESPONCE, szResponce, sizeof(szResponce));

	switch(wType)
	{
	case NETSRV_CUSTOM:
		// unsupported for now
		nRet = CheckCustom(NULL, dwAddress, wPort, szRequest, szResponce);
		pResponce->SetVariable(VID_RCC, ERR_SUCCESS);
		pResponce->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
		break;
	case NETSRV_SSH:
			nRet = CheckSSH(NULL, dwAddress, wPort, NULL, NULL);

			pResponce->SetVariable(VID_RCC, ERR_SUCCESS);
			pResponce->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
		break;
	case NETSRV_POP3:
		{
			char *pUser, *pPass;
			nRet = PC_ERR_BAD_PARAMS;

			pUser = szRequest;
			pPass = strchr(szRequest, ':');
			if (pPass != NULL)
			{
				*pPass = 0;
				pPass++;

				nRet = CheckPOP3(NULL, dwAddress, wPort, pUser, pPass);

			}

			pResponce->SetVariable(VID_RCC, ERR_SUCCESS);
			pResponce->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
		}
		break;
	case NETSRV_SMTP:
		nRet = PC_ERR_BAD_PARAMS;

		if (szRequest[0] != 0)
		{
			nRet = CheckSMTP(NULL, dwAddress, wPort, szRequest);
			pResponce->SetVariable(VID_RCC, ERR_SUCCESS);
			pResponce->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
		}

		pResponce->SetVariable(VID_RCC, ERR_SUCCESS);
		pResponce->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
		break;
	case NETSRV_FTP:
		bHandled = FALSE;
		break;
	case NETSRV_HTTP:
		{
			char *pHost;
			char *pURI;

			nRet = PC_ERR_BAD_PARAMS;

			pHost = szRequest;
			pURI = strchr(szRequest, ':');
			if (pURI != NULL)
			{
				*pURI = 0;
				pURI++;

				nRet = CheckHTTP(NULL, dwAddress, wPort, pURI, pHost,
						szResponce);
			}

			pResponce->SetVariable(VID_RCC, ERR_SUCCESS);
			pResponce->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
		}
		break;
	default:
		bHandled = FALSE;
		break;
	}

	return bHandled;
}

//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { "ServiceCheck.POP3(*)",         H_CheckPOP3,       NULL },
   { "ServiceCheck.SSH(*)",          H_CheckSSH,        NULL },
};

/*static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   //{ "Net.ArpCache",                 H_NetArpCache,     NULL },
};*/

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	"portCheck",
	NETXMS_VERSION_STRING,
	NULL, // unload handler
	&CommandHandler,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, //sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	NULL, //m_enums,
   0, NULL     // actions
};

//
// Entry point for NetXMS agent
//

extern "C" BOOL PORTCHECK_EXPORTABLE NxSubAgentInit(NETXMS_SUBAGENT_INFO **ppInfo)
{
   *ppInfo = &m_info;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.8  2005/02/08 18:32:56  alk
+ simple "custom" checker added

Revision 1.7  2005/01/29 00:21:29  alk
+ http checker

request string: "HOST:URI"
responce string: posix regex, e.g. '^HTTP/1.[01] 200 .*'

requst sent to server:
---
GET URI HTTP/1.1\r\n
Connection: close\r\n
Host: HOST\r\n\r\n
---

Revision 1.6  2005/01/28 23:45:01  alk
SMTP check added, requst string == rcpt to

Revision 1.5  2005/01/28 23:19:35  alk
VID_SERVICE_STATUS set

Revision 1.4  2005/01/28 12:56:46  victor
Make it compile on WIndows

Revision 1.3  2005/01/28 02:50:32  alk
added support for CMD_CHECK_NETWORK_SERVICE
suported:
	ssh: host/port req.
	pop3: host/port/request string req. request string format: "login:password"

Revision 1.2  2005/01/19 13:42:47  alk
+ ServiceCheck.SSH(host[, port]) Added

Revision 1.1.1.1  2005/01/18 18:38:54  alk
Initial import

implemented:
	ServiceCheck.POP3(host, user, password) - connect to host:110 and try to login


*/
