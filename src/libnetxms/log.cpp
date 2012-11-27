/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2012 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: log.cpp
**
**/

#include "libnetxms.h"

#if HAVE_SYSLOG_H
#include <syslog.h>
#endif

#define MAX_LOG_HISTORY_SIZE	16

/**
 * Static data
 */
#ifdef _WIN32
static HANDLE m_eventLogHandle = NULL;
static HMODULE m_msgModuleHandle = NULL;
#else
static unsigned int m_numMessages;
static const TCHAR **m_messages;
#endif
static TCHAR m_logFileName[MAX_PATH] = _T("");
static FILE *m_logFileHandle = NULL;
static MUTEX m_mutexLogAccess = INVALID_MUTEX_HANDLE;
static DWORD m_flags = 0;
static int m_rotationMode = NXLOG_ROTATION_BY_SIZE;
static int m_maxLogSize = 4096 * 1024;	// 4 MB
static int m_logHistorySize = 4;		// Keep up 4 previous log files
static TCHAR m_dailyLogSuffixTemplate[64] = _T("%Y%m%d");
static time_t m_currentDayStart = 0;
static void (*m_consoleWriter)(const TCHAR *, ...) = (void (*)(const TCHAR *, ...))_tprintf;

/**
 * Format current time for output
 */
static TCHAR *FormatLogTimestamp(TCHAR *buffer)
{
	INT64 now = GetCurrentTimeMs();
	time_t t = now / 1000;
#if HAVE_LOCALTIME_R
	struct tm ltmBuffer;
	struct tm *loc = localtime_r(&t, &ltmBuffer);
#else
	struct tm *loc = localtime(&t);
#endif
	_tcsftime(buffer, 32, _T("[%d-%b-%Y %H:%M:%S"), loc);
	_sntprintf(&buffer[21], 8, _T(".%03d]"), now % 1000);
	return buffer;
}

/**
 * Set timestamp of start of the current day
 */
static void SetDayStart()
{
	time_t now = time(NULL);
	struct tm dayStart;
#if HAVE_LOCALTIME_R
	localtime_r(&now, &dayStart);
#else
	struct tm *ltm = localtime(&now);
	memcpy(&dayStart, ltm, sizeof(struct tm));
#endif
	dayStart.tm_hour = 0;
	dayStart.tm_min = 0;
	dayStart.tm_sec = 0;
	m_currentDayStart = mktime(&dayStart);
}

/**
 * Set log rotation policy
 * Setting log size to 0 or mode to NXLOG_ROTATION_DISABLED disables log rotation
 */
BOOL LIBNETXMS_EXPORTABLE nxlog_set_rotation_policy(int rotationMode, int maxLogSize, int historySize, const TCHAR *dailySuffix)
{
	BOOL isValid = TRUE;

	if ((rotationMode >= 0) || (rotationMode <= 2))
	{
		m_rotationMode = rotationMode;
		if (rotationMode == NXLOG_ROTATION_BY_SIZE)
		{
			if ((maxLogSize == 0) || (maxLogSize >= 1024))
			{
				m_maxLogSize = maxLogSize;
			}
			else
			{
				m_maxLogSize = 1024;
				isValid = FALSE;
			}

			if ((historySize >= 0) && (historySize <= MAX_LOG_HISTORY_SIZE))
			{
				m_logHistorySize = historySize;
			}
			else
			{
				if (historySize > MAX_LOG_HISTORY_SIZE)
					m_logHistorySize = MAX_LOG_HISTORY_SIZE;
				isValid = FALSE;
			}
		}
		else if (rotationMode == NXLOG_ROTATION_DAILY)
		{
			if ((dailySuffix != NULL) && (dailySuffix[0] != 0))
				nx_strncpy(m_dailyLogSuffixTemplate, dailySuffix, sizeof(m_dailyLogSuffixTemplate) / sizeof(TCHAR));
			SetDayStart();
		}
	}
	else
	{
		isValid = FALSE;
	}

	return isValid;
}

