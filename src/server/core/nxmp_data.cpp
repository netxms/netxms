/* $Id$ */
/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
   
   nLen = (int)strlen(pszStr);
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

   m_pTrapList = NULL;
   m_dwNumTraps = 0;
   m_pCurrTrap = NULL;

	m_ppTemplateList = NULL;
	m_dwNumTemplates = 0;
	m_pCurrTemplate = NULL;

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

   for(i = 0; i < m_dwNumTraps; i++)
   {
      safe_free(m_pTrapList[i].pdwObjectId);
   }
   safe_free(m_pTrapList);

	for(i = 0; i < m_dwNumTemplates; i++)
	{
		delete m_ppTemplateList[i];
	}
	safe_free(m_ppTemplateList);
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
	char *eptr;
	int nVal;
	static TCHAR szNotNumberDCI[] = _T("DCI attribute %s must have numeric value");
	static TCHAR szNotNumberThreshold[] = _T("Threshold attribute %s must have numeric value");

   DbgPrintf(6, _T("NXMP_Parse: ParseVariable(\"%s\",\"%s\") - CONTEXT=%d"), pszName, pszValue, m_nContext);
   
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
      case CTX_TRAP:
         if (!stricmp(pszName, "EVENT"))
         {
            SetEvent(pszValue, NXMP_SETEVENT_TRAP);
         }
         else if (!stricmp(pszName, "DESCRIPTION"))
         {
            SetTrapDescription(pszValue);
         }
         else if (!stricmp(pszName, "USERTAG"))
         {
            SetTrapUserTag(pszValue);
         }
         else
         {
            Error("Unknown trap attribute %s", pszName);
         }
         break;
		case CTX_DCI:
         if (!stricmp(pszName, "NAME"))
         {
            m_pCurrDCI->SetName(pszValue);
         }
         else if (!stricmp(pszName, "DESCRIPTION"))
         {
            m_pCurrDCI->SetDescription(pszValue);
         }
         else if (!stricmp(pszName, "INSTANCE"))
         {
            m_pCurrDCI->SetInstance(pszValue);
         }
         else if (!stricmp(pszName, "SCRIPT"))
         {
            m_pCurrDCI->SetTransformationScript(pszValue);
         }
         else if (!stricmp(pszName, "DATATYPE"))
         {
				nVal = strtol(pszValue, &eptr, 0);
				if (*eptr == 0)
				{
					m_pCurrDCI->SetDataType(nVal);
				}
				else
				{
					Error(szNotNumberDCI, pszName);
				}
         }
         else if (!stricmp(pszName, "ORIGIN"))
         {
				nVal = strtol(pszValue, &eptr, 0);
				if (*eptr == 0)
				{
					m_pCurrDCI->SetOrigin(nVal);
				}
				else
				{
					Error(szNotNumberDCI, pszName);
				}
         }
         else if (!stricmp(pszName, "INTERVAL"))
         {
				nVal = strtol(pszValue, &eptr, 0);
				if (*eptr == 0)
				{
					m_pCurrDCI->SetInterval(nVal);
				}
				else
				{
					Error(szNotNumberDCI, pszName);
				}
         }
         else if (!stricmp(pszName, "RETENTION"))
         {
				nVal = strtol(pszValue, &eptr, 0);
				if (*eptr == 0)
				{
					m_pCurrDCI->SetRetentionTime(nVal);
				}
				else
				{
					Error(szNotNumberDCI, pszName);
				}
         }
         else if (!stricmp(pszName, "ALL_THRESHOLDS"))
         {
				nVal = strtol(pszValue, &eptr, 0);
				if (*eptr == 0)
				{
					m_pCurrDCI->SetAllThresholdsFlag(nVal);
				}
				else
				{
					Error(szNotNumberDCI, pszName);
				}
         }
         else if (!stricmp(pszName, "DELTA"))
         {
				nVal = strtol(pszValue, &eptr, 0);
				if (*eptr == 0)
				{
					m_pCurrDCI->SetDeltaCalcMethod(nVal);
				}
				else
				{
					Error(szNotNumberDCI, pszName);
				}
         }
         else if (!stricmp(pszName, "ADVANCED_SCHEDULE"))
         {
				nVal = strtol(pszValue, &eptr, 0);
				if (*eptr == 0)
				{
					m_pCurrDCI->SetAdvScheduleFlag(nVal);
				}
				else
				{
					Error(szNotNumberDCI, pszName);
				}
         }
         else
         {
            Error("Unknown DCI attribute %s", pszName);
         }
			break;
		case CTX_THRESHOLD:
         if (!stricmp(pszName, "FUNCTION"))
         {
				nVal = strtol(pszValue, &eptr, 0);
				if (*eptr == 0)
				{
					m_pCurrThreshold->setFunction(nVal);
				}
				else
				{
					Error(szNotNumberThreshold, pszName);
				}
         }
         else if (!stricmp(pszName, "CONDITION"))
         {
				nVal = strtol(pszValue, &eptr, 0);
				if (*eptr == 0)
				{
					m_pCurrThreshold->setOperation(nVal);
				}
				else
				{
					Error(szNotNumberThreshold, pszName);
				}
         }
         else if (!stricmp(pszName, "VALUE"))
         {
				m_pCurrThreshold->setValue(pszValue);
         }
         else if (!stricmp(pszName, "ACTIVATION_EVENT"))
         {
            SetEvent(pszValue, NXMP_SETEVENT_THRESHOLD_ACTIVATE);
         }
         else if (!stricmp(pszName, "DEACTIVATION_EVENT"))
         {
            SetEvent(pszValue, NXMP_SETEVENT_THRESHOLD_DEACTIVATE);
         }
         else if (!stricmp(pszName, "PARAM1"))
         {
				nVal = strtol(pszValue, &eptr, 0);
				if (*eptr == 0)
				{
					m_pCurrThreshold->setParam1(nVal);
				}
				else
				{
					Error(szNotNumberThreshold, pszName);
				}
         }
         else if (!stricmp(pszName, "PARAM2"))
         {
				nVal = strtol(pszValue, &eptr, 0);
				if (*eptr == 0)
				{
					m_pCurrThreshold->setParam2(nVal);
				}
				else
				{
					Error(szNotNumberThreshold, pszName);
				}
         }
         else
         {
            Error("Unknown threshold attribute %s", pszName);
         }
			break;
      default:
         break;
   }
   return bRet;
}


