/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: events.cpp
**
**/

#include "nxcore.h"
#include <stdarg.h>


//
// Global variables
//

Queue *g_pEventQueue = NULL;
EventPolicy *g_pEventPolicy = NULL;
char *g_szStatusText[] = { "NORMAL", "MINOR", "WARNING", "MAJOR", "CRITICAL", "UNKNOWN", "UNMANAGED", "DISABLED", "TESTING" };
char *g_szStatusTextSmall[] = { "Normal", "Minor", "Warning", "Major", "Critical", "Unknown", "Unmanaged", "Disabled", "Testing" };


//
// Static data
//

static DWORD m_dwNumTemplates = 0;
static EVENT_TEMPLATE *m_pEventTemplates = NULL;
static RWLOCK m_rwlockTemplateAccess;


//
// Default constructor for event
//

Event::Event()
{
   m_dwId = 0;
   m_dwSeverity = 0;
   m_dwSource = 0;
   m_dwFlags = 0;
   m_dwNumParameters = 0;
   m_ppszParameters = NULL;
   m_pszMessageText = NULL;
   m_tTimeStamp = 0;
}


//
// Construct event from template
//

Event::Event(EVENT_TEMPLATE *pTemplate, DWORD dwSourceId, char *szFormat, va_list args)
{
   m_tTimeStamp = time(NULL);
   m_dwId = pTemplate->dwId;
   m_dwSeverity = pTemplate->dwSeverity;
   m_dwFlags = pTemplate->dwFlags;
   m_dwSource = dwSourceId;
   m_pszMessageText = NULL;

   // Create parameters
   if (szFormat != NULL)
   {
      DWORD i;

      m_dwNumParameters = strlen(szFormat);
      m_ppszParameters = (char **)malloc(sizeof(char *) * m_dwNumParameters);

      for(i = 0; i < m_dwNumParameters; i++)
      {
         switch(szFormat[i])
         {
            case 's':
               m_ppszParameters[i] = strdup(va_arg(args, char *));
               break;
            case 'd':
               m_ppszParameters[i] = (char *)malloc(16);
               sprintf(m_ppszParameters[i], "%d", va_arg(args, LONG));
               break;
            case 'x':
            case 'i':
               m_ppszParameters[i] = (char *)malloc(16);
               sprintf(m_ppszParameters[i], "0x%08X", va_arg(args, DWORD));
               break;
            case 'a':
               m_ppszParameters[i] = (char *)malloc(16);
               IpToStr(va_arg(args, DWORD), m_ppszParameters[i]);
               break;
            default:
               m_ppszParameters[i] = (char *)malloc(64);
               sprintf(m_ppszParameters[i], "BAD FORMAT \"%c\" [value = 0x%08X]", szFormat[i], va_arg(args, DWORD));
               break;
         }
      }
   }
   else
   {
      m_dwNumParameters = 0;
      m_ppszParameters = NULL;
   }

   // Create message text from template
   ExpandMessageText(pTemplate->szMessageTemplate);
}


//
// Destructor for event
//

Event::~Event()
{
   if (m_pszMessageText != NULL)
      free(m_pszMessageText);
   if (m_ppszParameters != NULL)
   {
      DWORD i;

      for(i = 0; i < m_dwNumParameters; i++)
         if (m_ppszParameters[i] != NULL)
            free(m_ppszParameters[i]);
      free(m_ppszParameters);
   }
}


//
// Create message text from template
//

void Event::ExpandMessageText(char *szMessageTemplate)
{
   if (m_pszMessageText != NULL)
   {
      free(m_pszMessageText);
      m_pszMessageText = NULL;
   }
   m_pszMessageText = ExpandText(szMessageTemplate);
}


//
// Substitute % macros in given text with actual values
//