/**
 * Set console writer
 */
void LIBNETXMS_EXPORTABLE nxlog_set_console_writer(void (*writer)(const TCHAR *, ...))
{
	m_consoleWriter = writer;
}

/**
 * Rotate log
 */
static BOOL RotateLog(BOOL needLock)
{
	int i;
	TCHAR oldName[MAX_PATH], newName[MAX_PATH];

	if (m_flags & NXLOG_USE_SYSLOG)
		return FALSE;	// Cannot rotate system logs

	if (needLock)
		MutexLock(m_mutexLogAccess);

	if ((m_logFileHandle != NULL) && (m_flags & NXLOG_IS_OPEN))
	{
		fclose(m_logFileHandle);
		m_flags &= ~NXLOG_IS_OPEN;
	}

	if (m_rotationMode == NXLOG_ROTATION_BY_SIZE)
	{
		// Delete old files
		for(i = MAX_LOG_HISTORY_SIZE; i >= m_logHistorySize; i--)
		{
			_sntprintf(oldName, MAX_PATH, _T("%s.%d"), m_logFileName, i);
			_tunlink(oldName);
		}

		// Shift file names
		for(; i >= 0; i--)
		{
			_sntprintf(oldName, MAX_PATH, _T("%s.%d"), m_logFileName, i);
			_sntprintf(newName, MAX_PATH, _T("%s.%d"), m_logFileName, i + 1);
			_trename(oldName, newName);
		}

		// Rename current log to name.0
		_sntprintf(newName, MAX_PATH, _T("%s.0"), m_logFileName);
		_trename(m_logFileName, newName);
	}
	else if (m_rotationMode == NXLOG_ROTATION_DAILY)
	{
#if HAVE_LOCALTIME_R
      struct tm ltmBuffer;
      struct tm *loc = localtime_r(&m_currentDayStart, &ltmBuffer);
#else
      struct tm *loc = localtime(&m_currentDayStart);
#endif
		TCHAR buffer[64];
      _tcsftime(buffer, 64, m_dailyLogSuffixTemplate, loc);

		// Rename current log to name.suffix
		_sntprintf(newName, MAX_PATH, _T("%s.%s"), m_logFileName, buffer);
		_trename(m_logFileName, newName);

		SetDayStart();
	}

	// Reopen log
   m_logFileHandle = _tfopen(m_logFileName, _T("w"));
   if (m_logFileHandle != NULL)
   {
		m_flags |= NXLOG_IS_OPEN;
		TCHAR buffer[32];
      _ftprintf(m_logFileHandle, _T("%s Log file truncated.\n"), FormatLogTimestamp(buffer));
	}

	if (needLock)
		MutexUnlock(m_mutexLogAccess);

	return (m_flags & NXLOG_IS_OPEN) ? TRUE : FALSE;
}

/**
 * API for application to force log rotation
 */
BOOL LIBNETXMS_EXPORTABLE nxlog_rotate()
{
	return RotateLog(TRUE);
}

/**
 * Initialize log
 */
