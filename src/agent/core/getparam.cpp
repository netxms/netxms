/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: getparam.cpp
**
**/

#include "nxagentd.h"


//
// Externals
//

LONG H_AgentStats(char *cmd, char *arg, char *value);
LONG H_AgentUptime(char *cmd, char *arg, char *value);
LONG H_CRC32(char *cmd, char *arg, char *value);
LONG H_FileSize(char *cmd, char *arg, char *value);
LONG H_MD5Hash(char *cmd, char *arg, char *value);
LONG H_SHA1Hash(char *cmd, char *arg, char *value);


//
// Static data
//

static AGENT_PARAM *m_pParamList = NULL;
static int m_iNumParams = 0;
static DWORD m_dwTimedOutRequests = 0;
static DWORD m_dwAuthenticationFailures = 0;
static DWORD m_dwProcessedRequests = 0;
static DWORD m_dwFailedRequests = 0;
static DWORD m_dwUnsupportedRequests = 0;


//
// Handler for parameters which always returns string constant
//

static LONG H_StringConstant(char *cmd, char *arg, char *value)
{
   ret_string(value, arg);
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for parameters which returns floating point value from specific variable
//

static LONG H_FloatPtr(char *cmd, char *arg, char *value)
{
   ret_double(value, *((double *)arg));
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for parameters which returns DWORD value from specific variable
//

static LONG H_UIntPtr(char *cmd, char *arg, char *value)
{
   ret_uint(value, *((DWORD *)arg));
   return SYSINFO_RC_SUCCESS;
}


//
// Standard agent's parameters
//

static AGENT_PARAM m_stdParams[] =
{
   { "Agent.AcceptedConnections", H_UIntPtr, (char *)&g_dwAcceptedConnections },
   { "Agent.AcceptErrors", H_UIntPtr, (char *)&g_dwAcceptErrors },
   { "Agent.AuthenticationFailures", H_UIntPtr, (char *)&m_dwAuthenticationFailures },
   { "Agent.FailedRequests", H_UIntPtr, (char *)&m_dwFailedRequests },
   { "Agent.ProcessedRequests", H_UIntPtr, (char *)&m_dwProcessedRequests },
   { "Agent.RejectedConnections", H_UIntPtr, (char *)&g_dwRejectedConnections },
   { "Agent.TimedOutRequests", H_UIntPtr, (char *)&m_dwTimedOutRequests },
   { "Agent.UnsupportedRequests", H_UIntPtr, (char *)&m_dwUnsupportedRequests },
   { "Agent.Uptime", H_AgentUptime, NULL },
   { "Agent.Version", H_StringConstant, AGENT_VERSION_STRING },
   { "File.Hash.CRC32(*)", H_CRC32, NULL },
   { "File.Hash.MD5(*)", H_MD5Hash, NULL },
   { "File.Hash.SHA1(*)", H_SHA1Hash, NULL },
   { "File.Size(*)", H_FileSize, NULL }
};


//
// Initialize dynamic parameters list from default static list
//

BOOL InitParameterList(void)
{
   if (m_pParamList != NULL)
      return FALSE;

   m_iNumParams = sizeof(m_stdParams) / sizeof(AGENT_PARAM);
   m_pParamList = (AGENT_PARAM *)malloc(sizeof(AGENT_PARAM) * m_iNumParams);
   if (m_pParamList == NULL)
      return FALSE;

   memcpy(m_pParamList, m_stdParams, sizeof(AGENT_PARAM) * m_iNumParams);
   return TRUE;
}


//
// Add parameter to list
//

void AddParameter(char *szName, LONG (* fpHandler)(char *,char *,char *), char *pArg)
{
   m_pParamList = (AGENT_PARAM *)realloc(m_pParamList, sizeof(AGENT_PARAM) * (m_iNumParams + 1));
   strncpy(m_pParamList[m_iNumParams].name, szName, MAX_PARAM_NAME - 1);
   m_pParamList[m_iNumParams].handler = fpHandler;
   m_pParamList[m_iNumParams].arg = pArg;
   m_iNumParams++;
}


//
// Get parameter's value
//

DWORD GetParameterValue(char *pszParam, char *pszValue)
{
   int i, rc;
   DWORD dwErrorCode;

   for(i = 0; i < m_iNumParams; i++)
      if (MatchString(m_pParamList[i].name, pszParam, FALSE))
      {
         rc = m_pParamList[i].handler(pszParam, m_pParamList[i].arg, pszValue);
         switch(rc)
         {
            case SYSINFO_RC_SUCCESS:
               dwErrorCode = ERR_SUCCESS;
               m_dwProcessedRequests++;
               break;
            case SYSINFO_RC_ERROR:
               dwErrorCode = ERR_INTERNAL_ERROR;
               m_dwFailedRequests++;
               break;
            case SYSINFO_RC_UNSUPPORTED:
               dwErrorCode = ERR_UNKNOWN_PARAMETER;
               m_dwUnsupportedRequests++;
               break;
            default:
               WriteLog(MSG_UNEXPECTED_IRC, EVENTLOG_ERROR_TYPE, "ds", rc, pszParam);
               dwErrorCode = ERR_INTERNAL_ERROR;
               m_dwFailedRequests++;
               break;
         }
         break;
      }
   if (i == m_iNumParams)
   {
      dwErrorCode = ERR_UNKNOWN_PARAMETER;
      m_dwUnsupportedRequests++;
   }
   return dwErrorCode;
}
