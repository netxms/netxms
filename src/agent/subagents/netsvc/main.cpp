/*
 ** NetXMS Network Service check subagent
 ** Copyright (C) 2013-2025 Raden Solutions
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
 **/

#include "netsvc.h"
#include <netxms-version.h>
#include <nxcldefs.h>

// workaround for older cURL versions
#ifndef CURL_MAX_HTTP_HEADER
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

uint32_t g_netsvcFlags = NETSVC_AF_VERIFYPEER;
uint32_t g_netsvcTimeout = 1000;
char g_netsvcDomainName[128] = "example.org";

/**
 * Certificate bundle
 */
static char s_certBundle[1024] = "";

/**
 * Common cURL handle setup
 */
void CurlCommonSetup(CURL *curl, const char *url, const OptionList& options, uint32_t timeout)
{
   curl_easy_setopt(curl, CURLOPT_URL, url);
   curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (timeout != 0) ? timeout : g_netsvcTimeout);

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, static_cast<long>(1)); // do not install signal handlers or send signals
#endif

   // SSL-related stuff
   bool verifyPeer = options.getAsBoolean(_T("verify-peer"), (g_netsvcFlags & NETSVC_AF_VERIFYPEER) != 0);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, static_cast<long>(verifyPeer ? 1 : 0));

   if (s_certBundle[0] != 0)
      curl_easy_setopt(curl, CURLOPT_CAINFO, s_certBundle);

   if (options.getAsBoolean(_T("verify-host"), true))
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, static_cast<long>(2));
   else
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, static_cast<long>(0));

   const TCHAR *login = options.get(_T("login"), _T(""));
   const TCHAR *password = options.get(_T("password"), _T(""));
   if ((login[0] != 0) && (password[0] != 0))
   {
      char loginUTF8[128];
      char decryptedPassword[128];
      tchar_to_utf8(login, -1, loginUTF8, 128);
      tchar_to_utf8(password, -1, decryptedPassword, 128);
      DecryptPasswordA(loginUTF8, decryptedPassword, decryptedPassword, sizeof(decryptedPassword));
      curl_easy_setopt(curl, CURLOPT_USERNAME, loginUTF8);
      curl_easy_setopt(curl, CURLOPT_PASSWORD, decryptedPassword);
   }

   const TCHAR *tlsMode = options.get(_T("tls-mode"), _T(""));
   if (tlsMode[0] != 0)
   {
      if (!_tcsicmp(tlsMode, _T("try")))
      {
         curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_TRY);
      }
      else if (!_tcsicmp(tlsMode, _T("always")))
      {
         curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
      }
      else
      {
         curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_NONE);
         if (_tcsicmp(tlsMode, _T("none")))
            nxlog_debug_tag(DEBUG_TAG, 4, _T("CurlCommonSetup(%hs): invalid value \"%s\" for tls-mode, should be one of: none, try, always. TLS mode set to \"none\"."), url, tlsMode);
      }
   }
}

/**
 * Prepare curl handle for generic service test
 */
CURL *PrepareCurlHandle(const InetAddress& addr, uint16_t port, const char *schema, uint32_t timeout)
{
   CURL *curl = curl_easy_init();
   if (curl == nullptr)
      return nullptr;

   char url[NETSVC_MAX_URL_LENGTH], addrBuffer[64];
   snprintf(url, NETSVC_MAX_URL_LENGTH, "%s://%s:%u", schema, addr.toStringA(addrBuffer), static_cast<uint32_t>(port));
   CurlCommonSetup(curl, url, OptionList(_T("")), timeout);
   return curl;
}

/**
 * Handler for NetworkService.Status and NetworkService.ResponseTime metrics
 */