BOOL LIBNETXMS_EXPORTABLE nxlog_open(const TCHAR *logName, DWORD flags,
                                     const TCHAR *msgModule, unsigned int msgCount, const TCHAR **messages)
{
	m_flags = flags & 0x7FFFFFFF;
#ifdef _WIN32
	m_msgModuleHandle = GetModuleHandle(msgModule);
#else
	m_numMessages = msgCount;
	m_messages = messages;
#endif

   if (m_flags & NXLOG_USE_SYSLOG)
   {
#ifdef _WIN32
      m_eventLogHandle = RegisterEventSource(NULL, logName);
		if (m_eventLogHandle != NULL)
			m_flags |= NXLOG_IS_OPEN;
#else
#ifdef UNICODE
		char *mbBuffer;

		mbBuffer = MBStringFromWideString(logName);
      openlog(mbBuffer, LOG_PID, LOG_DAEMON);
		free(mbBuffer);
#else
      openlog(logName, LOG_PID, LOG_DAEMON);
#endif
		m_flags |= NXLOG_IS_OPEN;
#endif
   }
   else
   {
      TCHAR buffer[32];

		nx_strncpy(m_logFileName, logName, MAX_PATH);
      m_logFileHandle = _tfopen(logName, _T("a"));
      if (m_logFileHandle != NULL)
      {
			m_flags |= NXLOG_IS_OPEN;
         _ftprintf(m_logFileHandle, _T("\n%s Log file opened\n"), FormatLogTimestamp(buffer));
      }

      m_mutexLogAccess = MutexCreate();
		SetDayStart();
   }
	return (m_flags & NXLOG_IS_OPEN) ? TRUE : FALSE;
}

/**
 * Close log
 */
void LIBNETXMS_EXPORTABLE nxlog_close()
{
   if (m_flags & NXLOG_IS_OPEN)
   {
      if (m_flags & NXLOG_USE_SYSLOG)
      {
#ifdef _WIN32
         DeregisterEventSource(m_eventLogHandle);
#else
         closelog();
#endif
      }
      else
      {
         if (m_logFileHandle != NULL)
            fclose(m_logFileHandle);
         if (m_mutexLogAccess != INVALID_MUTEX_HANDLE)
            MutexDestroy(m_mutexLogAccess);
      }
	   m_flags &= ~NXLOG_IS_OPEN;
   }
}

/**
 * Write record to log file
 */
static void WriteLogToFile(TCHAR *message)
{
   TCHAR buffer[64];

   // Prevent simultaneous write to log file
   MutexLock(m_mutexLogAccess);

	// Check for new day start
   time_t t = time(NULL);
	if ((m_rotationMode == NXLOG_ROTATION_DAILY) && (t >= m_currentDayStart + 86400))
	{
		RotateLog(FALSE);
	}

	FormatLogTimestamp(buffer);
   if (m_logFileHandle != NULL)
	{
      _ftprintf(m_logFileHandle, _T("%s %s"), buffer, message);
		fflush(m_logFileHandle);
	}
   if (m_flags & NXLOG_PRINT_TO_STDOUT)
      m_consoleWriter(_T("%s %s"), buffer, message);

	// Check log size
	if ((m_rotationMode == NXLOG_ROTATION_BY_SIZE) && (m_maxLogSize != 0))
	{
		struct stat st;

		fstat(fileno(m_logFileHandle), &st);
		if (st.st_size >= m_maxLogSize)
			RotateLog(FALSE);
	}

   MutexUnlock(m_mutexLogAccess);
}

/**
 * Format message (UNIX version)
 */
#ifndef _WIN32

static TCHAR *FormatMessageUX(DWORD dwMsgId, TCHAR **ppStrings)
{
   const TCHAR *p;
   TCHAR *pMsg;
   int i, iSize, iLen;

   if (dwMsgId >= m_numMessages)
   {
      // No message with this ID
      pMsg = (TCHAR *)malloc(64 * sizeof(TCHAR));
      _sntprintf(pMsg, 64, _T("MSG 0x%08X - Unable to find message text\n"), (unsigned int)dwMsgId);
   }
   else
   {
      iSize = (_tcslen(m_messages[dwMsgId]) + 2) * sizeof(TCHAR);
      pMsg = (TCHAR *)malloc(iSize);

      for(i = 0, p = m_messages[dwMsgId]; *p != 0; p++)
         if (*p == _T('%'))
         {
            p++;
            if ((*p >= _T('1')) && (*p <= _T('9')))
            {
               iLen = _tcslen(ppStrings[*p - _T('1')]);
               iSize += iLen * sizeof(TCHAR);
               pMsg = (TCHAR *)realloc(pMsg, iSize);
               _tcscpy(&pMsg[i], ppStrings[*p - _T('1')]);
               i += iLen;
            }
            else
            {
               if (*p == 0)   // Handle single % character at the string end
                  break;
               pMsg[i++] = *p;
            }
         }
         else
         {
            pMsg[i++] = *p;
         }
      pMsg[i++] = _T('\n');
      pMsg[i] = 0;
   }

   return pMsg;
}

