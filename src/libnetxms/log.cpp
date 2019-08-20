/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2019 Victor Kirhenshtein
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

#define MAX_LOG_HISTORY_SIZE	   128

#define LOCAL_MSG_BUFFER_SIZE    1024

typedef TCHAR msg_buffer_t[LOCAL_MSG_BUFFER_SIZE];

/**
 * Debug tags
 */
struct DebugTagManager
{
   DebugTagTree* volatile active;
   DebugTagTree* volatile secondary;

   DebugTagManager()
   {
      active = new DebugTagTree();
      secondary = new DebugTagTree();
   }

   ~DebugTagManager()
   {
      delete active;
      delete secondary;
   }
};

/**
 * Static data
 */
#ifdef _WIN32
static HANDLE s_eventLogHandle = NULL;
#else
static char s_syslogName[64];
#endif
static TCHAR s_logFileName[MAX_PATH] = _T("");
static FILE *s_logFileHandle = NULL;
static MUTEX s_mutexLogAccess = INVALID_MUTEX_HANDLE;
static UINT32 s_flags = 0;
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
static volatile DebugTagManager s_tagTree;
static Mutex s_mutexDebugTagTreeWrite;

/**
 * Swaps tag tree pointers and waits till reader count drops to 0
 */
static inline void SwapAndWait()
{
   s_tagTree.secondary = InterlockedExchangeObjectPointer(&s_tagTree.active, s_tagTree.secondary);
   InterlockedIncrement(&s_tagTree.secondary->m_writers);
   while(s_tagTree.secondary->m_readers > 0)
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
      s_tagTree.secondary->setRootDebugLevel(level); // Update the secondary tree
      SwapAndWait();
      s_tagTree.secondary->setRootDebugLevel(level); // Update the previously active tree
      InterlockedDecrement(&s_tagTree.secondary->m_writers);
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
         s_tagTree.secondary->add(tag, level);
         SwapAndWait();
         s_tagTree.secondary->add(tag, level);
      }
      else if (level < 0)
      {
         s_tagTree.secondary->remove(tag);
         SwapAndWait();
         s_tagTree.secondary->remove(tag);
      }
      InterlockedDecrement(&s_tagTree.secondary->m_writers);
      s_mutexDebugTagTreeWrite.unlock();
   }
}

/**
 * Get current debug level
 */
