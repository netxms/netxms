/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
#include <nxstat.h>
#include "debug_tag_tree.h"

#if HAVE_SYSLOG_H
#include <syslog.h>
#endif

#define MAX_LOG_HISTORY_SIZE	128

/**
 * Static data
 */
#ifdef _WIN32
static HANDLE s_eventLogHandle = NULL;
static HMODULE s_msgModuleHandle = NULL;
#else
static unsigned int s_numMessages;
static const TCHAR **s_messages;
static char s_syslogName[64];
#endif
static TCHAR s_logFileName[MAX_PATH] = _T("");
static FILE *s_logFileHandle = NULL;
static MUTEX s_mutexLogAccess = INVALID_MUTEX_HANDLE;
static UINT32 s_flags = 0;
static DWORD s_debugMsg = 0;
static DWORD s_debugMsgTag = 0;
static DWORD s_genericMsg = 0;
static int s_rotationMode = NXLOG_ROTATION_BY_SIZE;
static UINT64 s_maxLogSize = 4096 * 1024;	// 4 MB
static int s_logHistorySize = 4;		// Keep up 4 previous log files
static TCHAR s_dailyLogSuffixTemplate[64] = _T("%Y%m%d");
static time_t s_currentDayStart = 0;
static NxLogConsoleWriter m_consoleWriter = (NxLogConsoleWriter)_tprintf;
static String s_logBuffer;
static THREAD s_writerThread = INVALID_THREAD_HANDLE;
static CONDITION s_writerStopCondition = INVALID_CONDITION_HANDLE;
static NxLogDebugWriter s_debugWriter = NULL;
static DebugTagTree* volatile s_tagTreeActive = new DebugTagTree();
static DebugTagTree* volatile s_tagTreeSecondary = new DebugTagTree();
static Mutex s_mutexDebugTagTreeWrite;

/**
 * Swaps tag tree pointers and waits till reader count drops to 0
 */
static inline void SwapAndWait()
{
   s_tagTreeSecondary = InterlockedExchangeObjectPointer(&s_tagTreeActive, s_tagTreeSecondary);
   InterlockedIncrement(&s_tagTreeSecondary->m_writers);
   while(s_tagTreeSecondary->m_readers > 0)
      ThreadSleepMs(10);
}

/**
 * Set debug level
 */
void LIBNETXMS_EXPORTABLE nxlog_set_debug_level(int level)
{
   if ((level >= 0) && (level <= 9))
   {
      s_mutexDebugTagTreeWrite.lock();
      s_tagTreeSecondary->setRootDebugLevel(level); // Update the secondary tree
      SwapAndWait();
      s_tagTreeSecondary->setRootDebugLevel(level); // Update the previously active tree
      InterlockedDecrement(&s_tagTreeSecondary->m_writers);
      s_mutexDebugTagTreeWrite.unlock();
   }
}

/**
 * Set debug level for tag
 */
void LIBNETXMS_EXPORTABLE nxlog_set_debug_level_tag(const TCHAR *tag, int level)
{
   if ((tag == NULL) || !_tcscmp(tag, _T("*")))
   {
      nxlog_set_debug_level(level);
   }
   else
   {
      s_mutexDebugTagTreeWrite.lock();
      if ((level >= 0) && (level <= 9))
      {
         s_tagTreeSecondary->add(tag, level);
         SwapAndWait();
         s_tagTreeSecondary->add(tag, level);
      }
      else if (level < 0)
      {
         s_tagTreeSecondary->remove(tag);
         SwapAndWait();
         s_tagTreeSecondary->remove(tag);
      }
      InterlockedDecrement(&s_tagTreeSecondary->m_writers);
      s_mutexDebugTagTreeWrite.unlock();
   }
}

/**
 * Get current debug level
 */
void LIBNETXMS_EXPORTABLE nxlog_reset_debug_level_tags()
{
   s_mutexDebugTagTreeWrite.lock();
   s_tagTreeSecondary->clear();
   SwapAndWait();
   s_tagTreeSecondary->clear();
   InterlockedDecrement(&s_tagTreeSecondary->m_writers);
   s_mutexDebugTagTreeWrite.unlock();
}

