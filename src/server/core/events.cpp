/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: events.cpp
**
**/

#include "nxcore.h"

/**
 * Global variables
 */
Queue *g_pEventQueue = NULL;
EventPolicy *g_pEventPolicy = NULL;

/**
 * Static data
 */
static UINT32 m_dwNumTemplates = 0;
static EVENT_TEMPLATE *m_pEventTemplates = NULL;
static RWLOCK m_rwlockTemplateAccess;

/**
 * Default constructor for event
 */
Event::Event()
{
   m_qwId = 0;
	m_name[0] = 0;
   m_qwRootId = 0;
   m_dwCode = 0;
   m_severity = SEVERITY_NORMAL;
   m_dwSource = 0;
   m_dwFlags = 0;
   m_pszMessageText = NULL;
   m_pszMessageTemplate = NULL;
   m_tTimeStamp = 0;
	m_pszUserTag = NULL;
	m_pszCustomMessage = NULL;
	m_parameters.setOwner(true);
}

/**
 * Copy constructor for event
 */
Event::Event(Event *src)
{
   m_qwId = src->m_qwId;
   _tcscpy(m_name, src->m_name);
   m_qwRootId = src->m_qwRootId;
   m_dwCode = src->m_dwCode;
   m_severity = src->m_severity;
   m_dwSource = src->m_dwSource;
   m_dwFlags = src->m_dwFlags;
   m_pszMessageText = _tcsdup_ex(src->m_pszMessageText);
   m_pszMessageTemplate = _tcsdup_ex(src->m_pszMessageTemplate);
   m_tTimeStamp = src->m_tTimeStamp;
	m_pszUserTag = _tcsdup_ex(src->m_pszUserTag);
	m_pszCustomMessage = _tcsdup_ex(src->m_pszCustomMessage);
	m_parameters.setOwner(true);
   for(int i = 0; i < src->m_parameters.size(); i++)
   {
      m_parameters.add(_tcsdup_ex((TCHAR *)src->m_parameters.get(i)));
   }
   m_parameterNames.addAll(&src->m_parameterNames);
}

/**
 * Construct event from template
 */
Event::Event(EVENT_TEMPLATE *pTemplate, UINT32 sourceId, const TCHAR *userTag, const char *szFormat, const TCHAR **names, va_list args)
{
	_tcscpy(m_name, pTemplate->szName);
   m_tTimeStamp = time(NULL);
   m_qwId = CreateUniqueEventId();
   m_qwRootId = 0;
   m_dwCode = pTemplate->dwCode;
   m_severity = pTemplate->dwSeverity;
   m_dwFlags = pTemplate->dwFlags;
   m_dwSource = sourceId;
   m_pszMessageText = NULL;
	m_pszUserTag = (userTag != NULL) ? _tcsdup(userTag) : NULL;
   if ((m_pszUserTag != NULL) && (_tcslen(m_pszUserTag) >= MAX_USERTAG_LENGTH))
      m_pszUserTag[MAX_USERTAG_LENGTH - 1] = 0;
	m_pszCustomMessage = NULL;
	m_parameters.setOwner(true);

   // Create parameters
   if (szFormat != NULL)
   {
		int count = (int)strlen(szFormat);
		TCHAR *buffer;

      for(int i = 0; i < count; i++)
      {
         switch(szFormat[i])
         {
            case 's':
               {
                  const TCHAR *s = va_arg(args, const TCHAR *);
					   m_parameters.add(_tcsdup_ex(s));
               }
               break;
            case 'm':	// multibyte string
               {
                  const char *s = va_arg(args, const char *);
#ifdef UNICODE
                  m_parameters.add((s != NULL) ? WideStringFromMBString(s) : _tcsdup(_T("")));
#else
					   m_parameters.add(strdup(CHECK_NULL_EX(s)));
#endif
               }
               break;
            case 'u':	// UNICODE string
               {
                  const WCHAR *s = va_arg(args, const WCHAR *);
#ifdef UNICODE
		   			m_parameters.add(wcsdup(CHECK_NULL_EX(s)));
#else
                  m_parameters.add((s != NULL) ? MBStringFromWideString(s) : strdup(""));
#endif
               }
               break;
            case 'd':
               buffer = (TCHAR *)malloc(16 * sizeof(TCHAR));
               _sntprintf(buffer, 16, _T("%d"), va_arg(args, LONG));
					m_parameters.add(buffer);
               break;
            case 'D':
               buffer = (TCHAR *)malloc(32 * sizeof(TCHAR));
               _sntprintf(buffer, 32, INT64_FMT, va_arg(args, INT64));
					m_parameters.add(buffer);
               break;
            case 't':
               buffer = (TCHAR *)malloc(32 * sizeof(TCHAR));
               _sntprintf(buffer, 32, INT64_FMT, (INT64)va_arg(args, time_t));
					m_parameters.add(buffer);
               break;
            case 'x':
            case 'i':
               buffer = (TCHAR *)malloc(16 * sizeof(TCHAR));
               _sntprintf(buffer, 16, _T("0x%08X"), va_arg(args, UINT32));
					m_parameters.add(buffer);
               break;
            case 'a':   // IPv4 address
               buffer = (TCHAR *)malloc(16 * sizeof(TCHAR));
               IpToStr(va_arg(args, UINT32), buffer);
					m_parameters.add(buffer);
               break;
            case 'A':   // InetAddress object
               buffer = (TCHAR *)malloc(64 * sizeof(TCHAR));
               va_arg(args, InetAddress *)->toString(buffer);
					m_parameters.add(buffer);
               break;
            case 'h':
               buffer = (TCHAR *)malloc(32 * sizeof(TCHAR));
               MACToStr(va_arg(args, BYTE *), buffer);
					m_parameters.add(buffer);
               break;
            default:
               buffer = (TCHAR *)malloc(64 * sizeof(TCHAR));
               _sntprintf(buffer, 64, _T("BAD FORMAT \"%c\" [value = 0x%08X]"), szFormat[i], va_arg(args, UINT32));
					m_parameters.add(buffer);
               break;
         }
			m_parameterNames.add(((names != NULL) && (names[i] != NULL)) ? names[i] : _T(""));
      }
   }

   m_pszMessageTemplate = _tcsdup(pTemplate->pszMessageTemplate);
}

