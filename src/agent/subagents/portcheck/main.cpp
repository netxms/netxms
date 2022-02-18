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
 * Get timeout from metric arguments
 * @return default timeout is returned if no timeout specified in args
 */
uint32_t GetTimeoutFromArgs(const TCHAR* metric, int argIndex)
{
   TCHAR timeoutText[64];
   if (!AgentGetParameterArg(metric, argIndex, timeoutText, 64))
      return s_defaultTimeout;
   TCHAR* eptr;
   uint32_t timeout = _tcstol(timeoutText, &eptr, 0);
   return ((timeout != 0) && (*eptr == 0)) ? timeout : s_defaultTimeout;
}

/**
 * Connect to given host
 */
SOCKET NetConnectTCP(const char* hostname, const InetAddress& addr, uint16_t port, uint32_t dwTimeout)
{
   InetAddress hostAddr = (hostname != nullptr) ? InetAddress::resolveHostName(hostname) : addr;
   if (!hostAddr.isValidUnicast() && !hostAddr.isLoopback())
      return INVALID_SOCKET;

   return ConnectToHost(hostAddr, port, (dwTimeout != 0) ? dwTimeout : s_defaultTimeout);
}

/**
 * Command handler
 */
bool CommandHandler(UINT32 dwCommand, NXCPMessage* pRequest, NXCPMessage* pResponse, AbstractCommSession* session)
{
   if (dwCommand != CMD_CHECK_NETWORK_SERVICE)
      return false;

   uint16_t serviceType = pRequest->getFieldAsUInt16(VID_SERVICE_TYPE);
   uint16_t port = pRequest->getFieldAsUInt16(VID_IP_PORT);
   InetAddress addr = pRequest->getFieldAsInetAddress(VID_IP_ADDRESS);

   char serviceRequest[1024 * 10], serviceResponse[1024 * 10];
   pRequest->getFieldAsMBString(VID_SERVICE_REQUEST, serviceRequest, sizeof(serviceRequest));
   pRequest->getFieldAsMBString(VID_SERVICE_RESPONSE, serviceResponse, sizeof(serviceResponse));

   int64_t start = GetCurrentTimeMs();
   bool bHandled = true;
   uint32_t status;

   switch (serviceType)
   {
      case NETSRV_CUSTOM:
         status = CheckCustom(nullptr, addr, port, s_defaultTimeout);
         pResponse->setField(VID_RCC, ERR_SUCCESS);
         pResponse->setField(VID_SERVICE_STATUS, status);
         break;
      case NETSRV_SSH:
         status = CheckSSH(nullptr, addr, port, nullptr, nullptr, s_defaultTimeout);
         pResponse->setField(VID_RCC, ERR_SUCCESS);
         pResponse->setField(VID_SERVICE_STATUS, status);
         break;
      case NETSRV_TELNET:
         status = CheckTelnet(nullptr, addr, port, nullptr, nullptr, s_defaultTimeout);
         pResponse->setField(VID_RCC, ERR_SUCCESS);
         pResponse->setField(VID_SERVICE_STATUS, status);
         break;
      case NETSRV_POP3:
      case NETSRV_POP3S:
      {
         status = PC_ERR_BAD_PARAMS;
         char *username = serviceRequest;
         char *password = strchr(serviceRequest, ':');
         if (password != nullptr)
         {
            *password = 0;
            password++;
            status = CheckPOP3(addr, port, serviceType == NETSRV_POP3S, username, password, s_defaultTimeout);
         }

         pResponse->setField(VID_RCC, ERR_SUCCESS);
         pResponse->setField(VID_SERVICE_STATUS, status);
         break;
      }
      case NETSRV_SMTP:
      case NETSRV_SMTPS:
         status = PC_ERR_BAD_PARAMS;

         if (serviceRequest[0] != 0)
         {
            status = CheckSMTP(serviceType == NETSRV_SMTPS, addr, port, serviceRequest, s_defaultTimeout);
            pResponse->setField(VID_RCC, ERR_SUCCESS);
            pResponse->setField(VID_SERVICE_STATUS, status);
         }

         pResponse->setField(VID_RCC, ERR_SUCCESS);
         pResponse->setField(VID_SERVICE_STATUS, status);
         break;
      case NETSRV_FTP:
         bHandled = FALSE;
         break;
      case NETSRV_HTTP:
      case NETSRV_HTTPS:
      {
         char* pHost;
         char* pURI;

         status = PC_ERR_BAD_PARAMS;

         pHost = serviceRequest;
         pURI = strchr(serviceRequest, ':');
         if (pURI != NULL)
         {
            *pURI = 0;
            pURI++;
            status = CheckHTTP(nullptr, addr, port, serviceType == NETSRV_HTTPS, pURI, pHost, serviceResponse, s_defaultTimeout);
         }

         pResponse->setField(VID_RCC, ERR_SUCCESS);
         pResponse->setField(VID_SERVICE_STATUS, status);
      }
      break;
      case NETSRV_TLS:
         status = CheckTLS(nullptr, addr, port, s_defaultTimeout);
         pResponse->setField(VID_RCC, ERR_SUCCESS);
         pResponse->setField(VID_SERVICE_STATUS, status);
         break;
      default:
         bHandled = false;
         break;
   }

   if (bHandled)
   {
      uint32_t elapsed = static_cast<uint32_t>(GetCurrentTimeMs() - start);
      pResponse->setField(VID_RESPONSE_TIME, elapsed);
   }
   return bHandled;
}