//
// Set event for target (trap, threshold, etc.)
//

void NXMP_Data::SetEvent(char *pszEvent, int nTarget)
{
   EVENT_TEMPLATE *pEvent;
	DWORD dwEventCode = 0;
#ifdef UNICODE
	WCHAR *pwszEvent;
#endif

#ifdef UNICODE
	pwszEvent = WideStringFromMBString(pszEvent);
   pEvent = FindEventTemplateByName(pwszEvent);
	free(pwszEvent);
#else
   pEvent = FindEventTemplateByName(pszEvent);
#endif
   if (pEvent != NULL)
	{
		dwEventCode = pEvent->dwCode;
	}
	else
	{
		DWORD dwIndex;

#ifdef UNICODE
		dwIndex = FindEvent(pwszEvent);
#else
		dwIndex = FindEvent(pszEvent);
#endif
		if (dwIndex != INVALID_INDEX)
		{
			// We set group bit to indicate that event code is not an
			// actual event code but index of a new event (coming in the same pack)
			dwEventCode = dwIndex | GROUP_FLAG;
		}
	}
#ifdef UNICODE
	free(pwszEvent);
#endif

	switch(nTarget)
	{
		case NXMP_SETEVENT_TRAP:
			if (m_pCurrTrap != NULL)
				m_pCurrTrap->dwEventCode = dwEventCode;
			break;
		case NXMP_SETEVENT_THRESHOLD_ACTIVATE:
			if (m_pCurrThreshold != NULL)
				m_pCurrThreshold->setEvent(dwEventCode);
			break;
		case NXMP_SETEVENT_THRESHOLD_DEACTIVATE:
			if (m_pCurrThreshold != NULL)
				m_pCurrThreshold->setRearmEvent(dwEventCode);
			break;
		default:
			break;
	}
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
   DbgPrintf(6, _T("NXMP_Parse: NewEvent(\"%s\")"), pszName);
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
// Find event by name and return it's index or INVALID_INDEX on error
//

DWORD NXMP_Data::FindEvent(TCHAR *pszName)
{
	DWORD i;

	for(i = 0; i < m_dwNumEvents; i++)
	{
		if (!_tcsicmp(pszName, m_pEventList[i].szName))
			return i;
	}
	return INVALID_INDEX;
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
// Set trap's user tag
//

void NXMP_Data::SetTrapUserTag(char *pszText)
{
   if (m_pCurrTrap == NULL)
      return;

#ifdef UNICODE
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszText, -1, m_pCurrTrap->szUserTag, MAX_USERTAG_LENGTH);
	m_pCurrTrap->szUserTag[MAX_USERTAG_LENGTH - 1] = 0;
#else
   nx_strncpy(m_pCurrTrap->szUserTag, pszText, MAX_USERTAG_LENGTH);
#endif
}


//
// Add new trap parameter mapping
//

void NXMP_Data::AddTrapParam(char *pszOID, int nPos, char *pszDescr)
{
	DWORD dwIndex, dwOID[MAX_OID_LEN];
#ifdef UNICODE
	WCHAR *pwszOID;
#endif

   if (m_pCurrTrap == NULL)
      return;

	dwIndex = m_pCurrTrap->dwNumMaps;
	m_pCurrTrap->dwNumMaps++;
	m_pCurrTrap->pMaps = (NXC_OID_MAP *)realloc(m_pCurrTrap->pMaps, sizeof(NXC_OID_MAP) * m_pCurrTrap->dwNumMaps);
	memset(&m_pCurrTrap->pMaps[dwIndex], 0, sizeof(NXC_OID_MAP));
	if (pszOID != NULL)
	{
#ifdef UNICODE
		pwszOID = WideStringFromMBString(pszOID);
		m_pCurrTrap->pMaps[dwIndex].dwOidLen = SNMPParseOID(pwszOID, dwOID, MAX_OID_LEN);
		free(pwszOID);
#else
		m_pCurrTrap->pMaps[dwIndex].dwOidLen = SNMPParseOID(pszOID, dwOID, MAX_OID_LEN);
#endif
		m_pCurrTrap->pMaps[dwIndex].pdwObjectId = (DWORD *)nx_memdup(dwOID, m_pCurrTrap->pMaps[dwIndex].dwOidLen * sizeof(DWORD));
	}
	else
	{
		m_pCurrTrap->pMaps[dwIndex].dwOidLen = (DWORD)nPos | 0x80000000;
	}
#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszDescr, -1,
	                    m_pCurrTrap->pMaps[dwIndex].szDescription, MAX_DB_STRING);
	m_pCurrTrap->pMaps[dwIndex].szDescription[MAX_DB_STRING - 1] = 0;
#else
	nx_strncpy(m_pCurrTrap->pMaps[dwIndex].szDescription, pszDescr, MAX_DB_STRING);
#endif
}


