/* $Id: main.cpp,v 1.3 2005-01-28 02:50:32 alk Exp $ */

#include <nms_common.h>
#include <nms_agent.h>
#include <nxclapi.h>
#include <nxcscpapi.h>


#include "main.h"
#include "net.h"
#include "pop3.h"
#include "ssh.h"

//
// Command handler
//
BOOL CommandHandler(DWORD dwCommand, CSCPMessage *pRequest, CSCPMessage *pResponce)
{
	BOOL bHandled = TRUE;
	WORD wType, wPort, wIpProto;
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
		bHandled = FALSE;
		break;
	case NETSRV_SSH:
			nRet = CheckSSH(NULL, dwAddress, wPort, NULL, NULL);

			pResponce->SetVariable(VID_RCC, nRet);
		break;
	case NETSRV_POP3:
		{
			char *pUser, *pPass;
			nRet = PC_ERR_PARAM;

			pUser = szRequest;
			pPass = strchr(szRequest, ':');
			if (pPass != NULL)
			{
				*pPass = 0;
				pPass++;

				nRet = CheckPOP3(NULL, dwAddress, wPort, pUser, pPass);
			}

			pResponce->SetVariable(VID_RCC, nRet);
		}
		break;
	case NETSRV_SMTP:
		bHandled = FALSE;
		break;
	case NETSRV_FTP:
		bHandled = FALSE;
		break;
	case NETSRV_HTTP:
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

static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   //{ "Net.ArpCache",                 H_NetArpCache,     NULL },
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	"portCheck",
	NETXMS_VERSION_STRING,
	NULL, // unload handler
	&CommandHandler,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums
};

//
// Entry point for NetXMS agent
//

extern "C" BOOL NxSubAgentInit(NETXMS_SUBAGENT_INFO **ppInfo)
{
   *ppInfo = &m_info;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.2  2005/01/19 13:42:47  alk
+ ServiceCheck.SSH(host[, port]) Added

Revision 1.1.1.1  2005/01/18 18:38:54  alk
Initial import

implemented:
	ServiceCheck.POP3(host, user, password) - connect to host:110 and try to login


*/