char *Event::ExpandText(char *szTemplate)
{
   char *pCurr;
   DWORD dwPos, dwSize, dwParam;
   NetObj *pObject;
   struct tm *lt;
   char *pText, szBuffer[4];

   pObject = FindObjectById(m_dwSource);
   if (pObject == NULL)    // Normally shouldn't happen
   {
      pObject = g_pEntireNet;
   }
   dwSize = strlen(szTemplate) + 1;
   pText = (char *)malloc(dwSize);
   for(pCurr = szTemplate, dwPos = 0; *pCurr != 0; pCurr++)
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
                  dwSize += strlen(pObject->Name());
                  pText = (char *)realloc(pText, dwSize);
                  strcpy(&pText[dwPos], pObject->Name());
                  dwPos += strlen(pObject->Name());
                  break;
               case 'a':   // IP address of event source
                  dwSize += 16;
                  pText = (char *)realloc(pText, dwSize);
                  IpToStr(pObject->IpAddr(), &pText[dwPos]);
                  dwPos = strlen(pText);
                  break;
               case 'i':   // Source object identifier
                  dwSize += 10;
                  pText = (char *)realloc(pText, dwSize);
                  sprintf(&pText[dwPos], "0x%08X", m_dwSource);
                  dwPos = strlen(pText);
                  break;
               case 't':   // Event's timestamp
                  dwSize += 32;
                  pText = (char *)realloc(pText, dwSize);
                  lt = localtime(&m_tTimeStamp);
                  strftime(&pText[dwPos], 32, "%d-%b-%Y %H:%M:%S", lt);
                  dwPos = strlen(pText);
                  break;
               case 'c':   // Event code
                  dwSize += 16;
                  pText = (char *)realloc(pText, dwSize);
                  sprintf(&pText[dwPos], "%lu", m_dwId);
                  dwPos = strlen(pText);
                  break;
               case 's':   // Severity code
                  dwSize += 3;
                  pText = (char *)realloc(pText, dwSize);
                  sprintf(&pText[dwPos], "%d", (int)m_dwSeverity);
                  dwPos = strlen(pText);
                  break;
               case 'S':   // Severity text
                  dwSize += strlen(g_szStatusTextSmall[m_dwSeverity]);
                  pText = (char *)realloc(pText, dwSize);
                  strcpy(&pText[dwPos], g_szStatusTextSmall[m_dwSeverity]);
                  dwPos += strlen(g_szStatusTextSmall[m_dwSeverity]);
                  break;
               case 'v':   // NetXMS server version
                  dwSize += strlen(NETXMS_VERSION_STRING);
                  pText = (char *)realloc(pText, dwSize);
                  strcpy(&pText[dwPos], NETXMS_VERSION_STRING);
                  dwPos += strlen(NETXMS_VERSION_STRING);
                  break;
               case 'm':
                  if (m_pszMessageText != NULL)
                  {
                     dwSize += strlen(m_pszMessageText);
                     pText = (char *)realloc(pText, dwSize);
                     strcpy(&pText[dwPos], m_pszMessageText);
                     dwPos += strlen(m_pszMessageText);
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
                  dwParam = atol(szBuffer);
                  if ((dwParam > 0) && (dwParam <= m_dwNumParameters))
                  {
                     dwParam--;
                     dwSize += strlen(m_ppszParameters[dwParam]);
                     pText = (char *)realloc(pText, dwSize);
                     strcpy(&pText[dwPos], m_ppszParameters[dwParam]);
                     dwPos += strlen(m_ppszParameters[dwParam]);
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


//
// Fill NXC_EVENT structure with event data
//

void Event::PrepareMessage(NXC_EVENT *pEventData)
{
   pEventData->dwTimeStamp = htonl(m_tTimeStamp);
   pEventData->dwEventId = htonl(m_dwId);
   pEventData->dwSeverity = htonl(m_dwSeverity);
   pEventData->dwSourceId = htonl(m_dwSource);
   strcpy(pEventData->szMessage, CHECK_NULL(m_pszMessageText));
}


//
// Load events from database
//

static BOOL LoadEvents(void)
{
   DB_RESULT hResult;
   DWORD i;
   BOOL bSuccess = FALSE;

   hResult = DBSelect(g_hCoreDB, "SELECT event_id,severity,flags,message,description FROM events ORDER BY event_id");
   if (hResult != NULL)
   {
      m_dwNumTemplates = DBGetNumRows(hResult);
      m_pEventTemplates = (EVENT_TEMPLATE *)malloc(sizeof(EVENT_TEMPLATE) * m_dwNumTemplates);
      for(i = 0; i < m_dwNumTemplates; i++)
      {
         m_pEventTemplates[i].dwId = DBGetFieldULong(hResult, i, 0);
         m_pEventTemplates[i].dwSeverity = DBGetFieldLong(hResult, i, 1);
         m_pEventTemplates[i].dwFlags = DBGetFieldLong(hResult, i, 2);
         m_pEventTemplates[i].szMessageTemplate = strdup(DBGetField(hResult, i, 3));
         DecodeSQLString(m_pEventTemplates[i].szMessageTemplate);
         m_pEventTemplates[i].szDescription = strdup(DBGetField(hResult, i, 4));
         DecodeSQLString(m_pEventTemplates[i].szDescription);
      }

      DBFreeResult(hResult);
      bSuccess = TRUE;
   }
   else
   {
      WriteLog(MSG_EVENT_LOAD_ERROR, EVENTLOG_ERROR_TYPE, NULL);
   }

   return bSuccess;
}


//
// Inilialize event handling subsystem
//

BOOL InitEventSubsystem(void)
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
      if (!g_pEventPolicy->LoadFromDB())
      {
         bSuccess = FALSE;
         WriteLog(MSG_EPP_LOAD_FAILED, EVENTLOG_ERROR_TYPE, NULL);
         delete g_pEventPolicy;
      }
   }

   return bSuccess;
}


//
// Shutdown event subsystem
//

void ShutdownEventSubsystem(void)
{
   delete g_pEventQueue;
   delete g_pEventPolicy;
   safe_free(m_pEventTemplates);
   RWLockDestroy(m_rwlockTemplateAccess);
}


//
// Reload event templates from database
//

void ReloadEvents(void)
{
   RWLockWriteLock(m_rwlockTemplateAccess, INFINITE);
   if (m_pEventTemplates != NULL)
      free(m_pEventTemplates);
   m_dwNumTemplates = 0;
   m_pEventTemplates = NULL;
   LoadEvents();
   RWLockUnlock(m_rwlockTemplateAccess);
}


//
// Perform binary search on event template by id
// Returns INULL if key not found or pointer to appropriate template
//

static EVENT_TEMPLATE *FindEventTemplate(DWORD dwId)
{
   DWORD dwFirst, dwLast, dwMid;

   dwFirst = 0;
   dwLast = m_dwNumTemplates - 1;

   if ((dwId < m_pEventTemplates[0].dwId) || (dwId > m_pEventTemplates[dwLast].dwId))
      return NULL;

   while(dwFirst < dwLast)
   {
      dwMid = (dwFirst + dwLast) / 2;
      if (dwId == m_pEventTemplates[dwMid].dwId)
         return &m_pEventTemplates[dwMid];
      if (dwId < m_pEventTemplates[dwMid].dwId)
         dwLast = dwMid - 1;
      else
         dwFirst = dwMid + 1;
   }

   if (dwId == m_pEventTemplates[dwLast].dwId)
      return &m_pEventTemplates[dwLast];

   return NULL;
}


//
// Post event to the queue.
// Arguments:
// dwEventId  - Event ID
// dwSourceId - Event source object ID
// szFormat   - Parameter format string, each parameter represented by one character.
//    The following format characters can be used:
//        s - String
//        d - Decimal integer
//        x - Hex integer
//        a - IP address
//        i - Object ID
//

BOOL PostEvent(DWORD dwEventId, DWORD dwSourceId, char *szFormat, ...)
{
   EVENT_TEMPLATE *pEventTemplate;
   Event *pEvent;
   va_list args;
   BOOL bResult = FALSE;

   RWLockReadLock(m_rwlockTemplateAccess, INFINITE);

   // Find event template
   if (m_dwNumTemplates > 0)    // Is there any templates?
   {
      pEventTemplate = FindEventTemplate(dwEventId);
      if (pEventTemplate != NULL)
      {
         // Template found, create new event
         va_start(args, szFormat);
         pEvent = new Event(pEventTemplate, dwSourceId, szFormat, args);
         va_end(args);

         // Add new event to queue
         g_pEventQueue->Put(pEvent);

         bResult = TRUE;
      }
   }

   RWLockUnlock(m_rwlockTemplateAccess);
   return bResult;
}
