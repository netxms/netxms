/* 
** Network Management System Core
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
** $module: log.cpp
**
**/

#include "nms_core.h"
#include <stdarg.h>


//
// Static data
//

#ifdef _WIN32
static HANDLE m_hEventLog = INVALID_HANDLE_VALUE;
#endif
static FILE *m_hLogFile = NULL;
static MUTEX m_mutexLogAccess = INVALID_MUTEX_HANDLE;


//
// Initialize log
//

void InitLog(void)
{
#ifdef _WIN32
   if (g_dwFlags & AF_USE_EVENT_LOG)
   {
      m_hEventLog = RegisterEventSource(NULL, CORE_EVENT_SOURCE);
   }
   else
   {
#endif
      char szTimeBuf[32];
      struct tm *loc;
      time_t t;

      m_hLogFile = fopen(g_szLogFile, "a");
      t = time(NULL);
      loc = localtime(&t);
      strftime(szTimeBuf, 32, "%d-%b-%Y %H:%M:%S", loc);
      fprintf(m_hLogFile, "**************************************************************\r\n[%s] Log file opened\r\n", szTimeBuf);

      m_mutexLogAccess = MutexCreate();
#ifdef _WIN32
   }
#endif
}


//
// Close log
//

void CloseLog(void)
{
#ifdef _WIN32
   if (g_dwFlags & AF_USE_EVENT_LOG)
   {
      DeregisterEventSource(m_hEventLog);
   }
   else
   {
#endif
      if (m_hLogFile != NULL)
         fclose(m_hLogFile);
      if (m_mutexLogAccess != INVALID_MUTEX_HANDLE)
         MutexDestroy(m_mutexLogAccess);
#ifdef _WIN32
   }
#endif
}


//
// Write record to log file
//

static void WriteLogToFile(char *szMessage)
{
   char szBuffer[64];
   time_t t;
   struct tm *loc;

   // Prevent simultaneous write to log file
   MutexLock(m_mutexLogAccess, INFINITE);

   t = time(NULL);
   loc = localtime(&t);
   strftime(szBuffer, 32, "[%d-%b-%Y %H:%M:%S]", loc);
   fprintf(m_hLogFile, "%s %s", szBuffer, szMessage);
   if (IsStandalone())
      printf("%s %s", szBuffer, szMessage);

   MutexUnlock(m_mutexLogAccess);
}


//
// Write log record
// Parameters:
// msg    - Message ID
// wType  - Message type (see ReportEvent() for details)
// format - Parameter format string, each parameter represented by one character.
//          The following format characters can be used:
//             s - String
//             d - Decimal integer
//             x - Hex integer
//             e - System error code (will appear in log as textual description)
//

void WriteLog(DWORD msg, WORD wType, char *format, ...)
{
   va_list args;
   char *strings[16], *msgBuf;
   int numStrings = 0;
   DWORD error;

   memset(strings, 0, sizeof(char *) * 16);

   if (format != NULL)
   {
      va_start(args, format);

      for(; (format[numStrings] != 0) && (numStrings < 16); numStrings++)
      {
         switch(format[numStrings])
         {
            case 's':
               strings[numStrings] = strdup(va_arg(args, char *));
               break;
            case 'd':
               strings[numStrings] = (char *)malloc(16);
               sprintf(strings[numStrings], "%d", va_arg(args, LONG));
               break;
            case 'x':
               strings[numStrings] = (char *)malloc(16);
               sprintf(strings[numStrings], "0x%08X", va_arg(args, DWORD));
               break;
            case 'e':
               error = va_arg(args, DWORD);
#ifdef _WIN32
               if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                 FORMAT_MESSAGE_FROM_SYSTEM | 
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, error,
                                 MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), // Default language
                                 (LPSTR)&msgBuf,0,NULL)>0)
               {
                  msgBuf[strcspn(msgBuf,"\r\n")] = 0;
                  strings[numStrings]=(char *)malloc(strlen(msgBuf) + 1);
                  strcpy(strings[numStrings], msgBuf);
                  LocalFree(msgBuf);
               }
               else
               {
                  strings[numStrings] = (char *)malloc(64);
                  sprintf(strings[numStrings], "MSG 0x%08X - Unable to find message text", error);
               }
#else   /* _WIN32 */
               strings[numStrings] = strdup(strerror(error));
#endif
               break;
            default:
               strings[numStrings] = (char *)malloc(32);
               sprintf(strings[numStrings], "BAD FORMAT (0x%08X)", va_arg(args, DWORD));
               break;
         }
      }
      va_end(args);
   }

#ifdef _WIN32
   if (g_dwFlags & AF_USE_EVENT_LOG)
   {
      ReportEvent(m_hEventLog, wType, 0, msg, NULL, numStrings, 0, (const char **)strings, NULL);
   }
   else
   {
      LPVOID lpMsgBuf;

      if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        NULL, msg, 0, (LPTSTR)&lpMsgBuf, 0, strings)>0)
      {
         WriteLogToFile((char *)lpMsgBuf);
         LocalFree(lpMsgBuf);
      }
      else
      {
         char message[64];

         sprintf(message,"MSG 0x%08X - Unable to find message text\r\n",msg);
         WriteLogToFile(message);
      }
   }
#else  /* _WIN32 */

   /* TODO: add event logging under UNIX */

#endif /* _WIN32 */

   while(--numStrings >= 0)
      free(strings[numStrings]);
}
