/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2020 Victor Kirhenshtein
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

#ifdef _WIN32
static void DefaultConsoleWriter(const TCHAR *, ...);
#endif

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
static int s_logFileHandle = -1;
static MUTEX s_mutexLogAccess = INVALID_MUTEX_HANDLE;
static UINT32 s_flags = 0;
static int s_rotationMode = NXLOG_ROTATION_BY_SIZE;
static UINT64 s_maxLogSize = 4096 * 1024;	// 4 MB
static int s_logHistorySize = 4;		// Keep up 4 previous log files
static time_t s_lastRotationAttempt = 0;
static TCHAR s_dailyLogSuffixTemplate[64] = _T("%Y%m%d");
static time_t s_currentDayStart = 0;
#ifdef _WIN32
static NxLogConsoleWriter m_consoleWriter = DefaultConsoleWriter;
#else
static NxLogConsoleWriter m_consoleWriter = (NxLogConsoleWriter)_tprintf;
#endif
static StringBuffer s_logBuffer;
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
 * Allocate string buffer on heap if requested size is bigger than local buffer size
 */
static inline TCHAR *AllocateStringBuffer(size_t size, TCHAR *localBuffer)
{
   return (size > LOCAL_MSG_BUFFER_SIZE) ? MemAllocString(size) : localBuffer;
}

/**
 * Allocate string buffer on heap if requested size is bigger than local buffer size
 */
static inline char *AllocateStringBufferA(size_t size, char *localBuffer)
{
   return (size > LOCAL_MSG_BUFFER_SIZE) ? MemAllocStringA(size) : localBuffer;
}

/**
 * Free string buffer if it was allocated on heap
 */