/**
 * Destructor for event
 */
Event::~Event()
{
   safe_free(m_pszMessageText);
   safe_free(m_pszMessageTemplate);
	safe_free(m_pszUserTag);
	safe_free(m_pszCustomMessage);
}

/**
 * Create message text from template
 */
void Event::expandMessageText()
{
   if (m_pszMessageTemplate != NULL)
   {
      if (m_pszMessageText != NULL)
      {
         free(m_pszMessageText);
         m_pszMessageText = NULL;
      }
      m_pszMessageText = expandText(m_pszMessageTemplate);
      free(m_pszMessageTemplate);
      m_pszMessageTemplate = NULL;
   }
}

/**
 * Substitute % macros in given text with actual values
 */
TCHAR *Event::expandText(const TCHAR *textTemplate, const TCHAR *alarmMsg, const TCHAR *alarmKey)
{
	return Event::expandText(this, m_dwSource, textTemplate, alarmMsg, alarmKey);
}

/**
 * Substitute % macros in given text with actual values
 */
TCHAR *Event::expandText(Event *event, UINT32 sourceObject, const TCHAR *textTemplate, const TCHAR *alarmMsg, const TCHAR *alarmKey)
{
   const TCHAR *pCurr;
   UINT32 dwPos, dwSize, dwParam;
	UINT32 sourceId = (event != NULL) ? event->getSourceId() : sourceObject;
   NetObj *pObject;
   struct tm *lt;
   TCHAR *pText, szBuffer[4], scriptName[256];
	int i;

	DbgPrintf(8, _T("Event::expandText(event=%p sourceObject=%d template='%s' alarmMsg='%s' alarmKey='%s')"),
	          event, (int)sourceObject, CHECK_NULL(textTemplate), CHECK_NULL(alarmMsg), CHECK_NULL(alarmKey));

   pObject = FindObjectById(sourceId);
   if (pObject == NULL)
   {
      pObject = FindObjectById(g_dwMgmtNode);
		if (pObject == NULL)
			pObject = g_pEntireNet;
   }
   dwSize = (UINT32)_tcslen(textTemplate) + 1;
   pText = (TCHAR *)malloc(dwSize * sizeof(TCHAR));
   for(pCurr = textTemplate, dwPos = 0; *pCurr != 0; pCurr++)
   {
      switch(*pCurr)
      {
         case '%':   // Metacharacter
            pCurr++;
            if (*pCurr == 0)
            {
               pCurr--;
               break;   // Abnormal loop termination
            }
            switch(*pCurr)
            {
               case '%':
                  pText[dwPos++] = '%';
                  break;
               case 'n':   // Name of event source
                  dwSize += (UINT32)_tcslen(pObject->getName());
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _tcscpy(&pText[dwPos], pObject->getName());
                  dwPos += (UINT32)_tcslen(pObject->getName());
                  break;
               case 'a':   // IP address of event source
                  dwSize += 64;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  GetObjectIpAddress(pObject).toString(&pText[dwPos]);
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'g':   // Source object's GUID
                  dwSize += 36;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  pObject->getGuid().toString(&pText[dwPos]);
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'i':   // Source object identifier in form 0xhhhhhhhh
                  dwSize += 10;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _sntprintf(&pText[dwPos], 11, _T("0x%08X"), sourceId);
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'I':   // Source object identifier in decimal form
                  dwSize += 10;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _sntprintf(&pText[dwPos], 11, _T("%u"), (unsigned int)sourceId);
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 't':   // Event's timestamp
                  dwSize += 32;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
						if (event != NULL)
						{
							lt = localtime(&event->m_tTimeStamp);
						}
						else
						{
							time_t t = time(NULL);
							lt = localtime(&t);
						}
                  _tcsftime(&pText[dwPos], 32, _T("%d-%b-%Y %H:%M:%S"), lt);
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'T':   // Event's timestamp as number of seconds since epoch
                  dwSize += 16;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
						_sntprintf(&pText[dwPos], 16, _T("%lu"), (unsigned long)((event != NULL) ? event->m_tTimeStamp : time(NULL)));
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'c':   // Event code
                  dwSize += 16;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
						_sntprintf(&pText[dwPos], 16, _T("%u"), (unsigned int)((event != NULL) ? event->m_dwCode : 0));
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'N':   // Event name
						if (event != NULL)
						{
							dwSize += (UINT32)_tcslen(event->m_name);
							pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
							_tcscpy(&pText[dwPos], event->m_name);
							dwPos += (UINT32)_tcslen(event->m_name);
						}
                  break;
               case 's':   // Severity code
						if (event != NULL)
						{
							dwSize += 3;
							pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
							_sntprintf(&pText[dwPos], 4, _T("%d"), (int)event->m_severity);
							dwPos = (UINT32)_tcslen(pText);
						}
                  break;
               case 'S':   // Severity text
						if (event != NULL)
						{
                     const TCHAR *statusText = GetStatusAsText(event->m_severity, false);
							dwSize += (UINT32)_tcslen(statusText);
							pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
							_tcscpy(&pText[dwPos], statusText);
							dwPos += (UINT32)_tcslen(statusText);
						}
                  break;
               case 'v':   // NetXMS server version
                  dwSize += (UINT32)_tcslen(NETXMS_VERSION_STRING);
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _tcscpy(&pText[dwPos], NETXMS_VERSION_STRING);
                  dwPos += (UINT32)_tcslen(NETXMS_VERSION_STRING);
                  break;
               case 'm':
                  if ((event != NULL) && (event->m_pszMessageText != NULL))
                  {
                     dwSize += (UINT32)_tcslen(event->m_pszMessageText);
	                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                     _tcscpy(&pText[dwPos], event->m_pszMessageText);
                     dwPos += (UINT32)_tcslen(event->m_pszMessageText);
                  }
                  break;
               case 'M':	// Custom message (usually set by matching script in EPP)
                  if ((event != NULL) && (event->m_pszCustomMessage != NULL))
                  {
                     dwSize += (UINT32)_tcslen(event->m_pszCustomMessage);
	                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                     _tcscpy(&pText[dwPos], event->m_pszCustomMessage);
                     dwPos += (UINT32)_tcslen(event->m_pszCustomMessage);
                  }
                  break;
               case 'A':   // Associated alarm message
                  if (alarmMsg != NULL)
                  {
                     dwSize += (UINT32)_tcslen(alarmMsg);
	                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                     _tcscpy(&pText[dwPos], alarmMsg);
                     dwPos += (UINT32)_tcslen(alarmMsg);
                  }
                  break;
               case 'K':   // Associated alarm key
                  if (alarmKey != NULL)
                  {
                     dwSize += (UINT32)_tcslen(alarmKey);
	                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                     _tcscpy(&pText[dwPos], alarmKey);
                     dwPos += (UINT32)_tcslen(alarmKey);
                  }
                  break;
               case 'u':	// User tag
                  if ((event != NULL) && (event->m_pszUserTag != NULL))
                  {
                     dwSize += (UINT32)_tcslen(event->m_pszUserTag);
	                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                     _tcscpy(&pText[dwPos], event->m_pszUserTag);
                     dwPos += (UINT32)_tcslen(event->m_pszUserTag);
                  }
                  break;
               case '0':
               case '1':
               case '2':
               case '3':
               case '4':
               case '5':
               case '6':
               case '7':
               case '8':
               case '9':
						if (event != NULL)
						{
							szBuffer[0] = *pCurr;
							if (isdigit(*(pCurr + 1)))
							{
								pCurr++;
								szBuffer[1] = *pCurr;
								szBuffer[2] = 0;
							}
							else
							{
								szBuffer[1] = 0;
							}
							dwParam = _tcstoul(szBuffer, NULL, 10);
							if ((dwParam > 0) && (dwParam <= (UINT32)event->m_parameters.size()))
							{
								const TCHAR *value = (TCHAR *)event->m_parameters.get(dwParam - 1);
								if (value == NULL)
									value = _T("");
								dwSize += (UINT32)_tcslen(value);
								pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
								_tcscpy(&pText[dwPos], value);
								dwPos += (UINT32)_tcslen(value);
							}
						}
                  break;
					case '[':	// Script
						for(i = 0, pCurr++; (*pCurr != ']') && (*pCurr != 0) && (i < 255); pCurr++)
						{
							scriptName[i++] = *pCurr;
						}
						if (*pCurr == 0)	// no terminating ]
						{
							pCurr--;
						}
						else
						{
							scriptName[i] = 0;

							// Entry point can be given in form script/entry_point
							TCHAR *entryPoint = _tcschr(scriptName, _T('/'));
							if (entryPoint != NULL)
							{
								*entryPoint = 0;
								entryPoint++;
								StrStrip(entryPoint);
							}
							StrStrip(scriptName);

							NXSL_VM *vm = g_pScriptLibrary->createVM(scriptName, new NXSL_ServerEnv);
							if (vm != NULL)
							{
								if (pObject->getObjectClass() == OBJECT_NODE)
									vm->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, pObject)));
								vm->setGlobalVariable(_T("$event"), (event != NULL) ? new NXSL_Value(new NXSL_Object(&g_nxslEventClass, event)) : new NXSL_Value);
								if (alarmMsg != NULL)
									vm->setGlobalVariable(_T("$alarmMessage"), new NXSL_Value(alarmMsg));
								if (alarmKey != NULL)
									vm->setGlobalVariable(_T("$alarmKey"), new NXSL_Value(alarmKey));

								if (vm->run(0, NULL, NULL, NULL, NULL, entryPoint))
								{
									NXSL_Value *result = vm->getResult();
									if (result != NULL)
									{
										const TCHAR *temp = result->getValueAsCString();
										if (temp != NULL)
										{
											dwSize += (UINT32)_tcslen(temp);
											pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
											_tcscpy(&pText[dwPos], temp);
											dwPos += (UINT32)_tcslen(temp);
											DbgPrintf(4, _T("Event::ExpandText(%d, \"%s\"): Script %s executed successfully"),
												(int)((event != NULL) ? event->m_dwCode : 0), textTemplate, scriptName);
										}
									}
								}
								else
								{
									DbgPrintf(4, _T("Event::ExpandText(%d, \"%s\"): Script %s execution error: %s"),
												 (int)((event != NULL) ? event->m_dwCode : 0), textTemplate, scriptName, vm->getErrorText());
									PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", scriptName, vm->getErrorText(), 0);
								}
							}
							else
							{
								DbgPrintf(4, _T("Event::ExpandText(%d, \"%s\"): Cannot find script %s"),
									(int)((event != NULL) ? event->m_dwCode : 0), textTemplate, scriptName);
							}
						}
						break;
					case '{':	// Custom attribute
						for(i = 0, pCurr++; (*pCurr != '}') && (*pCurr != 0) && (i < 255); pCurr++)
						{
							scriptName[i++] = *pCurr;
						}
						if (*pCurr == 0)	// no terminating }
						{
							pCurr--;
						}
						else
						{
							scriptName[i] = 0;
							StrStrip(scriptName);
							const TCHAR *temp = pObject->getCustomAttribute(scriptName);
							if (temp != NULL)
							{
								dwSize += (UINT32)_tcslen(temp);
								pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
								_tcscpy(&pText[dwPos], temp);
								dwPos += (UINT32)_tcslen(temp);
							}
						}
						break;
					case '<':	// Named parameter
						if (event != NULL)
						{
							for(i = 0, pCurr++; (*pCurr != '>') && (*pCurr != 0) && (i < 255); pCurr++)
							{
								scriptName[i++] = *pCurr;
							}
							if (*pCurr == 0)	// no terminating >
							{
								pCurr--;
							}
							else
							{
								scriptName[i] = 0;
								StrStrip(scriptName);
								int index = event->m_parameterNames.indexOfIgnoreCase(scriptName);
								if (index != -1)
								{
									const TCHAR *temp = (TCHAR *)event->m_parameters.get(index);
									if (temp != NULL)
									{
										dwSize += (UINT32)_tcslen(temp);
										pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
										_tcscpy(&pText[dwPos], temp);
										dwPos += (UINT32)_tcslen(temp);
									}
								}
							}
						}
						break;
               default:    // All other characters are invalid, ignore
                  break;
            }
            break;
         case '\\':  // Escape character
            pCurr++;
            if (*pCurr == 0)
            {
               pCurr--;
               break;   // Abnormal loop termination
            }
            switch(*pCurr)
            {
               case 't':
                  pText[dwPos++] = '\t';
                  break;
               case 'n':
                  pText[dwPos++] = 0x0D;
                  pText[dwPos++] = 0x0A;
                  break;
               default:
                  pText[dwPos++] = *pCurr;
                  break;
            }
            break;
         default:
            pText[dwPos++] = *pCurr;
            break;
      }
   }
   pText[dwPos] = 0;
   return pText;
}