static LONG H_NetworkServiceStatus(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char url[2048];
   if (!AgentGetParameterArgA(metric, 1, url, 2048))
      return SYSINFO_RC_UNSUPPORTED;

   if (url[0] == 0)
      return SYSINFO_RC_UNSUPPORTED;

   // Legacy version requires pattern as second argument
   // For new version pattern should be set as named option
   const OptionList options(metric, (arg[1] == 'L') ? 3 : 2);
   if (!options.isValid())
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR pattern[256];
   if (arg[1] == 'L')
   {
      if (!AgentGetParameterArg(metric, 2, pattern, 256))
         return SYSINFO_RC_UNSUPPORTED;
      if (pattern[0] == 0)
         _tcscpy(pattern, _T("^HTTP\\/(1\\.[01]|2) 200 .*"));
   }
   else
   {
      if (options.exists(_T("pattern")))
         _tcslcpy(pattern, options.get(_T("pattern")), 256);
      else
         pattern[0] = 0;
   }

   // Analyze URL
   URLParser urlParser(url);
   if (!urlParser.isValid())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("H_NetworkServiceStatus(%hs): URL parsing error"), url);
      return SYSINFO_RC_UNSUPPORTED;
   }

   int64_t start = GetCurrentTimeMs();

   const char *scheme = urlParser.scheme();
   if (scheme == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("H_NetworkServiceStatus(%hs): cannot get scheme from URL"), url);
      return SYSINFO_RC_UNSUPPORTED;
   }

   // Check for forbidden schemes
   if (!strcmp(scheme, "file"))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("H_NetworkServiceStatus(%hs): forbidden scheme"), url);
      return SYSINFO_RC_ACCESS_DENIED;
   }

   LONG rc;
   int checkResult = PC_ERR_INTERNAL;
   if (!strcmp(scheme, "ssh"))
   {
      const char *host = urlParser.host();
      const char *port = urlParser.port();
      if ((host != nullptr) && (port != nullptr))
      {
         rc = NetworkServiceStatus_SSH(host, port, options, &checkResult);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("H_NetworkServiceStatus(%hs): cannot extract host and port parts from URL"), url);
         rc = SYSINFO_RC_UNSUPPORTED;
      }
   }
   else if (!strcmp(scheme, "telnet"))
   {
      const char *host = urlParser.host();
      const char *port = urlParser.port();
      if ((host != nullptr) && (port != nullptr))
      {
         rc = NetworkServiceStatus_Telnet(host, port, options, &checkResult);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("H_NetworkServiceStatus(%hs): cannot extract host and port parts from URL"), url);
         rc = SYSINFO_RC_UNSUPPORTED;
      }
   }
   else if (!strcmp(scheme, "tcp"))
   {
      const char *host = urlParser.host();
      const char *port = urlParser.port();
      if ((host != nullptr) && (port != nullptr))
      {
         rc = NetworkServiceStatus_TCP(host, port, options, &checkResult);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("H_NetworkServiceStatus(%hs): cannot extract host and port parts from URL"), url);
         rc = SYSINFO_RC_UNSUPPORTED;
      }
   }
   else
   {
      CURL *curl = curl_easy_init();
      if (curl != nullptr)
      {
         CurlCommonSetup(curl, url, options, options.getAsUInt32(_T("timeout"), g_netsvcTimeout));
         if (!strcmp(scheme, "http") || !strcmp(scheme, "https"))
         {
            if (pattern[0] != 0)
            {
               const char *eptr;
               int eoffset;
               PCRE *compiledPattern = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
               if (compiledPattern != nullptr)
               {
                  rc = NetworkServiceStatus_HTTP(curl, options, url, compiledPattern, &checkResult);
                  _pcre_free_t(compiledPattern);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 5, _T("H_NetworkServiceStatus(%hs): Cannot compile pattern \"%s\""), url, pattern);
                  rc = SYSINFO_RC_UNSUPPORTED;
               }
            }
            else
            {
               rc = NetworkServiceStatus_HTTP(curl, options, url, nullptr, &checkResult);
            }
         }
         else if (!strcmp(scheme, "smtp") || !strcmp(scheme, "smtps"))
         {
            rc = NetworkServiceStatus_SMTP(curl, options, url, &checkResult);
         }
         else
         {
            rc = NetworkServiceStatus_Other(curl, options, url, &checkResult);
         }
         curl_easy_cleanup(curl);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("H_NetworkServiceStatus(%hs): curl_easy_init failed"), url);
         rc = SYSINFO_RC_ERROR;
      }
   }

   if ((rc == SYSINFO_RC_SUCCESS) && (checkResult == PC_ERR_BAD_PARAMS))
      rc = SYSINFO_RC_UNSUPPORTED;

   if (rc == SYSINFO_RC_SUCCESS)
   {
      if (*arg == 'R')
      {
         if (checkResult == PC_ERR_NONE)
         {
            ret_int64(value, GetCurrentTimeMs() - start);
         }
         else if (g_netsvcFlags & NETSVC_AF_NEGATIVE_TIME_ON_ERROR)
         {
            ret_int64(value, -(GetCurrentTimeMs() - start));
         }
         else
         {
            rc = SYSINFO_RC_ERROR;
         }
      }
      else
      {
         ret_int(value, checkResult);
      }
   }

   return rc;
}