static inline void FreeStringBuffer(void *buffer, void *localBuffer)
{
   if (buffer != localBuffer)
      MemFree(buffer);
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
 * Write string to file
 */
static inline void FileWrite(int fh, const TCHAR *text)
{
#ifdef UNICODE
   size_t len = wchar_utf8len(text, -1);
   char localBuffer[LOCAL_MSG_BUFFER_SIZE];
   char *buffer = AllocateStringBufferA(len + 1, localBuffer);
   wchar_to_utf8(text, -1, buffer, len + 1);
   _write(fh, buffer, strlen(buffer));
   FreeStringBuffer(buffer, localBuffer);
#else
   _write(fh, text, strlen(text));
#endif
}

/**
 * Write formatted string to file
 */
static inline void FileFormattedWrite(int fh, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   msg_buffer_t localBuffer;
   TCHAR *msg = FormatString(localBuffer, format, args);
   va_end(args);
   FileWrite(fh, msg);
   FreeFormattedString(msg, localBuffer);
}

#ifdef _WIN32

/**
 * Default console writer for Windows
 */
static void DefaultConsoleWriter(const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   msg_buffer_t localBuffer;
   TCHAR *msg = FormatString(localBuffer, format, args);
   va_end(args);
   FileWrite(STDOUT_FILENO, msg);
   FreeFormattedString(msg, localBuffer);
}

#endif

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
 * Release previously acquired tag tree
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
 * Escape string for JSON. Use local buffer if possible and allocate large buffer on heap if string is too long.
 */
static TCHAR *EscapeForJSON(const TCHAR *text, TCHAR *localBuffer, size_t *len)
{
   const TCHAR *ch = text;
   TCHAR *buffer = localBuffer;
   TCHAR *out = localBuffer;
   size_t count = 0;
   while(*ch != 0)
   {
      switch(*ch)
      {
         case _T('"'):
         case _T('\\'):
            *(out++) = _T('\\');
            *(out++) = *ch;
            count += 2;
            break;
         case 0x08:
            *(out++) = _T('\\');
            *(out++) = _T('b');
            count += 2;
            break;
         case 0x09:
            *(out++) = _T('\\');
            *(out++) = _T('t');
            count += 2;
            break;
         case 0x0A:
            *(out++) = _T('\\');
            *(out++) = _T('n');
            count += 2;
            break;
         case 0x0C:
            *(out++) = _T('\\');
            *(out++) = _T('f');
            count += 2;
            break;
         case 0x0D:
            *(out++) = _T('\\');
            *(out++) = _T('r');
            count += 2;
            break;
         default:
            if (*ch < 0x20)
            {
               TCHAR chcode[8];
               _sntprintf(chcode, 8, _T("\\u%04X"), *ch);
               memcpy(out, chcode, 6 * sizeof(TCHAR));
               out += 6;
               count += 6;
            }
            else
            {
               *(out++) = *ch;
               count++;
            }
            break;
      }
      if ((count > LOCAL_MSG_BUFFER_SIZE - 8) && (buffer == localBuffer))
      {
         buffer = MemAllocString(_tcslen(text) * 6 + 1);
         memcpy(buffer, localBuffer, count * sizeof(TCHAR));
         out = buffer + count;
      }
      ch++;
   }
   *out = 0;
   *len = count;
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

	if ((rotationMode >= 0) && (rotationMode <= 2))
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
	   nxlog_write_tag(NXLOG_INFO, _T("logger"), _T("Log rotation policy set to %d (size=") UINT64_FMT _T(", count=%d)"), rotationMode, maxLogSize, historySize);

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
	if (needLock)
		MutexLock(s_mutexLogAccess);

	if ((s_flags & NXLOG_ROTATION_ERROR) && (s_lastRotationAttempt > time(NULL) - 3600))
	{
	   if (needLock)
	      MutexUnlock(s_mutexLogAccess);
	   return (s_flags & NXLOG_IS_OPEN) ? true : false;
	}

	if ((s_logFileHandle != -1) && (s_flags & NXLOG_IS_OPEN))
	{
		_close(s_logFileHandle);
		s_flags &= ~NXLOG_IS_OPEN;
	}

	StringList rotationErrors;
	if (s_rotationMode == NXLOG_ROTATION_BY_SIZE)
	{
	   TCHAR oldName[MAX_PATH], newName[MAX_PATH];

		// Delete old files
	   int i;
		for(i = MAX_LOG_HISTORY_SIZE; i >= s_logHistorySize; i--)
		{
			_sntprintf(oldName, MAX_PATH, _T("%s.%d"), s_logFileName, i);
			if (_taccess(oldName, 0) == 0)
			{
            if (_tunlink(oldName) != 0)
            {
               TCHAR buffer[1024];
               _sntprintf(buffer, 1024, _T("Rotation error: cannot delete file \"%s\" (%s)"), oldName, _tcserror(errno));
               rotationErrors.add(buffer);
            }
			}
		}

		// Shift file names
		for(; i >= 0; i--)
		{
			_sntprintf(oldName, MAX_PATH, _T("%s.%d"), s_logFileName, i);
			_sntprintf(newName, MAX_PATH, _T("%s.%d"), s_logFileName, i + 1);
			if (_trename(oldName, newName) != 0)
         {
            TCHAR buffer[1024];
            _sntprintf(buffer, 1024, _T("Rotation error: cannot rename file \"%s\" to \"%s\" (%s)"), oldName, newName, _tcserror(errno));
            rotationErrors.add(buffer);
         }
		}

		// Rename current log to name.0
		_sntprintf(newName, MAX_PATH, _T("%s.0"), s_logFileName);
		if (_trename(s_logFileName, newName) != 0)
		{
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("Rotation error: cannot rename file \"%s\" to \"%s\" (%s)"), s_logFileName, newName, _tcserror(errno));
         rotationErrors.add(buffer);
		}
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
      TCHAR newName[MAX_PATH];
		_sntprintf(newName, MAX_PATH, _T("%s.%s"), s_logFileName, buffer);
		if (_trename(s_logFileName, newName) != 0)
		{
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("Rotation error: cannot rename file \"%s\" to \"%s\" (%s)"), s_logFileName, newName, _tcserror(errno));
         rotationErrors.add(buffer);
		}

		SetDayStart();
	}

   // Reopen log
   s_logFileHandle = _topen(s_logFileName, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
   if (s_logFileHandle != -1)
   {
      s_flags |= NXLOG_IS_OPEN;

      TCHAR timestamp[32];
      if (s_flags & NXLOG_JSON_FORMAT)
      {
         char message[1024];
#ifdef UNICODE
#define TIMESTAMP_FORMAT_SPECIFIER  "%ls"
#else
#define TIMESTAMP_FORMAT_SPECIFIER  "%s"
#endif
         if (rotationErrors.isEmpty())
         {
            snprintf(message, 1024, "\n{\"timestamp\":\"" TIMESTAMP_FORMAT_SPECIFIER "\",\"severity\":\"info\",\"tag\":\"logger\",\"message\":\"Log file truncated\"}\n",
                   FormatLogTimestamp(timestamp));
            _write(s_logFileHandle, message, strlen(message));
         }
         else
         {
            snprintf(message, 1024, "\n{\"timestamp\":\"" TIMESTAMP_FORMAT_SPECIFIER "\",\"severity\":\"error\",\"tag\":\"logger\",\"message\":\"Log file cannot be rotated (detailed error list is following)\"}\n",
                   FormatLogTimestamp(timestamp));
            _write(s_logFileHandle, message, strlen(message));
            for(int i = 0; i < rotationErrors.size(); i++)
            {
               TCHAR escapedTextBuffer[LOCAL_MSG_BUFFER_SIZE];
               size_t textLen;
               TCHAR *escapedText = EscapeForJSON(rotationErrors.get(i), escapedTextBuffer, &textLen);
               snprintf(message, 1024, "\n{\"timestamp\":\"" TIMESTAMP_FORMAT_SPECIFIER "\",\"severity\":\"error\",\"tag\":\"logger\",\"message\":\"" TIMESTAMP_FORMAT_SPECIFIER "\"}\n",
                      timestamp, escapedText);
               FreeStringBuffer(escapedText, escapedTextBuffer);
               _write(s_logFileHandle, message, strlen(message));
            }
         }
#undef TIMESTAMP_FORMAT_SPECIFIER
      }
      else
      {
         TCHAR tagf[20];
         FormatTag(_T("logger"), tagf);
         if (rotationErrors.isEmpty())
         {
            FileFormattedWrite(s_logFileHandle, _T("%s *I* [%s] Log file truncated\n"), FormatLogTimestamp(timestamp), tagf);
         }
         else
         {
            FileFormattedWrite(s_logFileHandle, _T("%s *E* [%s] Log file cannot be rotated (detailed error list is following)\n"), FormatLogTimestamp(timestamp), tagf);
            for(int i = 0; i < rotationErrors.size(); i++)
            {
               FileFormattedWrite(s_logFileHandle, _T("%s *E* [%s] %s\n"), FormatLogTimestamp(timestamp), tagf, rotationErrors.get(i));
            }
         }
      }

#ifndef _WIN32
      fcntl(s_logFileHandle, F_SETFD, fcntl(s_logFileHandle, F_GETFD) | FD_CLOEXEC);
#endif
   }

   if (rotationErrors.isEmpty())
      s_flags &= ~NXLOG_ROTATION_ERROR;
   else
      s_flags |= NXLOG_ROTATION_ERROR;
   s_lastRotationAttempt = time(NULL);

	if (needLock)
		MutexUnlock(s_mutexLogAccess);

	return (s_flags & NXLOG_IS_OPEN) ? true : false;
}

