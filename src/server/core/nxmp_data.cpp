/* $Id: nxmp_data.cpp,v 1.6 2006-12-26 22:53:59 victor Exp $ */
/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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
** File: nxmp_data.cpp
**
**/

#include "nxcore.h"
#include "nxmp_parser.h"


//
// Get string valus as unsigned int
//

static BOOL GetValueAsUInt(char *pszStr, DWORD *pdwValue)
{
   char *eptr;
   int nLen;
   
   nLen = strlen(pszStr);
   if (nLen == 0)
      return FALSE;

   if (pszStr[nLen - 1] == 'U')
   {
      pszStr[nLen - 1] = 0;
      *pdwValue = strtoul(pszStr, &eptr, 0);
   }
   else
   {
      LONG nValue;

      nValue = strtol(pszStr, &eptr, 0);
      memcpy(pdwValue, &nValue, sizeof(DWORD));
   }
   return (*eptr == 0);
}


//
// Constructor
//

NXMP_Data::NXMP_Data(NXMP_Lexer *pLexer, NXMP_Parser *pParser)
{
   m_pEventList = NULL;
   m_dwNumEvents = 0;
   m_pCurrEvent = NULL;
   m_nContext = CTX_NONE;
   m_pLexer = pLexer;
   m_pParser = pParser;
}


//
// Destructor
//

NXMP_Data::~NXMP_Data()
{
   DWORD i;

   for(i = 0; i < m_dwNumEvents; i++)
   {
      safe_free(m_pEventList[i].pszMessageTemplate);
      safe_free(m_pEventList[i].pszDescription);
   }
   safe_free(m_pEventList);
}


//
// Report error
//

void NXMP_Data::Error(const char *pszMsg, ...)
{
   va_list args;
   char szBuffer[1024];

   va_start(args, pszMsg);
   vsnprintf(szBuffer, 1024, pszMsg, args);
   va_end(args);
   m_pParser->Error(szBuffer);
   m_pLexer->SetErrorState();
}


//
// Parse variable
//

BOOL NXMP_Data::ParseVariable(char *pszName, char *pszValue)
{
   BOOL bRet = FALSE;
   DWORD dwValue;

   switch(m_nContext)
   {
      case CTX_EVENT:
         if (!stricmp(pszName, "CODE"))
         {
            if (GetValueAsUInt(pszValue, &dwValue))
            {
               SetEventCode(dwValue);
            }
            else
            {
               Error("Event code must be a numeric value");
            }
         }
         else if (!stricmp(pszName, "SEVERITY"))
         {
            if (GetValueAsUInt(pszValue, &dwValue))
            {
               SetEventSeverity(dwValue);
            }
            else
            {
               Error("Event severity must be a numeric value");
            }
         }
         else if (!stricmp(pszName, "FLAGS"))
         {
            if (GetValueAsUInt(pszValue, &dwValue))
            {
               SetEventFlags(dwValue);
            }
            else
            {
               Error("Event flags must be a numeric value");
            }
         }
         else if (!stricmp(pszName, "TEXT"))
         {
            SetEventText(pszValue);
         }
         else if (!stricmp(pszName, "DESCRIPTION"))
         {
            SetEventDescription(pszValue);
         }
         else
         {
            Error("Unknown event attribute %s", pszName);
         }
         break;
      default:
         break;
   }
   return bRet;
}


//
// Create new event
//

void NXMP_Data::NewEvent(char *pszName)
{
   m_dwNumEvents++;
   m_pEventList = (EVENT_TEMPLATE *)realloc(m_pEventList, sizeof(EVENT_TEMPLATE) * m_dwNumEvents);
   m_pCurrEvent = &m_pEventList[m_dwNumEvents - 1];
   memset(m_pCurrEvent, 0, sizeof(EVENT_TEMPLATE));
#ifdef UNICODE
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszName, -1, m_pCurrEvent->szName, MAX_EVENT_NAME - 1);
#else
   nx_strncpy(m_pCurrEvent->szName, pszName, MAX_EVENT_NAME);
#endif
   m_nContext = CTX_EVENT;
}


//
// Set event's text
//

