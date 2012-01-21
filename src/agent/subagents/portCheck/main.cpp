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
// Global variables
//

TCHAR g_szDomainName[128] = _T("netxms.org");


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
			nRet = CheckCustom(NULL, dwAddress, wPort, szRequest, szResponse, 0);
			pResponse->SetVariable(VID_RCC, ERR_SUCCESS);
			pResponse->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
			break;
		case NETSRV_SSH:
			nRet = CheckSSH(NULL, dwAddress, wPort, NULL, NULL, 0);

			pResponse->SetVariable(VID_RCC, ERR_SUCCESS);
			pResponse->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
			break;
		case NETSRV_TELNET:
			nRet = CheckTelnet(NULL, dwAddress, wPort, NULL, NULL, 0);

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

					nRet = CheckPOP3(NULL, dwAddress, wPort, pUser, pPass, 0);

				}

				pResponse->SetVariable(VID_RCC, ERR_SUCCESS);
				pResponse->SetVariable(VID_SERVICE_STATUS, (DWORD)nRet);
			}
			break;
		case NETSRV_SMTP:
			nRet = PC_ERR_BAD_PARAMS;

			if (szRequest[0] != 0)
			{
				nRet = CheckSMTP(NULL, dwAddress, wPort, szRequest, 0);
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
							szResponse, 0);
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

DWORD m_dwDefaultTimeout = 3000;

static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
	{ _T("DomainName"), CT_STRING, 0, 0, 128, 0, g_szDomainName },
	{ _T("Timeout"), CT_LONG, 0, 0, 0, 0, &m_dwDefaultTimeout },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

static BOOL SubagentInit(Config *config)
{
	config->parseTemplate(_T("portCheck"), m_cfgTemplate);
	return TRUE;
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "ServiceCheck.POP3(*)",         H_CheckPOP3,       NULL,
		DCI_DT_INT,		"Status of remote POP3 service" },
	{ "ServiceCheck.SMTP(*)",         H_CheckSMTP,       NULL,
		DCI_DT_INT,		"Status of remote SMTP service" },
	{ "ServiceCheck.SSH(*)",          H_CheckSSH,        NULL,
		DCI_DT_INT,		"Status of remote SSH service" },
	{ "ServiceCheck.HTTP(*)",         H_CheckHTTP,       NULL,
		DCI_DT_INT,		"Status of remote HTTP service" },
	{ "ServiceCheck.Custom(*)",       H_CheckCustom,     NULL,
		DCI_DT_INT,		"Status of remote TCP service" },
	{ "ServiceCheck.Telnet(*)",       H_CheckTelnet,     NULL,
		DCI_DT_INT,		"Status of remote TELNET service" },
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	"portCheck",
	NETXMS_VERSION_STRING,
	SubagentInit, NULL, // init and shutdown routines
	CommandHandler,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, NULL,	// lists
	0, NULL,	// tables
   0, NULL,	// actions
	0, NULL	// push parameters
};


//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(PORTCHECK)
{
	*ppInfo = &m_info;
	return TRUE;
}