#endif   /* ! _WIN32 */

/**
 * Write log record
 * Parameters:
 * msg    - Message ID
 * wType  - Message type (see ReportEvent() for details)
 * format - Parameter format string, each parameter represented by one character.
 *          The following format characters can be used:
 *             s - String (multibyte or UNICODE, depending on build)
 *             m - multibyte string
 *             u - UNICODE string
 *             d - Decimal integer
 *             x - Hex integer
 *             e - System error code (will appear in log as textual description)
 *             a - IP address in network byte order
 *             A - IPv6 address in network byte order
 */
void LIBNETXMS_EXPORTABLE nxlog_write(DWORD msg, WORD wType, const char *format, ...)
{
   va_list args;
   TCHAR *strings[16], *pMsg, szBuffer[256];
   int numStrings = 0;
   DWORD error;
#if defined(UNICODE) && !defined(_WIN32)
	char *mbMsg;
#endif

	if (!(m_flags & NXLOG_IS_OPEN))
		return;

   memset(strings, 0, sizeof(TCHAR *) * 16);

   if (format != NULL)
   {
      va_start(args, format);

      for(; (format[numStrings] != 0) && (numStrings < 16); numStrings++)
      {
         switch(format[numStrings])
         {
            case 's':
               strings[numStrings] = _tcsdup(va_arg(args, TCHAR *));
               break;
            case 'm':
#ifdef UNICODE
					strings[numStrings] = WideStringFromMBString(va_arg(args, char *));
#else
               strings[numStrings] = strdup(va_arg(args, char *));
#endif
               break;
            case 'u':
#ifdef UNICODE
               strings[numStrings] = wcsdup(va_arg(args, WCHAR *));
#else
					strings[numStrings] = MBStringFromWideString(va_arg(args, WCHAR *));
#endif
               break;
            case 'd':
               strings[numStrings] = (TCHAR *)malloc(16 * sizeof(TCHAR));
               _sntprintf(strings[numStrings], 16, _T("%d"), (int)(va_arg(args, LONG)));
               break;
            case 'x':
               strings[numStrings] = (TCHAR *)malloc(16 * sizeof(TCHAR));
               _sntprintf(strings[numStrings], 16, _T("0x%08X"), (unsigned int)(va_arg(args, DWORD)));
               break;
            case 'a':
               strings[numStrings] = (TCHAR *)malloc(20 * sizeof(TCHAR));
               IpToStr(va_arg(args, DWORD), strings[numStrings]);
               break;
            case 'A':
               strings[numStrings] = (TCHAR *)malloc(48 * sizeof(TCHAR));
               Ip6ToStr(va_arg(args, BYTE *), strings[numStrings]);
               break;
            case 'e':
               error = va_arg(args, DWORD);
#ifdef _WIN32
               if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                 FORMAT_MESSAGE_FROM_SYSTEM | 
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, error,
                                 MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), // Default language
                                 (LPTSTR)&pMsg, 0, NULL) > 0)
               {
                  pMsg[_tcscspn(pMsg, _T("\r\n"))] = 0;
                  strings[numStrings] = (TCHAR *)malloc((_tcslen(pMsg) + 1) * sizeof(TCHAR));
                  _tcscpy(strings[numStrings], pMsg);
                  LocalFree(pMsg);
               }
               else
               {
                  strings[numStrings] = (TCHAR *)malloc(64 * sizeof(TCHAR));
                  _sntprintf(strings[numStrings], 64, _T("MSG 0x%08X - Unable to find message text"), error);
               }
#else   /* _WIN32 */
#if HAVE_STRERROR_R
#if HAVE_POSIX_STRERROR_R
					strings[numStrings] = (TCHAR *)malloc(256 * sizeof(TCHAR));
					_tcserror_r((int)error, strings[numStrings], 256);
#else
					strings[numStrings] = _tcsdup(_tcserror_r((int)error, szBuffer, 256));
#endif
#else
					strings[numStrings] = _tcsdup(_tcserror((int)error));
#endif
#endif
               break;
            default:
               strings[numStrings] = (TCHAR *)malloc(32 * sizeof(TCHAR));
               _sntprintf(strings[numStrings], 32, _T("BAD FORMAT (0x%08X)"), (unsigned int)(va_arg(args, DWORD)));
               break;
         }
      }
      va_end(args);
   }