void NXMP_Data::SetEventText(char *pszText)
{
   int nLen;

   if (m_pCurrEvent == NULL)
      return;

   nLen = (int)strlen(pszText) + 1;
   m_pCurrEvent->pszMessageTemplate = (TCHAR *)realloc(m_pCurrEvent->pszMessageTemplate, nLen * sizeof(TCHAR));
#ifdef UNICODE
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszText, -1, m_pCurrEvent->pszMessageTemplate, nLen);
#else
   strcpy(m_pCurrEvent->pszMessageTemplate, pszText);
#endif
}


//
// Set event's description
//

void NXMP_Data::SetEventDescription(char *pszText)
{
   int nLen;

   if (m_pCurrEvent == NULL)
      return;

   nLen = (int)strlen(pszText) + 1;
   m_pCurrEvent->pszDescription = (TCHAR *)realloc(m_pCurrEvent->pszDescription, nLen * sizeof(TCHAR));
#ifdef UNICODE
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszText, -1, m_pCurrEvent->pszDescription, nLen);
#else
   strcpy(m_pCurrEvent->pszDescription, pszText);
#endif
}


//
// Create new trap
//

void NXMP_Data::NewTrap(char *pszOID)
{
	DWORD dwOID[MAX_OID_LEN];
#ifdef UNICODE
	WCHAR *pwszOID;
#endif

   m_dwNumTraps++;
   m_pTrapList = (NXC_TRAP_CFG_ENTRY *)realloc(m_pTrapList, sizeof(NXC_TRAP_CFG_ENTRY) * m_dwNumTraps);
   m_pCurrTrap = &m_pTrapList[m_dwNumTraps - 1];
   memset(m_pCurrTrap, 0, sizeof(NXC_TRAP_CFG_ENTRY));
#ifdef UNICODE
	pwszOID = WideStringFromMBString(pszOID);
	m_pCurrTrap->dwOidLen = SNMPParseOID(pwszOID, dwOID, MAX_OID_LEN);
	free(pwszOID);
#else
	m_pCurrTrap->dwOidLen = SNMPParseOID(pszOID, dwOID, MAX_OID_LEN);
#endif
	m_pCurrTrap->pdwObjectId = (DWORD *)nx_memdup(dwOID, m_pCurrTrap->dwOidLen * sizeof(DWORD));
   m_nContext = CTX_TRAP;
}


//
// Set trap's description
//

void NXMP_Data::SetTrapDescription(char *pszText)
{
   if (m_pCurrTrap == NULL)
      return;

#ifdef UNICODE
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszText, -1, m_pCurrTrap->szDescription, MAX_DB_STRING);
	m_pCurrTrap->szDescription[MAX_DB_STRING - 1] = 0;
#else
   nx_strncpy(m_pCurrTrap->szDescription, pszText, MAX_DB_STRING);
#endif
}


//
// Set trap's event
//

void NXMP_Data::SetTrapEvent(char *pszEvent)
{
   EVENT_TEMPLATE *pEvent;
#ifdef UNICODE
	WCHAR *pwszEvent;
#endif

   if (m_pCurrTrap == NULL)
      return;

#ifdef UNICODE
	pwszEvent = WideStringFromMBString(pszEvent);
   pEvent = FindEventTemplateByName(pwszEvent);
	free(pwszEvent);
#else
   pEvent = FindEventTemplateByName(pszEvent);
#endif
   if (pEvent != NULL)
	{
		m_pCurrTrap->dwEventCode = pEvent->dwCode;
	}
	else
	{
		//m_pCurrTrap->dwEventCode = 
	}
}


//
// Validate management pack data
//