/**
 * API for application to force log rotation
 */
bool LIBNETXMS_EXPORTABLE nxlog_rotate()
{
   return (s_logFileHandle != -1) ? RotateLog(true) : false;
}

/**
 * Background writer thread - file
 */
static THREAD_RESULT THREAD_CALL BackgroundWriterThread(void *arg)
{
   bool stop = false;
   while(!stop)
   {
      stop = ConditionWait(s_writerStopCondition, 1000);

	   // Check for new day start
      time_t t = time(NULL);
	   if ((s_logFileHandle != -1) && (s_rotationMode == NXLOG_ROTATION_DAILY) && (t >= s_currentDayStart + 86400))
	   {
		   RotateLog(false);
	   }

      MutexLock(s_mutexLogAccess);
      if (!s_logBuffer.isEmpty())
      {
         size_t buflen = s_logBuffer.length();
         char *data = s_logBuffer.getUTF8String();
         s_logBuffer.clear();
         MutexUnlock(s_mutexLogAccess);

         if (s_logFileHandle != -1)
         {
            if (s_flags & NXLOG_DEBUG_MODE)
            {
               char buffer[256];
               snprintf(buffer, 256, "##(" INT64_FMTA ")" INT64_FMTA " @" INT64_FMTA "\n", (int64_t)buflen, (int64_t)strlen(data), GetCurrentTimeMs());
               _write(s_logFileHandle, buffer, strlen(buffer));
            }

            _write(s_logFileHandle, data, strlen(data));

            // Check log size
            if ((s_rotationMode == NXLOG_ROTATION_BY_SIZE) && (s_maxLogSize != 0))
            {
               NX_STAT_STRUCT st;
               NX_FSTAT(s_logFileHandle, &st);
               if ((UINT64)st.st_size >= s_maxLogSize)
                  RotateLog(false);
            }
         }

	      MemFree(data);
      }
      else
      {
         MutexUnlock(s_mutexLogAccess);
      }
   }
   return THREAD_OK;
}