#ifdef _WIN32
   if (!(m_flags & NXLOG_USE_SYSLOG) || (m_flags & NXLOG_PRINT_TO_STDOUT))
   {
      LPVOID lpMsgBuf;

      if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        m_msgModuleHandle, msg, 0, (LPTSTR)&lpMsgBuf, 0, (va_list *)strings) > 0)
      {
         TCHAR *pCR;

         // Replace trailing CR/LF pair with LF
         pCR = _tcschr((TCHAR *)lpMsgBuf, _T('\r'));
         if (pCR != NULL)
         {
            *pCR = _T('\n');
            pCR++;
            *pCR = 0;
         }
			if (m_flags & NXLOG_USE_SYSLOG)
			{
				m_consoleWriter(_T("%s %s"), FormatLogTimestamp(szBuffer), (TCHAR *)lpMsgBuf);
			}
			else
			{
				WriteLogToFile((TCHAR *)lpMsgBuf);
			}
         LocalFree(lpMsgBuf);
      }
      else
      {
         TCHAR message[64];

         _sntprintf(message, 64, _T("MSG 0x%08X - cannot format message"), msg);
			if (m_flags & NXLOG_USE_SYSLOG)
			{
				m_consoleWriter(_T("%s %s"), FormatLogTimestamp(szBuffer), message);
			}
			else
			{
	         WriteLogToFile(message);
			}
      }
   }

   if (m_flags & NXLOG_USE_SYSLOG)
	{
      ReportEvent(m_eventLogHandle, (wType == EVENTLOG_DEBUG_TYPE) ? EVENTLOG_INFORMATION_TYPE : wType, 0, msg, NULL, numStrings, 0, (const TCHAR **)strings, NULL);
	}
#else  /* _WIN32 */
   pMsg = FormatMessageUX(msg, strings);
   if (m_flags & NXLOG_USE_SYSLOG)
   {
      int level;

      switch(wType)
      {
         case EVENTLOG_ERROR_TYPE:
            level = LOG_ERR;
            break;
         case EVENTLOG_WARNING_TYPE:
            level = LOG_WARNING;
            break;
         case EVENTLOG_INFORMATION_TYPE:
            level = LOG_NOTICE;
            break;
         case EVENTLOG_DEBUG_TYPE:
            level = LOG_DEBUG;
            break;
         default:
            level = LOG_INFO;
            break;
      }
#ifdef UNICODE
		mbMsg = MBStringFromWideString(pMsg);
      syslog(level, "%s", mbMsg);
		free(mbMsg);
#else
      syslog(level, "%s", pMsg);
#endif

		if (m_flags & NXLOG_PRINT_TO_STDOUT)
		{
			m_consoleWriter(_T("%s %s"), FormatLogTimestamp(szBuffer), pMsg);
		}
   }
   else
   {
      WriteLogToFile(pMsg);
   }
   free(pMsg);
#endif /* _WIN32 */

   while(--numStrings >= 0)
      free(strings[numStrings]);
}