/**
 * Acquire active tag tree for reading
 */
inline DebugTagTree *AcquireTagTree()
{
   DebugTagTree *tagTree;
   while(true)
   {
      tagTree = s_tagTreeActive;
      InterlockedIncrement(&tagTree->m_readers);
      if (tagTree->m_writers == 0)
         break;
      InterlockedDecrement(&tagTree->m_readers);
   }
   return tagTree;
}

/**
 * Release previously acquited tag tree
 */
inline void ReleaseTagTree(DebugTagTree *tagTree)
{
   InterlockedDecrement(&tagTree->m_readers);
}

/**
 * Get current debug level
 */
int LIBNETXMS_EXPORTABLE nxlog_get_debug_level()
{
   DebugTagTree *tagTree = AcquireTagTree();
   int level = tagTree->getRootDebugLevel();
   ReleaseTagTree(tagTree);
   return level;
}

/**
 * Get current debug level for tag
 */
int LIBNETXMS_EXPORTABLE nxlog_get_debug_level_tag(const TCHAR *tag)
{
   DebugTagTree *tagTree = AcquireTagTree();
   int level = tagTree->getDebugLevel(tag);
   ReleaseTagTree(tagTree);
   return level;
}

/**
 * Get current debug level for tag/object combination
 */
int LIBNETXMS_EXPORTABLE nxlog_get_debug_level_tag_object(const TCHAR *tag, UINT32 objectId)
{
   TCHAR fullTag[256];
   _sntprintf(fullTag, 256, _T("%s.%u"), tag, objectId);
   DebugTagTree *tagTree = AcquireTagTree();
   int level = tagTree->getDebugLevel(fullTag);
   ReleaseTagTree(tagTree);
   return level;
}

/**
 * Get all configured debug tags
 */
ObjectArray<DebugTagInfo> LIBNETXMS_EXPORTABLE *nxlog_get_all_debug_tags()
{
   DebugTagTree *tagTree = AcquireTagTree();
   ObjectArray<DebugTagInfo> *tags = tagTree->getAllTags();
   ReleaseTagTree(tagTree);
   return tags;
}

/**
 * Set additional debug writer callback. It will be called for each line written with nxlog_debug.
 */
extern "C" void LIBNETXMS_EXPORTABLE nxlog_set_debug_writer(NxLogDebugWriter writer)
{
   s_debugWriter = writer;
}

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
	_tcsftime(buffer, 32, _T("%Y.%m.%d %H:%M:%S"), loc);
	_sntprintf(&buffer[19], 8, _T(".%03d"), (int)(now % 1000));
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
	s_currentDayStart = mktime(&dayStart);
}

/**
 * Set log rotation policy
 * Setting log size to 0 or mode to NXLOG_ROTATION_DISABLED disables log rotation
 */
bool LIBNETXMS_EXPORTABLE nxlog_set_rotation_policy(int rotationMode, UINT64 maxLogSize, int historySize, const TCHAR *dailySuffix)
{
	bool isValid = true;

	if ((rotationMode >= 0) || (rotationMode <= 2))
	{
		s_rotationMode = rotationMode;
		if (rotationMode == NXLOG_ROTATION_BY_SIZE)
		{
			if ((maxLogSize == 0) || (maxLogSize >= 1024))
			{
				s_maxLogSize = maxLogSize;
			}
			else
			{
				s_maxLogSize = 1024;
				isValid = false;
			}

			if ((historySize >= 0) && (historySize <= MAX_LOG_HISTORY_SIZE))
			{
				s_logHistorySize = historySize;
			}
			else
			{
				if (historySize > MAX_LOG_HISTORY_SIZE)
					s_logHistorySize = MAX_LOG_HISTORY_SIZE;
				isValid = false;
			}
		}
		else if (rotationMode == NXLOG_ROTATION_DAILY)
		{
			if ((dailySuffix != NULL) && (dailySuffix[0] != 0))
				nx_strncpy(s_dailyLogSuffixTemplate, dailySuffix, sizeof(s_dailyLogSuffixTemplate) / sizeof(TCHAR));
			SetDayStart();
		}
	}
	else
	{
		isValid = false;
	}

	if (isValid)
	   nxlog_debug(0, _T("Log rotation policy set to %d (size=") UINT64_FMT _T(", count=%d)"), rotationMode, maxLogSize, historySize);

	return isValid;
}