BOOL NXMP_Data::Validate(DWORD dwFlags, TCHAR *pszErrorText, int nLen)
{
   DWORD i;
   BOOL bRet = FALSE;
   EVENT_TEMPLATE *pEvent;

   // Validate events
   for(i = 0; i < m_dwNumEvents; i++)
   {
      if ((m_pEventList[i].dwCode >= FIRST_USER_EVENT_ID) ||
          (m_pEventList[i].dwCode == 0))
      {
         pEvent = FindEventTemplateByName(m_pEventList[i].szName);
         if (pEvent != NULL)
         {
            if (!(dwFlags & NXMPIF_REPLACE_EVENT_BY_NAME))
            {
               _sntprintf(pszErrorText, nLen, _T("Event with name %s already exist"),
                          m_pEventList[i].szName);
               goto stop_processing;
            }
         }
      }
      else
      {
         pEvent = FindEventTemplateByCode(m_pEventList[i].dwCode);
         if (pEvent != NULL)
         {
            if (!(dwFlags & NXMPIF_REPLACE_EVENT_BY_CODE))
            {
               _sntprintf(pszErrorText, nLen, _T("Event with code %d already exist (existing event name: %s; new event name: %s)"),
                          pEvent->dwCode, pEvent->szName, m_pEventList[i].szName);
               goto stop_processing;
            }
         }
      }
   }

	// Validate traps
	for(i = 0; i < m_dwNumTraps; i++)
	{
	}

   bRet = TRUE;

stop_processing:
   return bRet;
}


//
// Update event in database
//

static BOOL UpdateEvent(EVENT_TEMPLATE *pEvent)
{
   TCHAR szQuery[4096], *pszEscMsg, *pszEscDescr;
   BOOL bEventExist = FALSE;
   DB_RESULT hResult;

   // Check if event with specific code exists
   _stprintf(szQuery, _T("SELECT event_code FROM event_cfg WHERE event_code=%d"), pEvent->dwCode);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bEventExist = TRUE;
      DBFreeResult(hResult);
   }

   // Create or update event template in database
   pszEscMsg = EncodeSQLString(pEvent->pszMessageTemplate);
   pszEscDescr = EncodeSQLString(pEvent->pszDescription);
   if (bEventExist)
   {
      _sntprintf(szQuery, 4096, _T("UPDATE event_cfg SET event_name='%s',severity=%d,flags=%d,message='%s',description='%s' WHERE event_code=%d"),
                 pEvent->szName, pEvent->dwSeverity, pEvent->dwFlags,
                 pszEscMsg, pszEscDescr, pEvent->dwCode);
   }
   else
   {
      _sntprintf(szQuery, 4096, _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,")
                                _T("message,description) VALUES (%d,'%s',%d,%d,'%s','%s')"),
                 pEvent->dwCode, pEvent->szName, pEvent->dwSeverity,
                 pEvent->dwFlags, pszEscMsg, pszEscDescr);
   }
   free(pszEscMsg);
   free(pszEscDescr);
   return DBQuery(g_hCoreDB, szQuery);
}


//
// Install management pack
//

DWORD NXMP_Data::Install(DWORD dwFlags)
{
   DWORD i, dwResult = RCC_SUCCESS;
   BOOL bRet = FALSE;
   EVENT_TEMPLATE *pEvent;

   // Install events
   for(i = 0; i < m_dwNumEvents; i++)
   {
      if ((m_pEventList[i].dwCode >= FIRST_USER_EVENT_ID) ||
          (m_pEventList[i].dwCode == 0))
      {
         pEvent = FindEventTemplateByName(m_pEventList[i].szName);
         if (pEvent != NULL)
         {
            if (!(dwFlags & NXMPIF_REPLACE_EVENT_BY_NAME))
            {
               dwResult = RCC_INTERNAL_ERROR;
               goto stop_processing;
            }
            m_pEventList[i].dwCode = pEvent->dwCode;
         }
         else
         {
            m_pEventList[i].dwCode = CreateUniqueId(IDG_EVENT);
         }
      }
      else
      {
         pEvent = FindEventTemplateByCode(m_pEventList[i].dwCode);
         if ((pEvent != NULL) && (!(dwFlags & NXMPIF_REPLACE_EVENT_BY_CODE)))
         {
            dwResult = RCC_INTERNAL_ERROR;
            goto stop_processing;
         }
      }
      if (!UpdateEvent(&m_pEventList[i]))
      {
         dwResult = RCC_DB_FAILURE;
         goto stop_processing;
      }
   }
   if (m_dwNumEvents > 0)
   {
      ReloadEvents();
      NotifyClientSessions(NX_NOTIFY_EVENTDB_CHANGED, 0);
   }

stop_processing:
   return dwResult;
}
