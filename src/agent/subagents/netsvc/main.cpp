/*
 ** NetXMS Network Service check subagent
 ** Copyright (C) 2013-2022 Alex Kirhenshtein
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

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <netxms-regex.h>
#include <netxms-version.h>
#include <stdlib.h>

#include <curl/curl.h>

#include "netsvc.h"

// workaround for older cURL versions
#ifndef CURL_MAX_HTTP_HEADER
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

uint32_t g_netsvcFlags = NETSVC_AF_VERIFYPEER;
uint32_t g_netsvcTimeout = 30;
uint32_t g_serviceCheckFlags = 0;

char g_certBundle[1024] =
{ 0 };

/**
 * Config file definition
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
{ _T("VerifyPeer"), CT_BOOLEAN_FLAG_32, 0, 0, NETSVC_AF_VERIFYPEER, 0,
        &g_netsvcFlags },
{ _T("CA"), CT_MB_STRING, 0, 0, 1024, 0, &g_certBundle },
{ _T("Timeout"), CT_WORD, 0, 0, 0, 0, &g_netsvcTimeout },
{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr } };

void CurlCommonSetup(CURL *curl, char *url, const OptionList& options)
{
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_netsvcTimeout);

#if HAVE_DECL_CURLOPT_NOSIGNAL
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, static_cast<long>(1)); // do not install signal handlers or send signals
#endif

    // SSL-related stuff
    bool verifyPeer = (((g_netsvcFlags & NETSVC_AF_VERIFYPEER) != 0)
            || options.exists(_T("verify-peer")))
            && !options.exists(_T("no-verify-peer"));
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,
            static_cast<long>(verifyPeer ? 1 : 0));
    if (g_certBundle[0] != 0)
    {
        curl_easy_setopt(curl, CURLOPT_CAINFO, g_certBundle);
    }
    if (options.exists(_T("no-verify-host")))
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST,
                static_cast<long>(0));
    else if (options.exists(_T("verify-host")))
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST,
                static_cast<long>(2));

}

void GetUrlValues(const TCHAR *uri, TCHAR *scheme, TCHAR *username, TCHAR *password, TCHAR *host, TCHAR *port)
{
    String str1(uri);
    ssize_t loc = str1.find(_T("://"));
    _tcscpy(scheme, str1.left(loc).cstr());

    String str2(str1.substring(loc + 3, str1.length() - loc - 3));
    ssize_t loc2 = str2.find(_T(":"));
    if(loc2 > 0)
    {
        _tcscpy(username, str2.left(loc2).cstr());
    }
    else
    {
        _tcscpy(username,_T(""));
    }

    String str3(str2.substring(loc2 + 1, str2.length() - loc2 - 1));
    ssize_t loc3 = str3.find(_T("@"));
    if(loc3 > 0)
    {
        _tcscpy(password, str3.left(loc3).cstr());
    }
    else
    {
        _tcscpy(password, _T(""));
    }

    String str4(str3.substring(loc3 + 1, str3.length() - loc3 - 1));
    ssize_t loc4 = str4.find(_T(":"));
    if(loc4 > 0)
    {
        _tcscpy(host, str4.left(loc4).cstr());

        String str5(str4.substring(loc4 + 1, str4.length() - loc4 - 1));
        ssize_t loc5 = str5.find(_T("/"));
        if(loc5 > 0)
        {
            _tcscpy(port, str5.left(loc5).cstr());
        }
        else
        {
            _tcscpy(port, _T(""));
        }
    }
    else
    {
        ssize_t loc4 = str4.find(_T("/"));
        if(loc4 > 0)
        {
            _tcscpy(host, str4.left(loc4).cstr());
        }
        else
        {
            _tcscpy(host, str4);
        }
    }
}

static LONG H_CheckService(const TCHAR *parameters, const TCHAR *arg,
        TCHAR *value, AbstractCommSession *session)
{
    LONG result =  SYSINFO_RC_UNKNOWN;

    TCHAR turl[2048] = _T("");
    AgentGetParameterArg(parameters, 1, turl, 2048);
    Trim(turl);

    TCHAR pattern[256] = _T("");
    AgentGetParameterArg(parameters, 2, pattern, 256);
    Trim(pattern);
    if (pattern[0] == 0)
    {
        _tcscpy(pattern, _T("^HTTP\\/(1\\.[01]|2) 200 .*"));
    }

    const OptionList options(parameters, 3);

    if (turl[0] == 0)
    {
        return SYSINFO_RC_UNSUPPORTED;
    }

    TCHAR scheme[256] = _T("");
    TCHAR username[256] = _T("");
    TCHAR password[256] = _T("");
    TCHAR thost[256] = _T("");
    TCHAR port[256] = _T("");
    GetUrlValues(turl, scheme, username, password, thost, port);

    char url[2048] = "";
    wcstombs(url,turl, sizeof(url)/sizeof(url[0]));

    char host[2048] = "";
    wcstombs(host,thost, sizeof(host)/sizeof(host[0]));

    int64_t start = GetCurrentTimeMs();

    CURL *curl = curl_easy_init();
    if (curl != nullptr)
    {
        CurlCommonSetup(curl, url, options);
        if(_tcsstr(scheme, _T("http")) == scheme)
        {
            const char *eptr;
            int eoffset;
            PCRE *compiledPattern = _pcre_compile_t(
                    reinterpret_cast<const PCRE_TCHAR*>(pattern),
                    PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
            if (compiledPattern == nullptr)
            {
                nxlog_debug_tag(DEBUG_TAG, 3,
                        _T("H_CheckService(%hs): Cannot compile pattern \"%s\""), turl,
                        pattern);
                result = SYSINFO_RC_UNSUPPORTED;
            }
            else
            {
                result = H_CheckServiceHTTP(curl, value, options, url, compiledPattern);
            }
        }
        else if(_tcsstr(scheme, _T("smtp")) == scheme)
        {
            result = H_CheckServiceSMTP(curl, value, options);
        }
        else if(_tcsstr(scheme, _T("ssh")) == scheme)
        {
            result = H_CheckServiceSSH(host, port, value, options);
        }
        else if(_tcsstr(scheme, _T("telnet")) == scheme)
        {
            result = H_CheckServiceTELNET(host, port, value, options);
        }
        else
        {
            result = H_CheckServiceDefault(curl, value);
        }

        curl_easy_cleanup(curl);
    }
    else
    {
        nxlog_debug_tag(DEBUG_TAG, 3,
                _T("H_CheckService(%hs): curl_init failed"), url);
    }

    if (*arg == 'R')
    {
       if (result == PC_ERR_NONE)
       {
           ret_int64(value, GetCurrentTimeMs() - start);
       }
       else if (g_serviceCheckFlags & SCF_NEGATIVE_TIME_ON_ERROR)
       {
           ret_int(value, -result);
       }
       else
       {
           result = SYSINFO_RC_ERROR;
       }
    }

    return result;
}

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
    bool success = config->parseTemplate(_T("netsvc"), m_cfgTemplate);

    if (success)
        success = InitializeLibCURL();

    return success;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
}

/**
 * Provided parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
{ _T("NetworkService.Check(*)"), H_CheckService, _T("C"), DCI_DT_INT, _T("Service {instance} status") },
{ _T("NetworkService.ResponseTime(*)"), H_CheckService, _T("R"), DCI_DT_INT, _T("Service {instance} response time") },
{ _T("NetworkService.CheckTLS(*)"), H_CheckTLS, _T("C"), DCI_DT_INT, _T("Status of remote TLS service") },
{ _T("NetworkService.TLS.Certificate.ExpirationDate(*)"), H_TLSCertificateInfo, _T("D"), DCI_DT_STRING, _T("Expiration date (YYYY-MM-DD) of X.509 certificate of remote TLS service") },
{ _T("NetworkService.TLS.Certificate.ExpirationTime(*)"), H_TLSCertificateInfo, _T("E"), DCI_DT_UINT64, _T("Expiration time of X.509 certificate of remote TLS service") },
{ _T("NetworkService.TLS.Certificate.ExpiresIn(*)"), H_TLSCertificateInfo, _T("U"), DCI_DT_INT, _T("Days until expiration of X.509 certificate of remote TLS service") },
{ _T("NetworkService.TLS.Certificate.Issuer(*)"), H_TLSCertificateInfo, _T("I"), DCI_DT_STRING, _T("Issuer of X.509 certificate of remote TLS service") },
{ _T("NetworkService.TLS.Certificate.Subject(*)"), H_TLSCertificateInfo, _T("S"), DCI_DT_STRING, _T("Subject of X.509 certificate of remote TLS service") },
{ _T("NetworkService.TLS.Certificate.TemplateID(*)"), H_TLSCertificateInfo, _T("T"), DCI_DT_STRING, _T("Template ID of X.509 certificate of remote TLS service") } };

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
NETXMS_SUBAGENT_INFO_MAGIC, _T("NETSVC"), NETXMS_VERSION_STRING, SubagentInit,
        SubagentShutdown, nullptr, nullptr, sizeof(m_parameters)
                / sizeof(NETXMS_SUBAGENT_PARAM), m_parameters, 0, nullptr, // enums
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