/**
 * Set console writer
 */
void LIBNETXMS_EXPORTABLE nxlog_set_console_writer(NxLogConsoleWriter writer)
{
	m_consoleWriter = writer;
}

/**
 * Rotate log
 */
static bool RotateLog(bool needLock)
{
	int i;
	TCHAR oldName[MAX_PATH], newName[MAX_PATH];

	if (s_flags & NXLOG_USE_SYSLOG)
		return FALSE;	// Cannot rotate system logs

	if (needLock)
		MutexLock(s_mutexLogAccess);

	if ((s_logFileHandle != NULL) && (s_flags & NXLOG_IS_OPEN))
	{
		fclose(s_logFileHandle);
		s_flags &= ~NXLOG_IS_OPEN;
	}

	if (s_rotationMode == NXLOG_ROTATION_BY_SIZE)
	{
		// Delete old files
		for(i = MAX_LOG_HISTORY_SIZE; i >= s_logHistorySize; i--)
		{
			_sntprintf(oldName, MAX_PATH, _T("%s.%d"), s_logFileName, i);
			_tunlink(oldName);
		}

		// Shift file names
		for(; i >= 0; i--)
		{
			_sntprintf(oldName, MAX_PATH, _T("%s.%d"), s_logFileName, i);
			_sntprintf(newName, MAX_PATH, _T("%s.%d"), s_logFileName, i + 1);
			_trename(oldName, newName);
		}

		// Rename current log to name.0
		_sntprintf(newName, MAX_PATH, _T("%s.0"), s_logFileName);
		_trename(s_logFileName, newName);
	}
	else if (s_rotationMode == NXLOG_ROTATION_DAILY)
	{
#if HAVE_LOCALTIME_R
      struct tm ltmBuffer;
      struct tm *loc = localtime_r(&s_currentDayStart, &ltmBuffer);
#else
      struct tm *loc = localtime(&s_currentDayStart);
#endif
		TCHAR buffer[64];
      _tcsftime(buffer, 64, s_dailyLogSuffixTemplate, loc);

		// Rename current log to name.suffix
		_sntprintf(newName, MAX_PATH, _T("%s.%s"), s_logFileName, buffer);
		_trename(s_logFileName, newName);

		SetDayStart();
	}

   // Reopen log
#if HAVE_FOPEN64
   s_logFileHandle = _tfopen64(s_logFileName, _T("w"));
#else
   s_logFileHandle = _tfopen(s_logFileName, _T("w"));
#endif
   if (s_logFileHandle != NULL)
   {
      s_flags |= NXLOG_IS_OPEN;
      TCHAR buffer[32];
      _ftprintf(s_logFileHandle, _T("%s Log file truncated.\n"), FormatLogTimestamp(buffer));
      fflush(s_logFileHandle);
#ifndef _WIN32
      int fd = fileno(s_logFileHandle);
      fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
#endif
   }

	if (needLock)
		MutexUnlock(s_mutexLogAccess);

	return (s_flags & NXLOG_IS_OPEN) ? true : false;
}

/**
 * API for application to force log rotation
 */
bool LIBNETXMS_EXPORTABLE nxlog_rotate()
{
	return RotateLog(true);
}

/**
 * Background writer thread
 */
