/* $Id$ */

#include <nms_common.h>
#include <nms_agent.h>
#include <nxcldefs.h>
#include <nxcpapi.h>

#ifdef _WITH_ENCRYPTION
#include <openssl/ssl.h>
#endif

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

char g_szDomainName[128] = "netxms.org";
char g_szFailedDir[1024] = "";

/**
 * Command handler
 */
BOOL CommandHandler(UINT32 dwCommand, NXCPMessage *pRequest, NXCPMessage *pResponse, AbstractCommSession *session)
{
	BOOL bHandled = TRUE;
	WORD wType, wPort;
	UINT32 dwAddress;
	char szRequest[1024 * 10];
	char szResponse[1024 * 10];
	UINT32 nRet;

	if (dwCommand != CMD_CHECK_NETWORK_SERVICE)
	{
		return FALSE;
	}

	wType = pRequest->getFieldAsUInt16(VID_SERVICE_TYPE);
	wPort = pRequest->getFieldAsUInt16(VID_IP_PORT);
	dwAddress = pRequest->getFieldAsUInt32(VID_IP_ADDRESS);
	pRequest->getFieldAsMBString(VID_SERVICE_REQUEST, szRequest, sizeof(szRequest));
	pRequest->getFieldAsMBString(VID_SERVICE_RESPONSE, szResponse, sizeof(szResponse));

   INT64 start = GetCurrentTimeMs();

	switch(wType)
	{
		case NETSRV_CUSTOM:
			// unsupported for now
			nRet = CheckCustom(NULL, dwAddress, wPort, 0);
			pResponse->setField(VID_RCC, ERR_SUCCESS);
			pResponse->setField(VID_SERVICE_STATUS, (UINT32)nRet);
			break;
		case NETSRV_SSH:
			nRet = CheckSSH(NULL, dwAddress, wPort, NULL, NULL, 0);

			pResponse->setField(VID_RCC, ERR_SUCCESS);
			pResponse->setField(VID_SERVICE_STATUS, (UINT32)nRet);
			break;
		case NETSRV_TELNET:
			nRet = CheckTelnet(NULL, dwAddress, wPort, NULL, NULL, 0);

			pResponse->setField(VID_RCC, ERR_SUCCESS);
			pResponse->setField(VID_SERVICE_STATUS, (UINT32)nRet);
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

				pResponse->setField(VID_RCC, ERR_SUCCESS);
				pResponse->setField(VID_SERVICE_STATUS, (UINT32)nRet);
			}
			break;
		case NETSRV_SMTP:
			nRet = PC_ERR_BAD_PARAMS;

			if (szRequest[0] != 0)
			{
				nRet = CheckSMTP(NULL, dwAddress, wPort, szRequest, 0);
				pResponse->setField(VID_RCC, ERR_SUCCESS);
				pResponse->setField(VID_SERVICE_STATUS, (UINT32)nRet);
			}

			pResponse->setField(VID_RCC, ERR_SUCCESS);
			pResponse->setField(VID_SERVICE_STATUS, (UINT32)nRet);
			break;
		case NETSRV_FTP:
			bHandled = FALSE;
			break;
		case NETSRV_HTTP:
		case NETSRV_HTTPS:
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

               if (wType == NETSRV_HTTP)
               {
                  nRet = CheckHTTP(NULL, dwAddress, wPort, pURI, pHost, szResponse, 0);
               }
               else
               {
                  nRet = CheckHTTPS(NULL, dwAddress, wPort, pURI, pHost, szResponse, 0);
               }
				}

				pResponse->setField(VID_RCC, ERR_SUCCESS);
				pResponse->setField(VID_SERVICE_STATUS, (UINT32)nRet);
			}
			break;
		default:
			bHandled = FALSE;
			break;
	}

   if (bHandled)
   {
      INT64 elapsed = GetCurrentTimeMs() - start;
      pResponse->setField(VID_RESPONSE_TIME, (INT32)elapsed);
   }
	return bHandled;
}

/**
 * Default request timeout
 */
UINT32 m_dwDefaultTimeout = 3000;

/**
 * Configuration template
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
	{ _T("DomainName"), CT_MB_STRING, 0, 0, 128, 0, g_szDomainName },
	{ _T("Timeout"), CT_LONG, 0, 0, 0, 0, &m_dwDefaultTimeout },
	{ _T("FailedDirectory"), CT_MB_STRING, 0, 0, 1024, 0, &g_szFailedDir },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Subagent initialization callback
 */
static BOOL SubagentInit(Config *config)
{
	config->parseTemplate(_T("portCheck"), m_cfgTemplate);
#ifdef _WITH_ENCRYPTION
   SSL_library_init();
   SSL_load_error_strings();
#endif
	return TRUE;
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("ServiceCheck.Custom(*)"),        H_CheckCustom, _T("C"),	 DCI_DT_INT, _T("Status of remote TCP service") },
	{ _T("ServiceCheck.HTTP(*)"),          H_CheckHTTP,   _T("C"),  DCI_DT_INT, _T("Status of remote HTTP service") },
	{ _T("ServiceCheck.HTTPS(*)"),         H_CheckHTTP,   _T("CS"), DCI_DT_INT, _T("Status of remote HTTPS service") },
	{ _T("ServiceCheck.POP3(*)"),          H_CheckPOP3,   _T("C"),  DCI_DT_INT, _T("Status of remote POP3 service") },
	{ _T("ServiceCheck.SMTP(*)"),          H_CheckSMTP,   _T("C"),	 DCI_DT_INT, _T("Status of remote SMTP service") },
	{ _T("ServiceCheck.SSH(*)"),           H_CheckSSH,    _T("C"),	 DCI_DT_INT, _T("Status of remote SSH service") },
	{ _T("ServiceCheck.Telnet(*)"),        H_CheckTelnet, _T("C"),	 DCI_DT_INT, _T("Status of remote TELNET service") },
	{ _T("ServiceResponseTime.Custom(*)"), H_CheckCustom, _T("R"),	 DCI_DT_INT, _T("Response time of remote TCP service") },
	{ _T("ServiceResponseTime.HTTP(*)"),   H_CheckHTTP,   _T("R"),	 DCI_DT_INT, _T("Response time of remote HTTP service") },
	{ _T("ServiceResponseTime.HTTPS(*)"),  H_CheckHTTP,   _T("RS"), DCI_DT_INT, _T("Response time of remote HTTPS service") },
	{ _T("ServiceResponseTime.POP3(*)"),   H_CheckPOP3,   _T("R"),  DCI_DT_INT, _T("Response time of remote POP3 service") },
	{ _T("ServiceResponseTime.SMTP(*)"),   H_CheckSMTP,   _T("R"),  DCI_DT_INT, _T("Response time of remote SMTP service") },
	{ _T("ServiceResponseTime.SSH(*)"),    H_CheckSSH,    _T("R"),  DCI_DT_INT, _T("Response time of remote SSH service") },
	{ _T("ServiceResponseTime.Telnet(*)"), H_CheckTelnet, _T("R"),  DCI_DT_INT, _T("Response time of remote TELNET service") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("PORTCHECK"),
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

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(PORTCHECK)
{
	*ppInfo = &m_info;
	return TRUE;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
