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

#include "nms_core.h"
#include <stdarg.h>


//
// Global variables
//

Queue *g_pEventQueue = NULL;


//
// Static data
//

static DWORD m_dwNumTemplates = 0;
static EVENT_TEMPLATE *m_pEventTemplates = NULL;


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
   m_pszParameters = NULL;
   m_szMessageText = NULL;
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
   if (dwSourceId == 0)
   {
      // Local node
      m_dwSource = FindLocalMgmtNode();
   }
   else
   {
      m_dwSource = dwSourceId;
   }

   // Create parameters
   if (szFormat != NULL)
   {
      DWORD i;

      m_dwNumParameters = strlen(szFormat);
      m_pszParameters = (char **)malloc(sizeof(char *) * m_dwNumParameters);

      for(i = 0; i < m_dwNumParameters; i++)
      {
         switch(szFormat[i])
         {
            case 's':
               m_pszParameters[i] = strdup(va_arg(args, char *));
               break;
            case 'd':
               m_pszParameters[i] = (char *)malloc(16);
               sprintf(m_pszParameters[i], "%d", va_arg(args, LONG));
               break;
            case 'x':
               m_pszParameters[i] = (char *)malloc(16);
               sprintf(m_pszParameters[i], "0x%08X", va_arg(args, DWORD));
               break;
            case 'a':
               m_pszParameters[i] = (char *)malloc(16);
               IpToStr(va_arg(args, DWORD), m_pszParameters[i]);
               break;
            default:
               m_pszParameters[i] = (char *)malloc(64);
               sprintf(m_pszParameters[i], "BAD FORMAT \"%c\" [value = 0x%08X]", szFormat[i], va_arg(args, DWORD));
               break;
         }
      }
   }
   else
   {
      m_dwNumParameters = 0;
      m_pszParameters = NULL;
   }

   // Create message text from template
   ExpandMessageText(pTemplate->szMessageTemplate);
}


//
// Destructor for event
//

Event::~Event()
{
   if (m_szMessageText != NULL)
      free(m_szMessageText);
   if (m_pszParameters != NULL)
   {
      DWORD i;

      for(i = 0; i < m_dwNumParameters; i++)
         if (m_pszParameters[i] != NULL)
            free(m_pszParameters[i]);
      free(m_pszParameters);
   }
}


//
// Create message text from template
//

void Event::ExpandMessageText(char *szMessageTemplate)
{
   char *pCurr;
   DWORD dwPos, dwSize, dwParam;
   NetObj *pObject;
   struct tm *lt;
   char szBuffer[4];

   pObject = FindObjectById(m_dwSource);
   if (pObject == NULL)    // Normally shouldn't happen
   {
      pObject = g_pEntireNet;
   }
   dwSize = strlen(szMessageTemplate) + 1;
   m_szMessageText = (char *)malloc(dwSize);
   for(pCurr = szMessageTemplate, dwPos = 0; *pCurr != 0; pCurr++)
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
                  m_szMessageText[dwPos++] = '%';
                  break;
               case 'n':   // Name of event source
                  dwSize += strlen(pObject->Name());
                  m_szMessageText = (char *)realloc(m_szMessageText, dwSize);
                  strcpy(&m_szMessageText[dwPos], pObject->Name());
                  dwPos += strlen(pObject->Name());
                  break;
               case 'a':   // IP address of event source
                  dwSize += 16;
                  m_szMessageText = (char *)realloc(m_szMessageText, dwSize);
                  IpToStr(pObject->IpAddr(), &m_szMessageText[dwPos]);
                  dwPos = strlen(m_szMessageText);
                  break;
               case 'i':   // Source object identifier
                  dwSize += 10;
                  m_szMessageText = (char *)realloc(m_szMessageText, dwSize);
                  sprintf(&m_szMessageText[dwPos], "0x%08X", m_dwSource);
                  dwPos = strlen(m_szMessageText);
                  break;
               case 't':
                  dwSize += 32;
                  m_szMessageText = (char *)realloc(m_szMessageText, dwSize);
                  lt = localtime(&m_tTimeStamp);
                  strftime(&m_szMessageText[dwPos], 32, "%d-%b-%Y %H:%M:%S", lt);
                  dwPos = strlen(m_szMessageText);
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
                     dwSize += strlen(m_pszParameters[dwParam]);
                     m_szMessageText = (char *)realloc(m_szMessageText, dwSize);
                     strcpy(&m_szMessageText[dwPos], m_pszParameters[dwParam]);
                     dwPos += strlen(m_pszParameters[dwParam]);
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
            m_szMessageText[dwPos++] = *pCurr;
            break;
         default:
            m_szMessageText[dwPos++] = *pCurr;
            break;
      }
   }
   m_szMessageText[dwPos] = 0;
}


//
// Inilialize event handling subsystem
//

BOOL InitEventSubsystem(void)
{
   DB_RESULT hResult;
   DWORD i;

   // Create event queue
   g_pEventQueue = new Queue;

   // Load events from database
   hResult = DBSelect(g_hCoreDB, "SELECT id,severity,flags,message,description FROM events ORDER BY id");
   if (hResult == 0)
   {
      WriteLog(MSG_EVENT_LOAD_ERROR, EVENTLOG_ERROR_TYPE, NULL);
      return FALSE;
   }

   m_dwNumTemplates = DBGetNumRows(hResult);
   m_pEventTemplates = (EVENT_TEMPLATE *)malloc(sizeof(EVENT_TEMPLATE) * m_dwNumTemplates);
   for(i = 0; i < m_dwNumTemplates; i++)
   {
      m_pEventTemplates[i].dwId = DBGetFieldULong(hResult, i, 0);
      m_pEventTemplates[i].dwSeverity = DBGetFieldLong(hResult, i, 1);
      m_pEventTemplates[i].dwFlags = DBGetFieldLong(hResult, i, 2);
      m_pEventTemplates[i].szMessageTemplate = strdup(DBGetField(hResult, i, 3));
      m_pEventTemplates[i].szDescription = strdup(DBGetField(hResult, i, 4));
   }

   DBFreeResult(hResult);
   return TRUE;
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
//

BOOL PostEvent(DWORD dwEventId, DWORD dwSourceId, char *szFormat, ...)
{
   EVENT_TEMPLATE *pEventTemplate;
   Event *pEvent;
   va_list args;

   // Find event template
   if (m_dwNumTemplates == 0)    // No event templates at all?
      return FALSE;
   pEventTemplate = FindEventTemplate(dwEventId);
   if (pEventTemplate == NULL)   // No event with such ID
      return FALSE;

   // Create new event
   va_start(args, szFormat);
   pEvent = new Event(pEventTemplate, dwSourceId, szFormat, args);
   va_end(args);

   // Add new event to queue
   g_pEventQueue->Put(pEvent);

   return TRUE;
}