//
// Create new template
//

void NXMP_Data::NewTemplate(char *pszName)
{
   m_dwNumTemplates++;
   m_ppTemplateList = (Template **)realloc(m_ppTemplateList, sizeof(Template *) * m_dwNumTemplates);
   m_pCurrTemplate = new Template;
	m_pCurrTemplate->Hide();
	m_ppTemplateList[m_dwNumTemplates - 1] = m_pCurrTemplate;
#ifdef UNICODE
	WCHAR wszName[MAX_OBJECT_NAME];

   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszName, -1, wszName, MAX_OBJECT_NAME - 1);
	m_pCurrTemplate->SetName(wszName);
#else
	m_pCurrTemplate->SetName(pszName);
#endif
   m_nContext = CTX_TEMPLATE;
	DbgPrintf(6, _T("NXMP_Data::NewTemplate(): new template \"%s\" created"), m_pCurrTemplate->Name());
}


//
// Create new DCI in template
//

void NXMP_Data::NewDCI(void)
{
	m_pCurrDCI = new DCItem;
   m_nContext = CTX_DCI;
}


//
// Add DCI schedule
//

void NXMP_Data::AddDCISchedule(char *pszStr)
{
	if (m_pCurrDCI != NULL)
		m_pCurrDCI->AddSchedule(pszStr);
}


//
// Close DCI
//

void NXMP_Data::CloseDCI(void)
{
	m_pCurrDCI->FinishMPParsing();
	m_pCurrTemplate->AddItem(m_pCurrDCI);
   m_nContext = CTX_TEMPLATE;
}


//
// Create new threshold in DCI
//

void NXMP_Data::NewThreshold(void)
{
	m_pCurrThreshold = new Threshold;
   m_nContext = CTX_THRESHOLD;
}


//
// Close DCI
//

void NXMP_Data::CloseThreshold(void)
{
	m_pCurrDCI->AddThreshold(m_pCurrThreshold);
   m_nContext = CTX_DCI;
}


//
// Validate threshold
//

BOOL NXMP_Data::ValidateThreshold(Threshold *pThreshold, DWORD dwIndex)
{
	if (pThreshold->getEventCode() == 0)
	{
		_sntprintf(m_pszValidationErrorText, m_nMaxValidationErrorLen,
		           _T("Template %s DCI %d threshold %d attribute \"activation event\" refers to unknown event"),
					  m_pCurrTemplate->Name(), m_dwCurrDCIIndex, dwIndex);
		return FALSE;
	}
	if (pThreshold->getRearmEventCode() == 0)
	{
		_sntprintf(m_pszValidationErrorText, m_nMaxValidationErrorLen,
		           _T("Template %s DCI %d threshold %d attribute \"deactivation event\" refers to unknown event"),
					  m_pCurrTemplate->Name(), m_dwCurrDCIIndex, dwIndex);
		return FALSE;
	}
	return TRUE;
}


//
// Threshold validation callback
//

static BOOL ThresholdValidationCallback(Threshold *pThreshold, DWORD dwIndex, void *pArg)
{
	return ((NXMP_Data *)pArg)->ValidateThreshold(pThreshold, dwIndex);
}


