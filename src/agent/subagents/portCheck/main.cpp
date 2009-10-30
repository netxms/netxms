/* $Id$ */

#define LIBNXCL_NO_DECLARATIONS

#include <nms_common.h>
#include <nms_agent.h>
#include <nxclapi.h>

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
BOOL CommandHandler(DWORD dwCommand, CSCPMessage *pRequest, CSCPMessage *pResponse, void *session)
{
	BOOL bHandled = TRUE;
	WORD wType, wPort;
	DWORD dwAddress;
	char szRequest[1024 * 10];
	char szResponse[1024 * 10];
	DWORD nRet;

	if (dwCommand != CMD_CHECK_NETWORK_SERVICE)
	{
		return FALSE;
	}

	wType = pRequest->GetVariableShort(VID_SERVICE_TYPE);
	wPort = pRequest->GetVariableShort(VID_IP_PORT);
	dwAddress = pRequest->GetVariableLong(VID_IP_ADDRESS);
	pRequest->GetVariableStr(VID_SERVICE_REQUEST, szRequest, sizeof(szRequest));
	pRequest->GetVariableStr(VID_SERVICE_RESPONSE, szResponse, sizeof(szResponse));

	switch(wType)
	{
		case NETSRV_CUSTOM:
			// unsupported for now
			nRet = CheckCustom(NULL, dwAddress, wPort, szRequest, szResponse);
			pResponse->SetVariable(VID_RCC, ERR_SUCCESS);
			pResponse->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
			break;
		case NETSRV_SSH:
			nRet = CheckSSH(NULL, dwAddress, wPort, NULL, NULL);

			pResponse->SetVariable(VID_RCC, ERR_SUCCESS);
			pResponse->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
			break;
		case NETSRV_TELNET:
			nRet = CheckTelnet(NULL, dwAddress, wPort, NULL, NULL);

			pResponse->SetVariable(VID_RCC, ERR_SUCCESS);
			pResponse->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
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

				pResponse->SetVariable(VID_RCC, ERR_SUCCESS);
				pResponse->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
			}
			break;
		case NETSRV_SMTP:
			nRet = PC_ERR_BAD_PARAMS;

			if (szRequest[0] != 0)
			{
				nRet = CheckSMTP(NULL, dwAddress, wPort, szRequest);
				pResponse->SetVariable(VID_RCC, ERR_SUCCESS);
				pResponse->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
			}

			pResponse->SetVariable(VID_RCC, ERR_SUCCESS);
			pResponse->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
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
							szResponse);
				}

				pResponse->SetVariable(VID_RCC, ERR_SUCCESS);
				pResponse->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
			}
			break;
		default:
			bHandled = FALSE;
			break;
	}

	return bHandled;
}

//
// Init callback
//

DWORD m_dwTimeout = 30000;

static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
	{ _T("Timeout"), CT_LONG, 0, 0, 0, 0, &m_dwTimeout },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

static BOOL SubagentInit(Config *config)
{
	if (config->parseTemplate(_T("portCheck"), m_cfgTemplate))
	{
	}
}

//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "ServiceCheck.POP3(*)",         H_CheckPOP3,       NULL,
		DCI_DT_INT,		"" },
	{ "ServiceCheck.SMTP(*)",         H_CheckSMTP,       NULL,
		DCI_DT_INT,		"" },
	{ "ServiceCheck.SSH(*)",          H_CheckSSH,        NULL,
		DCI_DT_INT,		"" },
	{ "ServiceCheck.HTTP(*)",         H_CheckHTTP,       NULL,
		DCI_DT_INT,		"" },
	{ "ServiceCheck.Custom(*)",       H_CheckCustom,     NULL,
		DCI_DT_INT,		"" },
	{ "ServiceCheck.Telnet(*)",       H_CheckTelnet,     NULL,
		DCI_DT_INT,		"" },
};

/*
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
	{ "Net.ArpCache",                 H_NetArpCache,     NULL },
};
*/

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	"portCheck",
	NETXMS_VERSION_STRING,
	SubagentInit, NULL, // init and shutdown routines
	CommandHandler,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, //sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	NULL, //m_enums,
	0, NULL     // actions
};

//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(PORTCHECK)
{
	*ppInfo = &m_info;
	return TRUE;
}
