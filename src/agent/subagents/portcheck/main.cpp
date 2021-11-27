/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: main.cpp
**
**/

#include "portcheck.h"
#include <nxcldefs.h>
#include <netxms-version.h>

/**
 * Global variables
 */
char g_szDomainName[128] = "netxms.org";
char g_hostName[128] = "";
char g_szFailedDir[1024] = "";
uint32_t g_serviceCheckFlags = 0;

/**
 * Default request timeout
 */
uint32_t s_defaultTimeout = 3000;

/**
 * Connect to given host
 */
SOCKET NetConnectTCP(const char *hostname, const InetAddress& addr, uint16_t port, uint32_t dwTimeout)
{
   InetAddress hostAddr = (hostname != nullptr) ? InetAddress::resolveHostName(hostname) : addr;
   if (!hostAddr.isValidUnicast() && !hostAddr.isLoopback())
      return INVALID_SOCKET;

   return ConnectToHost(hostAddr, port, (dwTimeout != 0) ? dwTimeout : s_defaultTimeout);
}

/**
 * Command handler
 */
bool CommandHandler(UINT32 dwCommand, NXCPMessage *pRequest, NXCPMessage *pResponse, AbstractCommSession *session)
{
	bool bHandled = true;
	WORD wType, wPort;
	char szRequest[1024 * 10];
	char szResponse[1024 * 10];
	UINT32 nRet;

	if (dwCommand != CMD_CHECK_NETWORK_SERVICE)
	{
		return false;
	}

	wType = pRequest->getFieldAsUInt16(VID_SERVICE_TYPE);
	wPort = pRequest->getFieldAsUInt16(VID_IP_PORT);
	InetAddress addr = pRequest->getFieldAsInetAddress(VID_IP_ADDRESS);
	pRequest->getFieldAsMBString(VID_SERVICE_REQUEST, szRequest, sizeof(szRequest));
	pRequest->getFieldAsMBString(VID_SERVICE_RESPONSE, szResponse, sizeof(szResponse));

   INT64 start = GetCurrentTimeMs();

	switch(wType)
	{
		case NETSRV_CUSTOM:
			// unsupported for now
			nRet = CheckCustom(NULL, addr, wPort, 0);
			pResponse->setField(VID_RCC, ERR_SUCCESS);
			pResponse->setField(VID_SERVICE_STATUS, (UINT32)nRet);
			break;
		case NETSRV_SSH:
			nRet = CheckSSH(NULL, addr, wPort, NULL, NULL, 0);

			pResponse->setField(VID_RCC, ERR_SUCCESS);
			pResponse->setField(VID_SERVICE_STATUS, (UINT32)nRet);
			break;
		case NETSRV_TELNET:
			nRet = CheckTelnet(NULL, addr, wPort, NULL, NULL, 0);

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

					nRet = CheckPOP3(NULL, addr, wPort, pUser, pPass, 0);

				}

				pResponse->setField(VID_RCC, ERR_SUCCESS);
				pResponse->setField(VID_SERVICE_STATUS, (UINT32)nRet);
			}
			break;
		case NETSRV_SMTP:
			nRet = PC_ERR_BAD_PARAMS;

			if (szRequest[0] != 0)
			{
				nRet = CheckSMTP(NULL, addr, wPort, szRequest, 0);
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
                  nRet = CheckHTTP(NULL, addr, wPort, pURI, pHost, szResponse, 0);
               }
               else
               {
                  nRet = CheckHTTPS(NULL, addr, wPort, pURI, pHost, szResponse, 0);
               }
				}

				pResponse->setField(VID_RCC, ERR_SUCCESS);
				pResponse->setField(VID_SERVICE_STATUS, (UINT32)nRet);
			}
			break;
		default:
			bHandled = false;
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
 * Configuration template
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
	{ _T("DomainName"), CT_MB_STRING, 0, 0, 128, 0, g_szDomainName },
	{ _T("FailedDirectory"), CT_MB_STRING, 0, 0, 1024, 0, &g_szFailedDir },
   { _T("HostName"), CT_MB_STRING, 0, 0, 128, 0, g_hostName },
   { _T("NegativeResponseTimeOnError"), CT_BOOLEAN_FLAG_32, 0, 0, SCF_NEGATIVE_TIME_ON_ERROR, 0, &g_serviceCheckFlags },
   { _T("Timeout"), CT_LONG, 0, 0, 0, 0, &s_defaultTimeout },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Subagent initialization callback
 */
static bool SubagentInit(Config *config)
{
	config->parseTemplate(_T("portCheck"), m_cfgTemplate);
	return true;
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
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
static NETXMS_SUBAGENT_INFO s_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("PORTCHECK"),
	NETXMS_VERSION_STRING,
	SubagentInit, nullptr, CommandHandler, nullptr,
	sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	s_parameters,
	0, nullptr,	// lists
	0, nullptr,	// tables
   0, nullptr,	// actions
	0, nullptr	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(PORTCHECK)
{
	*ppInfo = &s_info;
	return true;
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