/**
 * Add new parameter to event
 */
void Event::addParameter(const TCHAR *name, const TCHAR *value)
{
	m_parameters.add(_tcsdup(value));
	m_parameterNames.add(name);
}

/**
 * Set value of named parameter
 */
void Event::setNamedParameter(const TCHAR *name, const TCHAR *value)
{
	int index = m_parameterNames.indexOfIgnoreCase(name);
	if (index != -1)
	{
		m_parameters.replace(index, _tcsdup(value));
		m_parameterNames.replace(index, name);
	}
	else
	{
		m_parameters.add(_tcsdup(value));
		m_parameterNames.add(name);
	}
}

/**
 * Set value (and optionally name) of parameter at given index.
 *
 * @param index 0-based parameter index
 * @param name parameter name (can be NULL)
 * @param value new value
 */
void Event::setParameter(int index, const TCHAR *name, const TCHAR *value)
{
   if (index < 0)
      return;

   int addup = index - m_parameters.size();
   for(int i = 0; i < addup; i++)
   {
		m_parameters.add(_tcsdup(_T("")));
		m_parameterNames.add(_T(""));
   }
   if (index < m_parameters.size())
   {
		m_parameters.replace(index, _tcsdup(value));
		m_parameterNames.replace(index, CHECK_NULL_EX(name));
   }
   else
   {
		m_parameters.add(_tcsdup(value));
		m_parameterNames.add(CHECK_NULL_EX(name));
   }
}