/**
 * Background writer thread - stdout
 */
static THREAD_RESULT THREAD_CALL BackgroundWriterThreadStdOut(void *arg)
{
   bool stop = false;
   while(!stop)
   {
      stop = ConditionWait(s_writerStopCondition, 1000);

      MutexLock(s_mutexLogAccess);
      if (!s_logBuffer.isEmpty())
      {
         char *data = s_logBuffer.getUTF8String();
         s_logBuffer.clear();
         MutexUnlock(s_mutexLogAccess);

         _write(STDOUT_FILENO, data, strlen(data));
         MemFree(data);
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
      s_mutexLogAccess = MutexCreateFast();

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
      s_flags &= ~NXLOG_PRINT_TO_STDOUT;
   }
   else if (s_flags & NXLOG_USE_STDOUT)
   {
      s_flags |= NXLOG_IS_OPEN;
      s_flags &= ~NXLOG_PRINT_TO_STDOUT;
      if (s_flags & NXLOG_BACKGROUND_WRITER)
      {
         s_logBuffer.setAllocationStep(8192);
         s_writerStopCondition = ConditionCreate(TRUE);
         s_writerThread = ThreadCreateEx(BackgroundWriterThreadStdOut, 0, NULL);
      }
   }
   else
   {
      _tcslcpy(s_logFileName, logName, MAX_PATH);
      s_logFileHandle = _topen(logName, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if (s_logFileHandle != -1)
      {
			s_flags |= NXLOG_IS_OPEN;

         TCHAR timestamp[32];
         char message[1024];
#ifdef UNICODE
#define TIMESTAMP_FORMAT_SPECIFIER  "%ls"
#else
#define TIMESTAMP_FORMAT_SPECIFIER  "%s"
#endif
         if (s_flags & NXLOG_JSON_FORMAT)
         {
            snprintf(message, 1024, "\n{\"timestamp\":\"" TIMESTAMP_FORMAT_SPECIFIER "\",\"severity\":\"info\",\"tag\":\"logger\",\"message\":\"Log file opened (rotation policy %d, max size " INT64_FMTA ")\"}\n",
                   FormatLogTimestamp(timestamp), s_rotationMode, s_maxLogSize);
         }
         else
         {
            TCHAR tagf[20];
            FormatTag(_T("logger"), tagf);
            snprintf(message, 1024, "\n" TIMESTAMP_FORMAT_SPECIFIER " *I* [" TIMESTAMP_FORMAT_SPECIFIER "] Log file opened (rotation policy %d, max size " UINT64_FMTA ")\n",
                   FormatLogTimestamp(timestamp), tagf, s_rotationMode, s_maxLogSize);
         }
#undef TIMESTAMP_FORMAT_SPECIFIER
         _write(s_logFileHandle, message, strlen(message));

#ifndef _WIN32
         fcntl(s_logFileHandle, F_SETFD, fcntl(s_logFileHandle, F_GETFD) | FD_CLOEXEC);
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
      else if (s_flags & NXLOG_USE_SYSTEMD)
      {
         // Do nothing
      }
      else if (s_flags & NXLOG_USE_STDOUT)
      {
         if (s_flags & NXLOG_BACKGROUND_WRITER)
         {
            ConditionSet(s_writerStopCondition);
            ThreadJoin(s_writerThread);
            ConditionDestroy(s_writerStopCondition);
            s_writerThread = INVALID_THREAD_HANDLE;
            s_writerStopCondition = INVALID_CONDITION_HANDLE;
         }
      }
      else
      {
         if (s_flags & NXLOG_BACKGROUND_WRITER)
         {
            ConditionSet(s_writerStopCondition);
            ThreadJoin(s_writerThread);
            ConditionDestroy(s_writerStopCondition);
            s_writerThread = INVALID_THREAD_HANDLE;
            s_writerStopCondition = INVALID_CONDITION_HANDLE;
         }

         if (s_logFileHandle != -1)
         {
            _close(s_logFileHandle);
            s_logFileHandle = -1;
         }
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
   else if (s_flags & NXLOG_USE_STDOUT)
   {
      FileFormattedWrite(STDOUT_FILENO, _T("%s %s%s] %s\n"), timestamp, loglevel, tagf, message);
   }
   else if (s_logFileHandle != -1)
   {
	   // Check for new day start
      time_t t = time(NULL);
	   if ((s_rotationMode == NXLOG_ROTATION_DAILY) && (t >= s_currentDayStart + 86400))
	   {
		   RotateLog(false);
	   }

	   FileFormattedWrite(s_logFileHandle, _T("%s %s%s] %s\n"), timestamp, loglevel, tagf, message);

      // Check log size
	   if ((s_rotationMode == NXLOG_ROTATION_BY_SIZE) && (s_maxLogSize != 0))
	   {
	      NX_STAT_STRUCT st;
		   NX_FSTAT(s_logFileHandle, &st);
		   if ((UINT64)st.st_size >= s_maxLogSize)
			   RotateLog(false);
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
   const TCHAR *loglevel;
   switch(severity)
   {
      case NXLOG_ERROR:
         loglevel = _T("error");
         break;
      case NXLOG_WARNING:
         loglevel = _T("warning");
         break;
      case NXLOG_INFO:
         loglevel = _T("info");
         break;
      case NXLOG_DEBUG:
         loglevel = _T("debug");
         break;
      default:
         loglevel = _T("info");
         break;
   }

   TCHAR escapedTagBuffer[LOCAL_MSG_BUFFER_SIZE], escapedMessageBuffer[LOCAL_MSG_BUFFER_SIZE];
   size_t tagLen, messageLen;
   TCHAR *escapedTag = EscapeForJSON(CHECK_NULL_EX(tag), escapedTagBuffer, &tagLen);
   TCHAR *escapedMessage = EscapeForJSON(message, escapedMessageBuffer, &messageLen);

   TCHAR jsonBuffer[LOCAL_MSG_BUFFER_SIZE], timestamp[64];
   TCHAR *json = AllocateStringBuffer(tagLen + messageLen + 128, jsonBuffer);
   _tcscpy(json, _T("{\"timestamp\":\""));
   _tcscat(json, FormatLogTimestamp(timestamp));
   _tcscat(json, _T("\",\"severity\":\""));
   _tcscat(json, loglevel);
   _tcscat(json, _T("\",\"tag\":\""));
   _tcscat(json, escapedTag);
   _tcscat(json, _T("\",\"message\":\""));
   _tcscat(json, escapedMessage);
   _tcscat(json, _T("\"}\n"));

   MutexLock(s_mutexLogAccess);

   if (s_flags & NXLOG_BACKGROUND_WRITER)
   {
      s_logBuffer.append(json);
   }
   else if (s_flags & NXLOG_USE_STDOUT)
   {
      FileWrite(STDOUT_FILENO, json);
   }
   else if (s_logFileHandle != -1)
   {
      // Check for new day start
      time_t t = time(NULL);
      if ((s_rotationMode == NXLOG_ROTATION_DAILY) && (t >= s_currentDayStart + 86400))
      {
         RotateLog(false);
      }

      FileWrite(s_logFileHandle, json);

      // Check log size
      if ((s_rotationMode == NXLOG_ROTATION_BY_SIZE) && (s_maxLogSize != 0))
      {
         NX_STAT_STRUCT st;
         NX_FSTAT(s_logFileHandle, &st);
         if ((UINT64)st.st_size >= s_maxLogSize)
            RotateLog(false);
      }
   }

   if (s_flags & NXLOG_PRINT_TO_STDOUT)
      WriteLogToConsole(severity, timestamp, tag, message);

   MutexUnlock(s_mutexLogAccess);

   FreeStringBuffer(json, jsonBuffer);
   FreeStringBuffer(escapedMessage, escapedMessageBuffer);
   FreeStringBuffer(escapedTag, escapedTagBuffer);
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