/**
 * Config file definition
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("CA"), CT_MB_STRING, 0, 0, 1024, 0, &s_certBundle },
   { _T("DomainName"), CT_MB_STRING, 0, 0, 128, 0, g_netsvcDomainName },
   { _T("NegativeResponseTimeOnError"), CT_BOOLEAN_FLAG_32, 0, 0, NETSVC_AF_NEGATIVE_TIME_ON_ERROR, 0, &g_netsvcFlags },
   { _T("VerifyPeer"), CT_BOOLEAN_FLAG_32, 0, 0, NETSVC_AF_VERIFYPEER, 0, &g_netsvcFlags },
   { _T("Timeout"), CT_WORD, 0, 0, 0, 0, &g_netsvcTimeout },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
   if (!InitializeLibCURL())
      return false;

   return config->parseTemplate(_T("netsvc"), m_cfgTemplate);
}

/**
 * Command handler
 */
bool CommandHandler(uint32_t dwCommand, NXCPMessage *pRequest, NXCPMessage *pResponse, AbstractCommSession *session)
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
   uint32_t status;
   char *username, *password, *host, *uri;
   switch (serviceType)
   {
      case NETSRV_CUSTOM:
         status = CheckTCP(nullptr, addr, port, g_netsvcTimeout);
         break;
      case NETSRV_SSH:
         status = CheckSSH(nullptr, addr, port, g_netsvcTimeout);
         break;
      case NETSRV_TELNET:
         status = CheckTelnet(nullptr, addr, port, g_netsvcTimeout);
         break;
      case NETSRV_POP3:
      case NETSRV_POP3S:
         username = serviceRequest;
         password = strchr(serviceRequest, ':');
         if (password != nullptr)
         {
            *password = 0;
            password++;
            status = CheckPOP3(addr, port, serviceType == NETSRV_POP3S, username, password, g_netsvcTimeout);
         }
         else
         {
            status = PC_ERR_BAD_PARAMS;
         }
         break;
      case NETSRV_SMTP:
      case NETSRV_SMTPS:
         if (serviceRequest[0] != 0)
         {
            status = CheckSMTP(addr, port, serviceType == NETSRV_SMTPS, serviceRequest, g_netsvcTimeout);
         }
         else
         {
            status = PC_ERR_BAD_PARAMS;
         }
         break;
      case NETSRV_HTTP:
      case NETSRV_HTTPS:
         host = serviceRequest;
         uri = strchr(serviceRequest, ':');
         if (uri != nullptr)
         {
            *uri = 0;
            uri++;
            status = CheckHTTP(nullptr, addr, port, serviceType == NETSRV_HTTPS, uri, host, serviceResponse, g_netsvcTimeout);
         }
         else
         {
            status = PC_ERR_BAD_PARAMS;
         }
         break;
      case NETSRV_TLS:
         status = CheckTLS(nullptr, addr, port, g_netsvcTimeout);
         break;
      default:
         status = PC_ERR_BAD_PARAMS;
         break;
   }

   uint32_t elapsed = static_cast<uint32_t>(GetCurrentTimeMs() - start);
   pResponse->setField(VID_RCC, ERR_SUCCESS);
   pResponse->setField(VID_SERVICE_STATUS, status);
   pResponse->setField(VID_RESPONSE_TIME, elapsed);
   return true;
}