//
// DCI validation callback
//

static BOOL DCIValidationCallback(DCItem *pItem, DWORD dwIndex, void *pArg)
{
	((NXMP_Data *)pArg)->SetCurrDCIIndex(dwIndex);
	return pItem->EnumThresholds(ThresholdValidationCallback, pArg);
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
		if (m_pTrapList[i].dwEventCode == 0)
		{
         _sntprintf(pszErrorText, nLen, _T("Trap \"%s\" references unknown event"),
                    m_pTrapList[i].szDescription);
         goto stop_processing;
		}
	}

	// Validate templates
	m_pszValidationErrorText = pszErrorText;
	m_nMaxValidationErrorLen = nLen;
	for(i = 0; i < m_dwNumTemplates; i++)
	{
		m_pCurrTemplate = m_ppTemplateList[i];
		if (!m_ppTemplateList[i]->EnumDCI(DCIValidationCallback, this))
			goto stop_processing;
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
// Validate threshold
//

BOOL NXMP_Data::ResolveEventsForThreshold(Threshold *pThreshold, DWORD dwIndex)
{
	if (pThreshold->getEventCode() & GROUP_FLAG)
	{
		// Correct event code should be set already at event installation phase
		pThreshold->setEvent(m_pEventList[pThreshold->getEventCode() & ~GROUP_FLAG].dwCode);
	}
	if (pThreshold->getRearmEventCode() & GROUP_FLAG)
	{
		// Correct event code should be set already at event installation phase
		pThreshold->setRearmEvent(m_pEventList[pThreshold->getRearmEventCode() & ~GROUP_FLAG].dwCode);
	}
	return TRUE;
}


//
// Threshold event resolve callback
//

static BOOL ThresholdEventResolveCallback(Threshold *pThreshold, DWORD dwIndex, void *pArg)
{
	return ((NXMP_Data *)pArg)->ResolveEventsForThreshold(pThreshold, dwIndex);
}


//
// DCI event resolve callback
//

static BOOL DCIEventResolveCallback(DCItem *pItem, DWORD dwIndex, void *pArg)
{
	((NXMP_Data *)pArg)->SetCurrDCIIndex(dwIndex);
	return pItem->EnumThresholds(ThresholdEventResolveCallback, pArg);
}


//
// Install management pack
//

DWORD NXMP_Data::Install(DWORD dwFlags)
{
   DWORD i, dwResult = RCC_SUCCESS;
   BOOL bRet = FALSE;
   EVENT_TEMPLATE *pEvent;
   
   DbgPrintf(4, _T("NXMP: Installing management pack"));

   // Install events
   DbgPrintf(4, _T("NXMP: %d events to install"), m_dwNumEvents);
   for(i = 0; i < m_dwNumEvents; i++)
   {
	   DbgPrintf(5, _T("NXMP: Installing event %d (%s)"), m_pEventList[i].dwCode, m_pEventList[i].szName);
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
   DbgPrintf(4, _T("NXMP: Events installed"), m_dwNumEvents);

	// Install traps
   DbgPrintf(4, _T("NXMP: %d traps to install"), m_dwNumTraps);
	for(i = 0; i < m_dwNumTraps; i++)
	{
		// Resolve event name if needed
		if (m_pTrapList[i].dwEventCode & GROUP_FLAG)
		{
			// Correct event code should be set already at event installation phase
			m_pTrapList[i].dwEventCode = m_pEventList[m_pTrapList[i].dwEventCode & ~GROUP_FLAG].dwCode;
		}
		dwResult = CreateNewTrap(&m_pTrapList[i]);
		if (dwResult != RCC_SUCCESS)
			goto stop_processing;
	}
   DbgPrintf(4, _T("NXMP: Traps installed"), m_dwNumEvents);

	// Install templates
   DbgPrintf(4, _T("NXMP: %d templates to install"), m_dwNumTemplates);
	for(i = 0; i < m_dwNumTemplates; i++)
	{
		// Resolve events
		m_pCurrTemplate = m_ppTemplateList[i];
		m_ppTemplateList[i]->EnumDCI(DCIEventResolveCallback, this);

		m_ppTemplateList[i]->AssociateItems();
		NetObjInsert(m_ppTemplateList[i], TRUE);
		m_ppTemplateList[i]->AddParent(g_pTemplateRoot);
		g_pTemplateRoot->AddChild(m_ppTemplateList[i]);
		m_ppTemplateList[i]->Unhide();
		m_ppTemplateList[i] = NULL;	// Prevent object deletion by NXMP_Data destructor
	}
   DbgPrintf(4, _T("NXMP: Templates installed"), m_dwNumEvents);

stop_processing:
   DbgPrintf(4, _T("NXMP: management pack installation finished, RCC=%d"), dwResult);
   return dwResult;
}