/**
 * Fill message with event data
 */
void Event::prepareMessage(NXCPMessage *pMsg)
{
	UINT32 dwId = VID_EVENTLOG_MSG_BASE;

	pMsg->setField(VID_NUM_RECORDS, (UINT32)1);
	pMsg->setField(VID_RECORDS_ORDER, (WORD)RECORD_ORDER_NORMAL);
	pMsg->setField(dwId++, m_qwId);
	pMsg->setField(dwId++, m_dwCode);
	pMsg->setField(dwId++, (UINT32)m_tTimeStamp);
	pMsg->setField(dwId++, m_dwSource);
	pMsg->setField(dwId++, (WORD)m_severity);
	pMsg->setField(dwId++, CHECK_NULL(m_pszMessageText));
	pMsg->setField(dwId++, CHECK_NULL(m_pszUserTag));
	pMsg->setField(dwId++, (UINT32)m_parameters.size());
	for(int i = 0; i < m_parameters.size(); i++)
		pMsg->setField(dwId++, (TCHAR *)m_parameters.get(i));
}

/**
 * Load event configuration from database
 */
static BOOL LoadEvents()
{
   DB_RESULT hResult;
   UINT32 i;
   BOOL bSuccess = FALSE;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   hResult = DBSelect(hdb, _T("SELECT event_code,severity,flags,message,description,event_name FROM event_cfg ORDER BY event_code"));
   if (hResult != NULL)
   {
      m_dwNumTemplates = DBGetNumRows(hResult);
      m_pEventTemplates = (EVENT_TEMPLATE *)malloc(sizeof(EVENT_TEMPLATE) * m_dwNumTemplates);
      for(i = 0; i < m_dwNumTemplates; i++)
      {
         m_pEventTemplates[i].dwCode = DBGetFieldULong(hResult, i, 0);
         m_pEventTemplates[i].dwSeverity = DBGetFieldLong(hResult, i, 1);
         m_pEventTemplates[i].dwFlags = DBGetFieldLong(hResult, i, 2);
         m_pEventTemplates[i].pszMessageTemplate = DBGetField(hResult, i, 3, NULL, 0);
         m_pEventTemplates[i].pszDescription = DBGetField(hResult, i, 4, NULL, 0);
         DBGetField(hResult, i, 5, m_pEventTemplates[i].szName, MAX_EVENT_NAME);
      }

      DBFreeResult(hResult);
      bSuccess = TRUE;
   }
   else
   {
      nxlog_write(MSG_EVENT_LOAD_ERROR, EVENTLOG_ERROR_TYPE, NULL);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return bSuccess;
}

/**
 * Inilialize event handling subsystem
 */
BOOL InitEventSubsystem()
{
   BOOL bSuccess;

   // Create template access mutex
   m_rwlockTemplateAccess = RWLockCreate();

   // Create event queue
   g_pEventQueue = new Queue;

   // Load events from database
   bSuccess = LoadEvents();

   // Create and initialize event processing policy
   if (bSuccess)
   {
      g_pEventPolicy = new EventPolicy;
      if (!g_pEventPolicy->loadFromDB())
      {
         bSuccess = FALSE;
         nxlog_write(MSG_EPP_LOAD_FAILED, EVENTLOG_ERROR_TYPE, NULL);
         delete g_pEventPolicy;
      }
   }

   return bSuccess;
}

/**
 * Shutdown event subsystem
 */
void ShutdownEventSubsystem()
{
   delete g_pEventQueue;
   delete g_pEventPolicy;

   if (m_pEventTemplates != NULL)
   {
      UINT32 i;
      for(i = 0; i < m_dwNumTemplates; i++)
      {
         safe_free(m_pEventTemplates[i].pszDescription);
         safe_free(m_pEventTemplates[i].pszMessageTemplate);
      }
      free(m_pEventTemplates);
   }
   m_dwNumTemplates = 0;
   m_pEventTemplates = NULL;

   RWLockDestroy(m_rwlockTemplateAccess);
}

/**
 * Reload event templates from database
 */
void ReloadEvents()
{
   UINT32 i;

   RWLockWriteLock(m_rwlockTemplateAccess, INFINITE);
   if (m_pEventTemplates != NULL)
   {
      for(i = 0; i < m_dwNumTemplates; i++)
      {
         safe_free(m_pEventTemplates[i].pszDescription);
         safe_free(m_pEventTemplates[i].pszMessageTemplate);
      }
      free(m_pEventTemplates);
   }
   m_dwNumTemplates = 0;
   m_pEventTemplates = NULL;
   LoadEvents();
   RWLockUnlock(m_rwlockTemplateAccess);
}

/**
 * Delete event template from list
 */
void DeleteEventTemplateFromList(UINT32 eventCode)
{
   UINT32 i;

   RWLockWriteLock(m_rwlockTemplateAccess, INFINITE);
   for(i = 0; i < m_dwNumTemplates; i++)
   {
      if (m_pEventTemplates[i].dwCode == eventCode)
      {
         m_dwNumTemplates--;
         safe_free(m_pEventTemplates[i].pszDescription);
         safe_free(m_pEventTemplates[i].pszMessageTemplate);
         memmove(&m_pEventTemplates[i], &m_pEventTemplates[i + 1],
                 sizeof(EVENT_TEMPLATE) * (m_dwNumTemplates - i));
         break;
      }
   }
   RWLockUnlock(m_rwlockTemplateAccess);
}

/**
 * Perform binary search on event template by id
 * Returns INULL if key not found or pointer to appropriate template
 */
static EVENT_TEMPLATE *FindEventTemplate(UINT32 eventCode)
{
   UINT32 dwFirst, dwLast, dwMid;

   dwFirst = 0;
   dwLast = m_dwNumTemplates - 1;

   if ((eventCode < m_pEventTemplates[0].dwCode) || (eventCode > m_pEventTemplates[dwLast].dwCode))
      return NULL;

   while(dwFirst < dwLast)
   {
      dwMid = (dwFirst + dwLast) / 2;
      if (eventCode == m_pEventTemplates[dwMid].dwCode)
         return &m_pEventTemplates[dwMid];
      if (eventCode < m_pEventTemplates[dwMid].dwCode)
         dwLast = dwMid - 1;
      else
         dwFirst = dwMid + 1;
   }

   if (eventCode == m_pEventTemplates[dwLast].dwCode)
      return &m_pEventTemplates[dwLast];

   return NULL;
}

/**
 * Post event to given event queue.
 *
 * @param queue event queue to post events to
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param userTag event's user tag
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address
 *        i - Object ID
 * @param names names for parameters (NULL if parameters are unnamed)
 * @param args event parameters
 */
static BOOL RealPostEvent(Queue *queue, UINT32 eventCode, UINT32 sourceId,
                          const TCHAR *userTag, const char *format, const TCHAR **names, va_list args)
{
   EVENT_TEMPLATE *pEventTemplate;
   Event *pEvent;
   BOOL bResult = FALSE;

   RWLockReadLock(m_rwlockTemplateAccess, INFINITE);

   // Find event template
   if (m_dwNumTemplates > 0)    // Is there any templates?
   {
      pEventTemplate = FindEventTemplate(eventCode);
      if (pEventTemplate != NULL)
      {
         // Template found, create new event
         pEvent = new Event(pEventTemplate, sourceId, userTag, format, names, args);

         // Add new event to queue
         queue->put(pEvent);

         bResult = TRUE;
      }
   }

   RWLockUnlock(m_rwlockTemplateAccess);

   if (pEventTemplate == NULL)
   {
      DbgPrintf(3, _T("RealPostEvent: event with code %d not defined"), eventCode);
   }
   return bResult;
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address
 *        i - Object ID
 *        t - timestamp (time_t) as raw value (seconds since epoch)
 */
BOOL NXCORE_EXPORTABLE PostEvent(UINT32 eventCode, UINT32 sourceId, const char *format, ...)
{
   va_list args;
   BOOL bResult;

   va_start(args, format);
   bResult = RealPostEvent(g_pEventQueue, eventCode, sourceId, NULL, format, NULL, args);
   va_end(args);
   return bResult;
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address
 *        i - Object ID
 * @param names names for parameters (NULL if parameters are unnamed)
 */
BOOL NXCORE_EXPORTABLE PostEventWithNames(UINT32 eventCode, UINT32 sourceId, const char *format, const TCHAR **names, ...)
{
   va_list args;
   BOOL bResult;

   va_start(args, names);
   bResult = RealPostEvent(g_pEventQueue, eventCode, sourceId, NULL, format, names, args);
   va_end(args);
   return bResult;
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address
 *        i - Object ID
 * @param names names for parameters (NULL if parameters are unnamed)
 */
BOOL NXCORE_EXPORTABLE PostEventWithTagAndNames(UINT32 eventCode, UINT32 sourceId, const TCHAR *userTag, const char *format, const TCHAR **names, ...)
{
   va_list args;
   BOOL bResult;

   va_start(args, names);
   bResult = RealPostEvent(g_pEventQueue, eventCode, sourceId, userTag, format, names, args);
   va_end(args);
   return bResult;
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param parameters event parameters list
 */
BOOL NXCORE_EXPORTABLE PostEventWithNames(UINT32 eventCode, UINT32 sourceId, StringMap *parameters)
{
   /*
   int count = parameters->size();
   if (count > 1023)
      count = 1023;

   char format[1024];
   memset(format, 's', count);
   format[count] = 0;

   const TCHAR *names[1024];
   const TCHAR *args[1024];
   for(int i = 0; i < count; i++)
   {
      names[i] = parameters->getKeyByIndex(i);
      args[i] = parameters->getValueByIndex(i);
   }

   return RealPostEvent(g_pEventQueue, eventCode, sourceId, NULL, format, names, args);
   */
   return FALSE;
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param userTag event's user tag
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address
 *        i - Object ID
 * @param names names for parameters (NULL if parameters are unnamed)
 * @param args event parameters
 */
BOOL NXCORE_EXPORTABLE PostEventWithTag(UINT32 eventCode, UINT32 sourceId, const TCHAR *userTag, const char *format, ...)
{
   va_list args;
   BOOL bResult;

   va_start(args, format);
   bResult = RealPostEvent(g_pEventQueue, eventCode, sourceId, userTag, format, NULL, args);
   va_end(args);
   return bResult;
}

/**
 * Post event to given event queue.
 *
 * @param queue event queue to post events to
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address
 *        i - Object ID
 */
BOOL NXCORE_EXPORTABLE PostEventEx(Queue *queue, UINT32 eventCode, UINT32 sourceId, const char *format, ...)
{
   va_list args;
   BOOL bResult;

   va_start(args, format);
   bResult = RealPostEvent(queue, eventCode, sourceId, NULL, format, NULL, args);
   va_end(args);
   return bResult;
}

/**
 * Resend events from specific queue to system event queue
 */
void NXCORE_EXPORTABLE ResendEvents(Queue *queue)
{
   while(1)
   {
      void *pEvent = queue->get();
      if (pEvent == NULL)
         break;
      g_pEventQueue->put(pEvent);
   }
}

/**
 * Create NXMP record for event
 */
void CreateNXMPEventRecord(String &str, UINT32 eventCode)
{
   EVENT_TEMPLATE *p;
   String strText, strDescr;

   RWLockReadLock(m_rwlockTemplateAccess, INFINITE);

   // Find event template
   if (m_dwNumTemplates > 0)    // Is there any templates?
   {
      p = FindEventTemplate(eventCode);
      if (p != NULL)
      {
         str.appendFormattedString(_T("\t\t<event id=\"%d\">\n")
			                       _T("\t\t\t<name>%s</name>\n")
                                _T("\t\t\t<code>%d</code>\n")
                                _T("\t\t\t<severity>%d</severity>\n")
                                _T("\t\t\t<flags>%d</flags>\n")
                                _T("\t\t\t<message>%s</message>\n")
                                _T("\t\t\t<description>%s</description>\n")
                                _T("\t\t</event>\n"),
										  p->dwCode, (const TCHAR *)EscapeStringForXML2(p->szName), p->dwCode, p->dwSeverity,
                                p->dwFlags, (const TCHAR *)EscapeStringForXML2(p->pszMessageTemplate),
										  (const TCHAR *)EscapeStringForXML2(p->pszDescription));
      }
   }

   RWLockUnlock(m_rwlockTemplateAccess);
}

/**
 * Resolve event name
 */
BOOL EventNameFromCode(UINT32 eventCode, TCHAR *pszBuffer)
{
   EVENT_TEMPLATE *p;
   BOOL bRet = FALSE;

   RWLockReadLock(m_rwlockTemplateAccess, INFINITE);

   // Find event template
   if (m_dwNumTemplates > 0)    // Is there any templates?
   {
      p = FindEventTemplate(eventCode);
      if (p != NULL)
      {
         _tcscpy(pszBuffer, p->szName);
         bRet = TRUE;
      }
      else
      {
         _tcscpy(pszBuffer, _T("UNKNOWN_EVENT"));
      }
   }
   else
   {
      _tcscpy(pszBuffer, _T("UNKNOWN_EVENT"));
   }

   RWLockUnlock(m_rwlockTemplateAccess);
   return bRet;
}

/**
 * Find event template by code - suitable for external call
 */
EVENT_TEMPLATE *FindEventTemplateByCode(UINT32 eventCode)
{
   EVENT_TEMPLATE *p = NULL;

   RWLockReadLock(m_rwlockTemplateAccess, INFINITE);
   p = FindEventTemplate(eventCode);
   RWLockUnlock(m_rwlockTemplateAccess);
   return p;
}

/**
 * Find event template by name - suitable for external call
 */
EVENT_TEMPLATE *FindEventTemplateByName(const TCHAR *name)
{
   EVENT_TEMPLATE *p = NULL;
   UINT32 i;

   RWLockReadLock(m_rwlockTemplateAccess, INFINITE);
   for(i = 0; i < m_dwNumTemplates; i++)
   {
      if (!_tcscmp(m_pEventTemplates[i].szName, name))
      {
         p = &m_pEventTemplates[i];
         break;
      }
   }
   RWLockUnlock(m_rwlockTemplateAccess);
   return p;
}

/**
 * Translate event name to code
 * If event with given name does not exist, returns supplied default value
 */
UINT32 NXCORE_EXPORTABLE EventCodeFromName(const TCHAR *name, UINT32 defaultValue)
{
	EVENT_TEMPLATE *p = FindEventTemplateByName(name);
	return (p != NULL) ? p->dwCode : defaultValue;
}

/**
 * Get status as text
 */
const TCHAR NXCORE_EXPORTABLE *GetStatusAsText(int status, bool allCaps)
{
   static const TCHAR *statusText[] = { _T("NORMAL"), _T("WARNING"), _T("MINOR"), _T("MAJOR"), _T("CRITICAL"), _T("UNKNOWN"), _T("UNMANAGED"), _T("DISABLED"), _T("TESTING") };
   static const TCHAR *statusTextSmall[] = { _T("Normal"), _T("Warning"), _T("Minor"), _T("Major"), _T("Critical"), _T("Unknown"), _T("Unmanaged"), _T("Disabled"), _T("Testing") };

   if (allCaps)
   {
      return ((status >= STATUS_NORMAL) && (status <= STATUS_TESTING)) ? statusText[status] : _T("INTERNAL ERROR");
   }
   else
   {
      return ((status >= STATUS_NORMAL) && (status <= STATUS_TESTING)) ? statusTextSmall[status] : _T("INTERNAL ERROR");
   }
}