/**
 * Provided metrics
 */
static NETXMS_SUBAGENT_PARAM s_metrics[] =
{
   { _T("HTTP.Checksum.MD5(*)"), H_HTTPChecksum, _T("5"), DCI_DT_STRING, _T("MD5 hash for content at {instance}") },
   { _T("HTTP.Checksum.SHA1(*)"), H_HTTPChecksum, _T("1"), DCI_DT_STRING, _T("SHA1 hash for content at {instance}") },
   { _T("HTTP.Checksum.SHA256(*)"), H_HTTPChecksum, _T("2"), DCI_DT_STRING, _T("SHA256 hash for content at {instance}") },
   { _T("NetworkService.Status(*)"), H_NetworkServiceStatus, _T("C"), DCI_DT_INT, _T("Status of remote network service {instance}") },
   { _T("NetworkService.ResponseTime(*)"), H_NetworkServiceStatus, _T("R"), DCI_DT_INT, _T("Response time of remote network service {instance}") },
   { _T("NetworkService.TLSStatus(*)"), H_CheckTLS, _T("C"), DCI_DT_INT, _T("Status of remote TLS service {instance}") },
   { _T("NetworkService.TLSResponseTime(*)"), H_CheckTLS, _T("R"), DCI_DT_INT, _T("Response time of remote TLS service {instance}") },
   { _T("TLS.Certificate.ExpirationDate(*)"), H_TLSCertificateInfo, _T("D"), DCI_DT_STRING, _T("Expiration date (YYYY-MM-DD) of X.509 certificate of remote TLS service") },
   { _T("TLS.Certificate.ExpirationTime(*)"), H_TLSCertificateInfo, _T("E"), DCI_DT_UINT64, _T("Expiration time of X.509 certificate of remote TLS service") },
   { _T("TLS.Certificate.ExpiresIn(*)"), H_TLSCertificateInfo, _T("U"), DCI_DT_INT, _T("Days until expiration of X.509 certificate of remote TLS service") },
   { _T("TLS.Certificate.Issuer(*)"), H_TLSCertificateInfo, _T("I"), DCI_DT_STRING, _T("Issuer of X.509 certificate of remote TLS service") },
   { _T("TLS.Certificate.Subject(*)"), H_TLSCertificateInfo, _T("S"), DCI_DT_STRING, _T("Subject of X.509 certificate of remote TLS service") },
   { _T("TLS.Certificate.TemplateID(*)"), H_TLSCertificateInfo, _T("T"), DCI_DT_STRING, _T("Template ID of X.509 certificate of remote TLS service") },
   // Legacy metrics - netsvc subagent prior to 4.3
   { _T("NetworkService.Check(*)"), H_NetworkServiceStatus, _T("CL"), DCI_DT_DEPRECATED, _T("Service {instance} status") },
   { _T("Service.Check(*)"), H_NetworkServiceStatus, _T("CL"), DCI_DT_DEPRECATED, _T("Service {instance} status") },
   // Legacy metrics - portcheck subagent
   { _T("ServiceCheck.Custom(*)"), H_CheckTCP, _T("C"), DCI_DT_DEPRECATED, _T("Status of remote TCP service {instance}") },
   { _T("ServiceCheck.HTTP(*)"), H_CheckHTTP, _T("C"), DCI_DT_DEPRECATED, _T("Status of remote HTTP service {instance}") },
   { _T("ServiceCheck.HTTPS(*)"), H_CheckHTTP, _T("CS"), DCI_DT_DEPRECATED, _T("Status of remote HTTPS service {instance}") },
   { _T("ServiceCheck.POP3(*)"), H_CheckPOP3, _T("C"), DCI_DT_DEPRECATED, _T("Status of remote POP3 service {instance}") },
   { _T("ServiceCheck.POP3S(*)"), H_CheckPOP3, _T("CS"), DCI_DT_DEPRECATED, _T("Status of remote POP3S service {instance}") },
   { _T("ServiceCheck.SMTP(*)"), H_CheckSMTP, _T("C"), DCI_DT_DEPRECATED, _T("Status of remote SMTP service {instance}") },
   { _T("ServiceCheck.SMTPS(*)"), H_CheckSMTP, _T("CS"), DCI_DT_DEPRECATED, _T("Status of remote SMTPS service {instance}") },
   { _T("ServiceCheck.SSH(*)"), H_CheckSSH, _T("C"), DCI_DT_DEPRECATED, _T("Status of remote SSH service {instance}") },
   { _T("ServiceCheck.Telnet(*)"), H_CheckTelnet, _T("C"), DCI_DT_DEPRECATED, _T("Status of remote TELNET service {instance}") },
   { _T("ServiceCheck.TLS(*)"), H_CheckTLS, _T("C"), DCI_DT_DEPRECATED, _T("Status of remote TLS service {instance}") },
   { _T("ServiceResponseTime.Custom(*)"), H_CheckTCP, _T("R"), DCI_DT_DEPRECATED, _T("Response time of remote TCP service {instance}") },
   { _T("ServiceResponseTime.HTTP(*)"), H_CheckHTTP, _T("R"), DCI_DT_DEPRECATED, _T("Response time of remote HTTP service {instance}") },
   { _T("ServiceResponseTime.HTTPS(*)"), H_CheckHTTP, _T("RS"), DCI_DT_DEPRECATED, _T("Response time of remote HTTPS service {instance}") },
   { _T("ServiceResponseTime.POP3(*)"), H_CheckPOP3, _T("R"), DCI_DT_DEPRECATED, _T("Response time of remote POP3 service {instance}") },
   { _T("ServiceResponseTime.POP3S(*)"), H_CheckPOP3, _T("RS"), DCI_DT_DEPRECATED, _T("Response time of remote POP3 service {instance}") },
   { _T("ServiceResponseTime.SMTP(*)"), H_CheckSMTP, _T("R"), DCI_DT_DEPRECATED, _T("Response time of remote SMTP service {instance}") },
   { _T("ServiceResponseTime.SSH(*)"), H_CheckSSH, _T("R"), DCI_DT_DEPRECATED, _T("Response time of remote SSH service {instance}") },
   { _T("ServiceResponseTime.Telnet(*)"), H_CheckTelnet, _T("R"), DCI_DT_DEPRECATED, _T("Response time of remote TELNET service {instance}") },
   { _T("ServiceResponseTime.TLS(*)"), H_CheckTLS, _T("R"), DCI_DT_DEPRECATED, _T("Response time of remote TLS service {instance}") },
   // Legacy metrics - ECS subagent
   { _T("ECS.HttpMD5(*)"), H_HTTPChecksum, _T("5"), DCI_DT_DEPRECATED, _T("MD5 hash for content at {instance}") },
   { _T("ECS.HttpSHA1(*)"), H_HTTPChecksum, _T("1"), DCI_DT_DEPRECATED, _T("SHA1 hash for content at {instance}") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC, _T("NETSVC"), NETXMS_VERSION_STRING,
   SubagentInit, nullptr, CommandHandler, nullptr, nullptr,
   sizeof(s_metrics) / sizeof(NETXMS_SUBAGENT_PARAM), s_metrics,
   0, nullptr,  // lists
   0, nullptr,  // tables
   0, nullptr,  // actions
   0, nullptr   // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(NETSVC)
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