/**
 * Configuration template
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] = {
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
static bool SubagentInit(Config* config)
{
   config->parseTemplate(_T("portCheck"), m_cfgTemplate);
   return true;
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] = {
   { _T("ServiceCheck.Custom(*)"), H_CheckCustom, _T("C"), DCI_DT_INT, _T("Status of remote TCP service") },
   { _T("ServiceCheck.HTTP(*)"), H_CheckHTTP, _T("C"), DCI_DT_INT, _T("Status of remote HTTP service") },
   { _T("ServiceCheck.HTTPS(*)"), H_CheckHTTP, _T("CS"), DCI_DT_INT, _T("Status of remote HTTPS service") },
   { _T("ServiceCheck.POP3(*)"), H_CheckPOP3, _T("C"), DCI_DT_INT, _T("Status of remote POP3 service") },
   { _T("ServiceCheck.POP3S(*)"), H_CheckPOP3, _T("CS"), DCI_DT_INT, _T("Status of remote POP3S service") },
   { _T("ServiceCheck.SMTP(*)"), H_CheckSMTP, _T("C"), DCI_DT_INT, _T("Status of remote SMTP service") },
   { _T("ServiceCheck.SMTPS(*)"), H_CheckSMTP, _T("CS"), DCI_DT_INT, _T("Status of remote SMTPS service") },
   { _T("ServiceCheck.SSH(*)"), H_CheckSSH, _T("C"), DCI_DT_INT, _T("Status of remote SSH service") },
   { _T("ServiceCheck.Telnet(*)"), H_CheckTelnet, _T("C"), DCI_DT_INT, _T("Status of remote TELNET service") },
   { _T("ServiceCheck.TLS(*)"), H_CheckTLS, _T("C"), DCI_DT_INT, _T("Status of remote TLS service") },
   { _T("ServiceResponseTime.Custom(*)"), H_CheckCustom, _T("R"), DCI_DT_INT, _T("Response time of remote TCP service") },
   { _T("ServiceResponseTime.HTTP(*)"), H_CheckHTTP, _T("R"), DCI_DT_INT, _T("Response time of remote HTTP service") },
   { _T("ServiceResponseTime.HTTPS(*)"), H_CheckHTTP, _T("RS"), DCI_DT_INT, _T("Response time of remote HTTPS service") },
   { _T("ServiceResponseTime.POP3(*)"), H_CheckPOP3, _T("R"), DCI_DT_INT, _T("Response time of remote POP3 service") },
   { _T("ServiceResponseTime.SMTP(*)"), H_CheckSMTP, _T("R"), DCI_DT_INT, _T("Response time of remote SMTP service") },
   { _T("ServiceResponseTime.SSH(*)"), H_CheckSSH, _T("R"), DCI_DT_INT, _T("Response time of remote SSH service") },
   { _T("ServiceResponseTime.Telnet(*)"), H_CheckTelnet, _T("R"), DCI_DT_INT, _T("Response time of remote TELNET service") },
   { _T("ServiceResponseTime.Telnet(*)"), H_CheckTLS, _T("R"), DCI_DT_INT, _T("Response time of remote TLS service") },
   { _T("TLS.Certificate.ExpirationDate(*)"), H_TLSCertificateInfo, _T("D"), DCI_DT_STRING, _T("Expiration date (YYYY-MM-DD) of X.509 certificate of remote TLS service") },
   { _T("TLS.Certificate.ExpirationTime(*)"), H_TLSCertificateInfo, _T("E"), DCI_DT_UINT64, _T("Expiration time of X.509 certificate of remote TLS service") },
   { _T("TLS.Certificate.ExpiresIn(*)"), H_TLSCertificateInfo, _T("U"), DCI_DT_INT, _T("Days until expiration of X.509 certificate of remote TLS service") },
   { _T("TLS.Certificate.Issuer(*)"), H_TLSCertificateInfo, _T("I"), DCI_DT_STRING, _T("Issuer of X.509 certificate of remote TLS service") },
   { _T("TLS.Certificate.Subject(*)"), H_TLSCertificateInfo, _T("S"), DCI_DT_STRING, _T("Subject of X.509 certificate of remote TLS service") },
   { _T("TLS.Certificate.TemplateID(*)"), H_TLSCertificateInfo, _T("T"), DCI_DT_STRING, _T("Template ID of X.509 certificate of remote TLS service") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info = {
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("PORTCHECK"),
   NETXMS_VERSION_STRING,
   SubagentInit, nullptr, CommandHandler, nullptr,
   sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   s_parameters,
   0, nullptr, // lists
   0, nullptr, // tables
   0, nullptr, // actions
   0, nullptr  // push parameters
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