static THREAD_RESULT THREAD_CALL BackgroundWriterThread(void *arg)
{
   bool stop = false;
   while(!stop)
   {
      stop = ConditionWait(s_writerStopCondition, 1000);

	   // Check for new day start
      time_t t = time(NULL);
	   if ((s_rotationMode == NXLOG_ROTATION_DAILY) && (t >= s_currentDayStart + 86400))
	   {
		   RotateLog(FALSE);
	   }

      MutexLock(s_mutexLogAccess);
      if (!s_logBuffer.isEmpty())
      {
         if (s_flags & NXLOG_PRINT_TO_STDOUT)
            m_consoleWriter(_T("%s"), s_logBuffer.getBuffer());

         size_t buflen = s_logBuffer.length();
         char *data = s_logBuffer.getUTF8String();
         s_logBuffer.clear();
         MutexUnlock(s_mutexLogAccess);

         if (s_flags & NXLOG_DEBUG_MODE)
         {
            char marker[64];
            sprintf(marker, "##(" INT64_FMTA ")" INT64_FMTA " @" INT64_FMTA "\n",
                    (INT64)buflen, (INT64)strlen(data), GetCurrentTimeMs());
#ifdef _WIN32
            fwrite(marker, 1, strlen(marker), s_logFileHandle);
#else
            write(fileno(s_logFileHandle), marker, strlen(marker));
#endif
         }

#ifdef _WIN32
			fwrite(data, 1, strlen(data), s_logFileHandle);
#else
         // write is used here because on linux fwrite is not working
         // after calling fwprintf on a stream
			size_t size = strlen(data);
			size_t offset = 0;
			do
			{
			   int bw = write(fileno(s_logFileHandle), &data[offset], size);
			   if (bw < 0)
			      break;
			   size -= bw;
			   offset += bw;
			} while(size > 0);
#endif
         MemFree(data);

	      // Check log size
	      if ((s_logFileHandle != NULL) && (s_rotationMode == NXLOG_ROTATION_BY_SIZE) && (s_maxLogSize != 0))
	      {
	         NX_STAT_STRUCT st;
		      NX_FSTAT(fileno(s_logFileHandle), &st);
		      if ((UINT64)st.st_size >= s_maxLogSize)
			      RotateLog(FALSE);
	      }
      }
      else
      {
         MutexUnlock(s_mutexLogAccess);
      }
   }
   return THREAD_OK;
}

/**
 * Initialize log
 */
bool LIBNETXMS_EXPORTABLE nxlog_open(const TCHAR *logName, UINT32 flags,
                                     const TCHAR *msgModule, unsigned int msgCount, const TCHAR **messages,
                                     DWORD debugMsg, DWORD debugMsgTag, DWORD genericMsg)
{
	s_flags = flags & 0x7FFFFFFF;
#ifdef _WIN32
	s_msgModuleHandle = GetModuleHandle(msgModule);
#else
	s_numMessages = msgCount;
	s_messages = messages;
#endif
	s_debugMsg = debugMsg;
   s_debugMsgTag = debugMsgTag;
	s_genericMsg = genericMsg;

   if (s_flags & NXLOG_USE_SYSLOG)
   {
#ifdef _WIN32
      s_eventLogHandle = RegisterEventSource(NULL, logName);
		if (s_eventLogHandle != NULL)
			s_flags |= NXLOG_IS_OPEN;
#else
#ifdef UNICODE
		WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, logName, -1, s_syslogName, 64, NULL, NULL);
		s_syslogName[63] = 0;
#else
		nx_strncpy(s_syslogName, logName, 64);
#endif
      openlog(s_syslogName, LOG_PID, LOG_DAEMON);
		s_flags |= NXLOG_IS_OPEN;
#endif
   }
   else
   {
      TCHAR buffer[32];

		nx_strncpy(s_logFileName, logName, MAX_PATH);
#if HAVE_FOPEN64
      s_logFileHandle = _tfopen64(logName, _T("a"));
#else
      s_logFileHandle = _tfopen(logName, _T("a"));
#endif
      if (s_logFileHandle != NULL)
      {
			s_flags |= NXLOG_IS_OPEN;
         _ftprintf(s_logFileHandle, _T("\n%s Log file opened (rotation policy %d, max size ") UINT64_FMT _T(")\n"),
                   FormatLogTimestamp(buffer), s_rotationMode, s_maxLogSize);
         fflush(s_logFileHandle);

#ifndef _WIN32
         int fd = fileno(s_logFileHandle);
         fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
#endif

         if (s_flags & NXLOG_BACKGROUND_WRITER)
         {
            s_logBuffer.setAllocationStep(8192);
            s_writerStopCondition = ConditionCreate(TRUE);
            s_writerThread = ThreadCreateEx(BackgroundWriterThread, 0, NULL);
         }
      }

      s_mutexLogAccess = MutexCreate();
		SetDayStart();
   }
	return (s_flags & NXLOG_IS_OPEN) ? true : false;
}