void LIBNETXMS_EXPORTABLE nxlog_reset_debug_level_tags()
{
   s_mutexDebugTagTreeWrite.lock();
   s_tagTree.secondary->clear();
   SwapAndWait();
   s_tagTree.secondary->clear();
   InterlockedDecrement(&s_tagTree.secondary->m_writers);
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
      tagTree = s_tagTree.active;
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
				_tcslcpy(s_dailyLogSuffixTemplate, dailySuffix, sizeof(s_dailyLogSuffixTemplate) / sizeof(TCHAR));
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
		      NX_FSTAT(_fileno(s_logFileHandle), &st);
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
bool LIBNETXMS_EXPORTABLE nxlog_open(const TCHAR *logName, UINT32 flags)
{
   if (s_mutexLogAccess == INVALID_MUTEX_HANDLE)
      s_mutexLogAccess = MutexCreate();

	s_flags = flags & 0x7FFFFFFF;
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
		strlcpy(s_syslogName, logName, 64);
#endif
      openlog(s_syslogName, LOG_PID, LOG_DAEMON);
		s_flags |= NXLOG_IS_OPEN;
#endif
   }
   else if (s_flags & NXLOG_USE_SYSTEMD)
   {
      s_flags |= NXLOG_IS_OPEN;
   }
   else if (s_flags & NXLOG_USE_STDOUT)
   {
      s_flags |= NXLOG_IS_OPEN;
      s_flags &= ~NXLOG_PRINT_TO_STDOUT;
      s_logFileHandle = stdout;
   }
   else
   {
      TCHAR buffer[32];

      _tcslcpy(s_logFileName, logName, MAX_PATH);
#if HAVE_FOPEN64
      s_logFileHandle = _tfopen64(logName, _T("a"));
#else
      s_logFileHandle = _tfopen(logName, _T("a"));
#endif
      if (s_logFileHandle != NULL)
      {
			s_flags |= NXLOG_IS_OPEN;
			if (s_flags & NXLOG_JSON_FORMAT)
			{
			   _ftprintf(s_logFileHandle, _T("\n{\"timestamp\":\"%s\",\"severity\":\"info\",\"tag\":\"\",\"message\":\"Log file opened (rotation policy %d, max size ") UINT64_FMT _T(")\"}\n"),
                      FormatLogTimestamp(buffer), s_rotationMode, s_maxLogSize);
			}
			else
			{
            _ftprintf(s_logFileHandle, _T("\n%s Log file opened (rotation policy %d, max size ") UINT64_FMT _T(")\n"),
                      FormatLogTimestamp(buffer), s_rotationMode, s_maxLogSize);
			}
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
      else if (s_flags & (NXLOG_USE_SYSTEMD | NXLOG_USE_STDOUT))
      {
         // Do nothing
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
      }
	   s_flags &= ~NXLOG_IS_OPEN;
   }

   if (s_mutexLogAccess != INVALID_MUTEX_HANDLE)
   {
      MutexDestroy(s_mutexLogAccess);
      s_mutexLogAccess = INVALID_MUTEX_HANDLE;
   }
}

/**
 * Format tag for printing
 */
static inline TCHAR *FormatTag(const TCHAR *tag, TCHAR *tagf)
{
   int i;
   if (tag != NULL)
   {
      for(i = 0; (i < 19) && tag[i] != 0; i++)
         tagf[i] = tag[i];
   }
   else
   {
      i = 0;
   }
   for(; i < 19; i++)
      tagf[i] = ' ';
   tagf[i] = 0;
   return tagf;
}

/**
 * Write log to console (assume that lock already set)
 */
static void WriteLogToConsole(INT16 severity, const TCHAR *timestamp, const TCHAR *tag, const TCHAR *message)
{
   const TCHAR *loglevel;
   switch(severity)
   {
      case NXLOG_ERROR:
         loglevel = _T("*E* [");
         break;
      case NXLOG_WARNING:
         loglevel = _T("*W* [");
         break;
      case NXLOG_INFO:
         loglevel = _T("*I* [");
         break;
      case NXLOG_DEBUG:
         loglevel = _T("*D* [");
         break;
      default:
         loglevel = _T("*?* [");
         break;
   }

   TCHAR tagf[20];
   m_consoleWriter(_T("%s %s%s] %s\n"), timestamp, loglevel, FormatTag(tag, tagf), message);
}

/**
 * Write record to log file (text format)
 */
static void WriteLogToFileAsText(INT16 severity, const TCHAR *tag, const TCHAR *message)
{
   const TCHAR *loglevel;
   switch(severity)
   {
      case NXLOG_ERROR:
         loglevel = _T("*E* [");
	      break;
      case NXLOG_WARNING:
         loglevel = _T("*W* [");
	      break;
      case NXLOG_INFO:
         loglevel = _T("*I* [");
	      break;
      case NXLOG_DEBUG:
         loglevel = _T("*D* [");
	      break;
      default:
         loglevel = _T("*?* [");
	      break;
   }

   TCHAR tagf[20];
   FormatTag(tag, tagf);

   MutexLock(s_mutexLogAccess);

   TCHAR timestamp[64];
   FormatLogTimestamp(timestamp);
   if (s_flags & NXLOG_BACKGROUND_WRITER)
   {
      s_logBuffer.append(timestamp);
      s_logBuffer.append(_T(" "));
      s_logBuffer.append(loglevel);
      s_logBuffer.append(tagf);
      s_logBuffer.append(_T("] "));
      s_logBuffer.append(message);
      s_logBuffer.append(_T("\n"));
   }
   else
   {
	   // Check for new day start
      time_t t = time(NULL);
	   if ((s_rotationMode == NXLOG_ROTATION_DAILY) && (t >= s_currentDayStart + 86400))
	   {
		   RotateLog(FALSE);
	   }

      if (s_logFileHandle != NULL)
	   {
         _ftprintf(s_logFileHandle, _T("%s %s%s] %s\n"), timestamp, loglevel, tagf, message);
		   fflush(s_logFileHandle);
	   }

      // Check log size
	   if ((s_logFileHandle != NULL) && (s_rotationMode == NXLOG_ROTATION_BY_SIZE) && (s_maxLogSize != 0))
	   {
	      NX_STAT_STRUCT st;
		   NX_FSTAT(_fileno(s_logFileHandle), &st);
		   if ((UINT64)st.st_size >= s_maxLogSize)
			   RotateLog(FALSE);
	   }
   }

   if (s_flags & NXLOG_PRINT_TO_STDOUT)
      WriteLogToConsole(severity, timestamp, tag, message);

   MutexUnlock(s_mutexLogAccess);
}

/**
 * Write record to log file (JSON format)
 */
static void WriteLogToFileAsJSON(INT16 severity, const TCHAR *tag, const TCHAR *message)
{
   const char *loglevel;
   switch(severity)
   {
      case NXLOG_ERROR:
         loglevel = "error";
         break;
      case NXLOG_WARNING:
         loglevel = "warning";
         break;
      case NXLOG_INFO:
         loglevel = "info";
         break;
      case NXLOG_DEBUG:
         loglevel = "debug";
         break;
      default:
         loglevel = "info";
         break;
   }

   json_t *json = json_object();
   TCHAR timestamp[64];
   json_object_set_new(json, "timestamp", json_string_t(FormatLogTimestamp(timestamp)));
   json_object_set_new(json, "severity", json_string(loglevel));
   json_object_set_new(json, "tag", json_string_t(CHECK_NULL_EX(tag)));
   json_object_set_new(json, "message", json_string_t(message));
   char *text = json_dumps(json, JSON_PRESERVE_ORDER | JSON_COMPACT);
   json_decref(json);

   MutexLock(s_mutexLogAccess);

   if (s_flags & NXLOG_BACKGROUND_WRITER)
   {
      s_logBuffer.appendMBString(text, strlen(text), CP_UTF8);
      s_logBuffer.append(_T("\n"));
   }
   else
   {
      // Check for new day start
      time_t t = time(NULL);
      if ((s_rotationMode == NXLOG_ROTATION_DAILY) && (t >= s_currentDayStart + 86400))
      {
         RotateLog(FALSE);
      }

      if (s_logFileHandle != NULL)
      {
#ifdef _WIN32
         fwrite(text, 1, strlen(text), s_logFileHandle);
         _fputtc(_T('\n'), s_logFileHandle);
#else
         // write is used here because on linux fwrite is not working
         // after calling fwprintf on a stream
         size_t size = strlen(text);
         size_t offset = 0;
         do
         {
            int bw = write(fileno(s_logFileHandle), &text[offset], size);
            if (bw < 0)
               break;
            size -= bw;
            offset += bw;
         } while(size > 0);
         write(fileno(s_logFileHandle), "\n", 1);
#endif
         fflush(s_logFileHandle);
      }

      // Check log size
      if ((s_logFileHandle != NULL) && (s_rotationMode == NXLOG_ROTATION_BY_SIZE) && (s_maxLogSize != 0))
      {
         NX_STAT_STRUCT st;
         NX_FSTAT(_fileno(s_logFileHandle), &st);
         if ((UINT64)st.st_size >= s_maxLogSize)
            RotateLog(FALSE);
      }
   }

   if (s_flags & NXLOG_PRINT_TO_STDOUT)
      WriteLogToConsole(severity, timestamp, tag, message);

   MutexUnlock(s_mutexLogAccess);
   free(text); // Intentionally use free() instead if MemFree() - memory is allocated by libjansson
}

/**
 * Write record to log file (JSON format)
 */
static inline void WriteLogToFile(INT16 severity, const TCHAR *tag, const TCHAR *message)
{
   if (s_flags & NXLOG_JSON_FORMAT)
      WriteLogToFileAsJSON(severity, tag, message);
   else
      WriteLogToFileAsText(severity, tag, message);
}

/**
 * Format message into local or dynamic buffer
 */
static inline TCHAR *FormatString(msg_buffer_t localBuffer, const TCHAR *format, va_list args)
{
   va_list args2;
   va_copy(args2, args);   // Save arguments for stage 2 if it will be needed

   int ch = _vsntprintf(localBuffer, LOCAL_MSG_BUFFER_SIZE, format, args);
   if ((ch != -1) && (ch < LOCAL_MSG_BUFFER_SIZE))
   {
      va_end(args2);
      return localBuffer;
   }

   size_t bufferSize = (ch != -1) ? (ch + 1) : 65536;
   TCHAR *buffer = MemAllocString(bufferSize);
   _vsntprintf(buffer, bufferSize, format, args2);
   va_end(args2);
   return buffer;
}

/**
 * Free formatted string if needed
 */
static inline void FreeFormattedString(TCHAR *message, msg_buffer_t localBuffer)
{
   if (message != localBuffer)
      MemFree(message);
}

/**
 * Write log record - internal implementation
 */
static void WriteLog(INT16 severity, const TCHAR *tag, const TCHAR *format, va_list args)
{
   if ((severity == NXLOG_DEBUG) && (s_debugWriter != NULL))
   {
      va_list args2;
      va_copy(args2, args);
      MutexLock(s_mutexLogAccess);
      s_debugWriter(tag, format, args2);
      MutexUnlock(s_mutexLogAccess);
      va_end(args2);
   }

   if (!(s_flags & NXLOG_IS_OPEN))
      return;

   if (s_flags & NXLOG_USE_SYSLOG)
   {
      msg_buffer_t localBuffer;
      TCHAR *message = FormatString(localBuffer, format, args);

#ifdef _WIN32
      const TCHAR *strings = message;
      ReportEvent(s_eventLogHandle,
         (severity == NXLOG_DEBUG) ? EVENTLOG_INFORMATION_TYPE : severity,
         0, 1000, NULL, 1, 0, &strings, NULL);
#else /* _WIN32 */
      int level;
      switch(severity)
      {
         case NXLOG_ERROR:
            level = LOG_ERR;
            break;
         case NXLOG_WARNING:
            level = LOG_WARNING;
            break;
         case NXLOG_INFO:
            level = LOG_NOTICE;
            break;
         case NXLOG_DEBUG:
            level = LOG_DEBUG;
            break;
         default:
            level = LOG_INFO;
            break;
      }

#ifdef UNICODE
      char *mbmsg = MBStringFromWideString(message);
      if (tag != NULL)
      {
         char mbtag[64];
         WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, tag, -1, mbtag, 64, NULL, NULL);
         mbtag[63] = 0;
         syslog(level, "[%s] %s", mbtag, mbmsg);
      }
      else
      {
         syslog(level, "%s", mbmsg);
      }
      MemFree(mbmsg);
#else
      if (tag != NULL)
         syslog(level, "[%s] %s", tag, message);
      else
         syslog(level, "%s", message);
#endif
#endif   /* _WIN32 */
      if (s_flags & NXLOG_PRINT_TO_STDOUT)
      {
         MutexLock(s_mutexLogAccess);
         TCHAR timestamp[64];
         WriteLogToConsole(severity, FormatLogTimestamp(timestamp), tag, message);
         MutexUnlock(s_mutexLogAccess);
      }
      FreeFormattedString(message, localBuffer);
   }
   else if (s_flags & NXLOG_USE_SYSTEMD)
   {
      int level;
      switch(severity)
      {
         case NXLOG_ERROR:
            level = 3;
            break;
         case NXLOG_WARNING:
            level = 4;
            break;
         case NXLOG_INFO:
            level = 5;
            break;
         case NXLOG_DEBUG:
            level = 7;
            break;
         default:
            level = 6;
            break;
      }

      TCHAR tagf[20];
      MutexLock(s_mutexLogAccess);
      if (tag != NULL)
         _ftprintf(stderr, _T("<%d>[%s] "), level, FormatTag(tag, tagf));
      else
         _ftprintf(stderr, _T("<%d> "), level);
      _vftprintf(stderr, format, args);
      _fputtc(_T('\n'), stderr);
      fflush(stderr);
      MutexUnlock(s_mutexLogAccess);
   }
   else
   {
      msg_buffer_t buffer;
      TCHAR *message = FormatString(buffer, format, args);
      WriteLogToFile(severity, tag, message);
      FreeFormattedString(message, buffer);
   }
}

/**
 * Write message to log with tag
 */
void LIBNETXMS_EXPORTABLE nxlog_write(INT16 severity, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   WriteLog(severity, NULL, format, args);
   va_end(args);
}

/**
 * Write message to log with tag
 */
void LIBNETXMS_EXPORTABLE nxlog_write2(INT16 severity, const TCHAR *format, va_list args)
{
   WriteLog(severity, NULL, format, args);
}

/**
 * Write message to log with tag
 */
void LIBNETXMS_EXPORTABLE nxlog_write_tag(INT16 severity, const TCHAR *tag, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   WriteLog(severity, tag, format, args);
   va_end(args);
}

/**
 * Write message to log with tag
 */
void LIBNETXMS_EXPORTABLE nxlog_write_tag2(INT16 severity, const TCHAR *tag, const TCHAR *format, va_list args)
{
   WriteLog(severity, tag, format, args);
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
   WriteLog(NXLOG_DEBUG, NULL, format, args);
   va_end(args);
}

/**
 * Write debug message
 */
void LIBNETXMS_EXPORTABLE nxlog_debug2(int level, const TCHAR *format, va_list args)
{
   if (level > nxlog_get_debug_level_tag(_T("*")))
      return;

   WriteLog(NXLOG_DEBUG, NULL, format, args);
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
   WriteLog(NXLOG_DEBUG, tag, format, args);
   va_end(args);
}