/**
 * Close log
 */
void LIBNETXMS_EXPORTABLE nxlog_close()
{
   if (s_flags & NXLOG_IS_OPEN)
   {
      if (s_flags & NXLOG_USE_SYSLOG)
      {
#ifdef _WIN32
         DeregisterEventSource(s_eventLogHandle);
#else
         closelog();
#endif
      }
      else
      {
         if (s_flags & NXLOG_BACKGROUND_WRITER)
         {
            ConditionSet(s_writerStopCondition);
            ThreadJoin(s_writerThread);
            ConditionDestroy(s_writerStopCondition);
         }

         if (s_logFileHandle != NULL)
            fclose(s_logFileHandle);
         if (s_mutexLogAccess != INVALID_MUTEX_HANDLE)
            MutexDestroy(s_mutexLogAccess);
      }
	   s_flags &= ~NXLOG_IS_OPEN;
   }
}

/**
 * Write record to log file
 */
static void WriteLogToFile(TCHAR *message, const WORD type)
{
   const TCHAR *loglevel;
   switch(type)
   {
      case NXLOG_ERROR:
         loglevel = _T("*E* ");
	      break;
      case NXLOG_WARNING:
         loglevel = _T("*W* ");
	      break;
      case NXLOG_INFO:
         loglevel = _T("*I* ");
	      break;
      case NXLOG_DEBUG:
         loglevel = _T("*D* ");
	      break;
      default:
         loglevel = _T("*?* ");
	      break;
   }

   TCHAR buffer[64];
   if (s_flags & NXLOG_BACKGROUND_WRITER)
   {
      MutexLock(s_mutexLogAccess);

	   FormatLogTimestamp(buffer);
      s_logBuffer.append(buffer);
      s_logBuffer.append(_T(" "));
      s_logBuffer.append(loglevel);
      s_logBuffer.append(message);

      MutexUnlock(s_mutexLogAccess);
   }
   else
   {
      // Prevent simultaneous write to log file
      MutexLock(s_mutexLogAccess);

	   // Check for new day start
      time_t t = time(NULL);
	   if ((s_rotationMode == NXLOG_ROTATION_DAILY) && (t >= s_currentDayStart + 86400))
	   {
		   RotateLog(FALSE);
	   }

	   FormatLogTimestamp(buffer);
      if (s_logFileHandle != NULL)
	   {
         _ftprintf(s_logFileHandle, _T("%s %s%s"), buffer, loglevel, message);
		   fflush(s_logFileHandle);
	   }
      if (s_flags & NXLOG_PRINT_TO_STDOUT)
         m_consoleWriter(_T("%s %s%s"), buffer, loglevel, message);

	   // Check log size
	   if ((s_logFileHandle != NULL) && (s_rotationMode == NXLOG_ROTATION_BY_SIZE) && (s_maxLogSize != 0))
	   {
	      NX_STAT_STRUCT st;
		   NX_FSTAT(fileno(s_logFileHandle), &st);
		   if ((UINT64)st.st_size >= s_maxLogSize)
			   RotateLog(FALSE);
	   }

      MutexUnlock(s_mutexLogAccess);
   }
}

/**
 * Format message (UNIX version)
 */
#ifndef _WIN32