/**
 * Write debug message with tag
 */
void LIBNETXMS_EXPORTABLE nxlog_debug_tag2(const TCHAR *tag, int level, const TCHAR *format, va_list args)
{
   if (level > nxlog_get_debug_level_tag(tag))
      return;

   WriteLog(NXLOG_DEBUG, tag, format, args);
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
   WriteLog(NXLOG_DEBUG, fullTag, format, args);
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

   WriteLog(NXLOG_DEBUG, fullTag, format, args);
}

/**
 * Call ReportEvent if writing to Windows Event Log, otherwise write normal message using provided alternative text
 */
void LIBNETXMS_EXPORTABLE nxlog_report_event(DWORD msg, int level, int stringCount, const TCHAR *altMessage, ...)
{
   va_list args;
   va_start(args, altMessage);
#ifdef _WIN32
   if (s_flags & NXLOG_USE_SYSLOG)
   {
      if (s_flags & NXLOG_PRINT_TO_STDOUT)
      {
         va_list args2;
         va_copy(args2, args);
         msg_buffer_t localBuffer;
         TCHAR *message = FormatString(localBuffer, altMessage, args2);
         va_end(args2);

         MutexLock(s_mutexLogAccess);
         TCHAR timestamp[64];
         WriteLogToConsole(level, FormatLogTimestamp(timestamp), NULL, message);
         MutexUnlock(s_mutexLogAccess);
         FreeFormattedString(message, localBuffer);
      }

      const TCHAR **strings = (stringCount > 0) ? (const TCHAR **)alloca(sizeof(TCHAR*) * stringCount) : NULL;
      for (int i = 0; i < stringCount; i++)
         strings[i] = va_arg(args, TCHAR*);
      ReportEvent(s_eventLogHandle, level, 0, msg, NULL, stringCount, 0, strings, NULL);
   }
   else
   {
      WriteLog(level, NULL, altMessage, args);
   }
#else
   WriteLog(level, NULL, altMessage, args);
#endif
   va_end(args);
}