static TCHAR *FormatMessageUX(UINT32 dwMsgId, TCHAR **ppStrings)
{
   const TCHAR *p;
   TCHAR *pMsg;
   int i, iSize, iLen;

   if (dwMsgId >= s_numMessages)
   {
      // No message with this ID
      pMsg = (TCHAR *)malloc(64 * sizeof(TCHAR));
      _sntprintf(pMsg, 64, _T("MSG 0x%08X - Unable to find message text\n"), (unsigned int)dwMsgId);
   }
   else
   {
      iSize = (_tcslen(s_messages[dwMsgId]) + 2) * sizeof(TCHAR);
      pMsg = (TCHAR *)malloc(iSize);

      for(i = 0, p = s_messages[dwMsgId]; *p != 0; p++)
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
 *             H - IPv6 address in network byte order (will appear in [])
 *             I - pointer to InetAddress object
 */
void LIBNETXMS_EXPORTABLE nxlog_write(DWORD msg, WORD wType, const char *format, ...)
{
   va_list args;
   TCHAR *strings[16], *pMsg, szBuffer[256];
   int numStrings = 0;
   UINT32 error;
#if defined(UNICODE) && !defined(_WIN32)
	char *mbMsg;
#endif

	if (!(s_flags & NXLOG_IS_OPEN) || (msg == 0))
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
               _sntprintf(strings[numStrings], 16, _T("0x%08X"), (unsigned int)(va_arg(args, UINT32)));
               break;
            case 'a':
               strings[numStrings] = (TCHAR *)malloc(20 * sizeof(TCHAR));
               IpToStr(va_arg(args, UINT32), strings[numStrings]);
               break;
            case 'A':
               strings[numStrings] = (TCHAR *)malloc(48 * sizeof(TCHAR));
               Ip6ToStr(va_arg(args, BYTE *), strings[numStrings]);
               break;
            case 'H':
               strings[numStrings] = (TCHAR *)malloc(48 * sizeof(TCHAR));
               strings[numStrings][0] = _T('[');
               Ip6ToStr(va_arg(args, BYTE *), strings[numStrings] + 1);
               _tcscat(strings[numStrings], _T("]"));
               break;
            case 'I':
               strings[numStrings] = (TCHAR *)malloc(48 * sizeof(TCHAR));
               va_arg(args, InetAddress *)->toString(strings[numStrings]);
               break;
            case 'e':
               error = va_arg(args, UINT32);
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
               _sntprintf(strings[numStrings], 32, _T("BAD FORMAT (0x%08X)"), (unsigned int)(va_arg(args, UINT32)));
               break;
         }
      }
      va_end(args);
   }

#ifdef _WIN32
   if (!(s_flags & NXLOG_USE_SYSLOG) || (s_flags & NXLOG_PRINT_TO_STDOUT))
   {
      LPVOID lpMsgBuf;

      if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        s_msgModuleHandle, msg, 0, (LPTSTR)&lpMsgBuf, 0, (va_list *)strings) > 0)
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
			if (s_flags & NXLOG_USE_SYSLOG)
			{
				m_consoleWriter(_T("%s %s"), FormatLogTimestamp(szBuffer), (TCHAR *)lpMsgBuf);
			}
			else
			{
				WriteLogToFile((TCHAR *)lpMsgBuf, wType);
			}
         LocalFree(lpMsgBuf);
      }
      else
      {
         TCHAR message[64];

         _sntprintf(message, 64, _T("MSG 0x%08X - cannot format message"), msg);
			if (s_flags & NXLOG_USE_SYSLOG)
			{
				m_consoleWriter(_T("%s %s"), FormatLogTimestamp(szBuffer), message);
			}
			else
			{
	         WriteLogToFile(message, wType);
			}
      }
   }

   if (s_flags & NXLOG_USE_SYSLOG)
	{
      ReportEvent(s_eventLogHandle, (wType == EVENTLOG_DEBUG_TYPE) ? EVENTLOG_INFORMATION_TYPE : wType, 0, msg, NULL, numStrings, 0, (const TCHAR **)strings, NULL);
	}
#else  /* _WIN32 */
   pMsg = FormatMessageUX(msg, strings);
   if (s_flags & NXLOG_USE_SYSLOG)
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
		MemFree(mbMsg);
#else
      syslog(level, "%s", pMsg);
#endif

		if (s_flags & NXLOG_PRINT_TO_STDOUT)
		{
			m_consoleWriter(_T("%s %s"), FormatLogTimestamp(szBuffer), pMsg);
		}
   }
   else
   {
      WriteLogToFile(pMsg, wType);
   }
   MemFree(pMsg);
#endif /* _WIN32 */

   while(--numStrings >= 0)
      MemFree(strings[numStrings]);
}

/**
 * Write generic message to log (useful for warning and error messages generated within libraries)
 */
void LIBNETXMS_EXPORTABLE nxlog_write_generic(WORD type, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   TCHAR msg[8192];
   _vsntprintf(msg, 8192, format, args);
   va_end(args);
   nxlog_write(s_genericMsg, type, "s", msg);
}

/**
 * Write debug message
 */
void LIBNETXMS_EXPORTABLE nxlog_debug(int level, const TCHAR *format, ...)
{
   if (level > nxlog_get_debug_level_tag(_T("*")))
      return;

   va_list args;
   va_start(args, format);
   TCHAR buffer[8192];
   _vsntprintf(buffer, 8192, format, args);
   va_end(args);
   nxlog_write(s_debugMsg, NXLOG_DEBUG, "s", buffer);

   if (s_debugWriter != NULL)
      s_debugWriter(NULL, buffer);
}

/**
 * Write debug message
 */
void LIBNETXMS_EXPORTABLE nxlog_debug2(int level, const TCHAR *format, va_list args)
{
   if (level > nxlog_get_debug_level_tag(_T("*")))
      return;

   TCHAR buffer[8192];
   _vsntprintf(buffer, 8192, format, args);
   nxlog_write(s_debugMsg, NXLOG_DEBUG, "s", buffer);

   if (s_debugWriter != NULL)
      s_debugWriter(NULL, buffer);
}

/**
 * Write debug message with tag
 */
static void nxlog_debug_tag_internal(const TCHAR *tag, int level, const TCHAR *format, va_list args)
{
   TCHAR tagf[20];
   int i;
   for(i = 0; (i < 19) && tag[i] != 0; i++)
      tagf[i] = tag[i];
   for(; i < 19; i++)
      tagf[i] = ' ';
   tagf[i] = 0;

   TCHAR buffer[8192];
   _vsntprintf(buffer, 8192, format, args);
   nxlog_write(s_debugMsgTag, NXLOG_DEBUG, "ss", tagf, buffer);

   if (s_debugWriter != NULL)
      s_debugWriter(tag, buffer);
}

/**
 * Write debug message with tag
 */
void LIBNETXMS_EXPORTABLE nxlog_debug_tag(const TCHAR *tag, int level, const TCHAR *format, ...)
{
   if (level > nxlog_get_debug_level_tag(tag))
      return;

   va_list args;
   va_start(args, format);
   nxlog_debug_tag_internal(tag, level, format, args);
   va_end(args);
}

/**
 * Write debug message with tag
 */
void LIBNETXMS_EXPORTABLE nxlog_debug_tag2(const TCHAR *tag, int level, const TCHAR *format, va_list args)
{
   if (level > nxlog_get_debug_level_tag(tag))
      return;

   nxlog_debug_tag_internal(tag, level, format, args);
}

/**
 * Write debug message with tag and object ID (added as last part of a tag)
 */
void LIBNETXMS_EXPORTABLE nxlog_debug_tag_object(const TCHAR *tag, UINT32 objectId, int level, const TCHAR *format, ...)
{
   TCHAR fullTag[256];
   _sntprintf(fullTag, 256, _T("%s.%u"), tag, objectId);
   if (level > nxlog_get_debug_level_tag(fullTag))
      return;

   va_list args;
   va_start(args, format);
   nxlog_debug_tag_internal(fullTag, level, format, args);
   va_end(args);
}

/**
 * Write debug message with tag and object ID (added as last part of a tag)
 */
void LIBNETXMS_EXPORTABLE nxlog_debug_tag_object2(const TCHAR *tag, UINT32 objectId, int level, const TCHAR *format, va_list args)
{
   TCHAR fullTag[256];
   _sntprintf(fullTag, 256, _T("%s.%u"), tag, objectId);
   if (level > nxlog_get_debug_level_tag(fullTag))
      return;

   nxlog_debug_tag_internal(fullTag, level, format, args);
}
