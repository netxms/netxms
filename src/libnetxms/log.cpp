/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

typedef Buffer<TCHAR, LOCAL_MSG_BUFFER_SIZE> msg_buffer_t;

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
static HANDLE s_eventLogHandle = nullptr;
#else
static char s_syslogName[64];
#endif
static TCHAR s_logFileName[MAX_PATH] = _T("");
static int s_logFileHandle = -1;
static Mutex s_mutexLogAccess(MutexType::FAST);
static uint32_t s_flags = 0;
static int s_rotationMode = NXLOG_ROTATION_BY_SIZE;
static uint64_t s_maxLogSize = 4096 * 1024;	// 4 MB
static int s_logHistorySize = 4;		// Keep up 4 previous log files
static time_t s_lastRotationAttempt = 0;
static TCHAR s_dailyLogSuffixTemplate[64] = _T("%Y%m%d");
static time_t s_currentDayStart = 0;
static NxLogConsoleWriter s_consoleWriter = WriteToTerminalEx;
static StringBuffer s_logBuffer;
static THREAD s_writerThread = INVALID_THREAD_HANDLE;
static Condition s_writerStopCondition(true);
static NxLogDebugWriter s_debugWriter = nullptr;
static volatile DebugTagManager s_tagTree;
static Mutex s_mutexDebugTagTreeWrite(MutexType::FAST);
static NxLogRotationHook s_rotationHook = nullptr;

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
 * Format message into local or dynamic buffer
 */
static inline void FormatString(msg_buffer_t& buffer, const TCHAR *format, va_list args)
{
   va_list args2;
   va_copy(args2, args);   // Save arguments for stage 2 if it will be needed

   int ch = _vsntprintf(buffer, LOCAL_MSG_BUFFER_SIZE, format, args);
   if ((ch != -1) && (ch < LOCAL_MSG_BUFFER_SIZE))
   {
      va_end(args2);
      return;
   }

   size_t bufferSize = (ch != -1) ? (ch + 1) : 65536;
   buffer.realloc(bufferSize);
   _vsntprintf(buffer, bufferSize, format, args2);
   va_end(args2);
}

/**
 * Write string to file
 */
static inline void FileWrite(int fh, const TCHAR *text)
{
#ifdef UNICODE
   size_t len = wchar_utf8len(text, -1);
   Buffer<char, LOCAL_MSG_BUFFER_SIZE> buffer(len + 1);
   wchar_to_utf8(text, -1, buffer, len + 1);
   _write(fh, buffer, strlen(buffer));
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
   msg_buffer_t msg(LOCAL_MSG_BUFFER_SIZE);
   FormatString(msg, format, args);
   va_end(args);
   FileWrite(fh, msg);
}

/**
 * Set debug level
 */
void LIBNETXMS_EXPORTABLE nxlog_set_debug_level(int level)
{
   if ((level < 0) || (level > 9))
      return;

   s_mutexDebugTagTreeWrite.lock();
   s_tagTree.secondary->setRootDebugLevel(level); // Update the secondary tree
   SwapAndWait();
   s_tagTree.secondary->setRootDebugLevel(level); // Update the previously active tree
   InterlockedDecrement(&s_tagTree.secondary->m_writers);
   s_mutexDebugTagTreeWrite.unlock();
}

/**
 * Set debug level for tag
 */
void LIBNETXMS_EXPORTABLE nxlog_set_debug_level_tag(const TCHAR *tag, int level)
{
   if ((tag == nullptr) || !_tcscmp(tag, _T("*")))
   {
      nxlog_set_debug_level(level);
      return;
   }

   if (level > 9)
      return;

   s_mutexDebugTagTreeWrite.lock();
   if (level >= 0)
   {
      s_tagTree.secondary->add(tag, level);
      SwapAndWait();
      s_tagTree.secondary->add(tag, level);
   }
   else
   {
      s_tagTree.secondary->remove(tag);
      SwapAndWait();
      s_tagTree.secondary->remove(tag);
   }
   InterlockedDecrement(&s_tagTree.secondary->m_writers);
   s_mutexDebugTagTreeWrite.unlock();
}

/**
 * Set debug level for tag given as tag:value (helper for setting tags in command line)
 */
void LIBNETXMS_EXPORTABLE nxlog_set_debug_level_tag(const char *definition)
{
   const char *p = strchr(definition, ':');
   if (p == nullptr)
      return;

#ifdef UNICODE
   WCHAR tag[64];
   size_t chars = mb_to_wchar(definition, static_cast<ssize_t>(p - definition), tag, 64);
   tag[chars] = 0;
#else
   char tag[64];
   size_t len = static_cast<size_t>(p - definition);
   if (len > 63)
      len = 63;
   memcpy(tag, definition, len);
   tag[len] = 0;
#endif

   nxlog_set_debug_level_tag(tag, strtol(p + 1, nullptr, 10));
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
int LIBNETXMS_EXPORTABLE nxlog_get_debug_level_tag_object(const TCHAR *tag, uint32_t objectId)
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
void LIBNETXMS_EXPORTABLE nxlog_set_debug_writer(NxLogDebugWriter writer)
{
   s_debugWriter = writer;
}

/**
 * Set log rotation hook
 */
void LIBNETXMS_EXPORTABLE nxlog_set_rotation_hook(NxLogRotationHook hook)
{
   s_rotationHook = hook;
}

/**
 * Format current time for output
 */
static TCHAR *FormatLogTimestamp(TCHAR *buffer)
{
	int64_t now = GetCurrentTimeMs();
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
   if (tag != nullptr)
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
static size_t EscapeForJSON(const TCHAR *text, msg_buffer_t& buffer)
{
   const TCHAR *ch = text;
   TCHAR *out = buffer.buffer();
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
      if ((count > LOCAL_MSG_BUFFER_SIZE - 8) && buffer.isInternal())
      {
         buffer.realloc(_tcslen(text) * 6 + 1);
         out = buffer.buffer() + count;
      }
      ch++;
   }
   *out = 0;
   return count;
}

/**
 * Set timestamp of start of the current day
 */
static void SetDayStart()
{
	time_t now = time(nullptr);
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
	s_consoleWriter = writer;
}

/**
 * Rotate log
 */
static bool RotateLog(bool needLock)
{
	if (needLock)
		s_mutexLogAccess.lock();

	if ((s_flags & NXLOG_ROTATION_ERROR) && (s_lastRotationAttempt > time(nullptr) - 3600))
	{
	   if (needLock)
	      s_mutexLogAccess.unlock();
	   return (s_flags & NXLOG_IS_OPEN) ? true : false;
	}

	if ((s_logFileHandle != -1) && (s_flags & NXLOG_IS_OPEN))
	{
		_close(s_logFileHandle);
		s_flags &= ~NXLOG_IS_OPEN;
	}

	const size_t bufferSize = MAX_PATH * 2 + 256;
	TCHAR *buffer = MemAllocString(bufferSize);
	StringList rotationErrors;
	if (s_rotationMode == NXLOG_ROTATION_BY_SIZE)
	{
	   StringBuffer oldName, newName;

		// Delete old files
	   int32_t i;
		for(i = MAX_LOG_HISTORY_SIZE; i >= s_logHistorySize; i--)
		{
		   oldName = s_logFileName;
		   oldName.append(_T('.'));
		   oldName.append(i);
			if (_taccess(oldName, F_OK) == 0)
			{
            if (_tremove(oldName) != 0)
            {
               _sntprintf(buffer, bufferSize, _T("Rotation error: cannot delete file \"%s\" (%s)"), oldName.cstr(), _tcserror(errno));
               rotationErrors.add(buffer);
            }
			}
		}

		// Shift file names
		for(; i >= 0; i--)
		{
         oldName = s_logFileName;
         oldName.append(_T('.'));
         oldName.append(i);
         if (_taccess(oldName, F_OK) == 0)
         {
            newName = s_logFileName;
            newName.append(_T('.'));
            newName.append(i + 1);
            if (_trename(oldName, newName) != 0)
            {
               _sntprintf(buffer, bufferSize, _T("Rotation error: cannot rename file \"%s\" to \"%s\" (%s)"),
                        oldName.cstr(), newName.cstr(), _tcserror(errno));
               rotationErrors.add(buffer);
            }
         }
		}

		// Rename current log to name.0
      newName = s_logFileName;
      newName.append(_T(".0"));
		if (_trename(s_logFileName, newName) != 0)
		{
         _sntprintf(buffer, bufferSize, _T("Rotation error: cannot rename file \"%s\" to \"%s\" (%s)"),
                  s_logFileName, newName.cstr(), _tcserror(errno));
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
      _tcsftime(buffer, bufferSize, s_dailyLogSuffixTemplate, loc);

		// Rename current log to name.suffix
      StringBuffer newName(s_logFileName);
      newName.append(_T('.'));
      newName.append(buffer);
		if (_trename(s_logFileName, newName) != 0)
		{
         _sntprintf(buffer, bufferSize, _T("Rotation error: cannot rename file \"%s\" to \"%s\" (%s)"),
                  s_logFileName, newName.cstr(), _tcserror(errno));
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
#ifdef UNICODE
         char *message = reinterpret_cast<char*>(buffer);
#else
         char *message = buffer;
#endif
#ifdef UNICODE
#define TIMESTAMP_FORMAT_SPECIFIER  "%ls"
#else
#define TIMESTAMP_FORMAT_SPECIFIER  "%s"
#endif
         if (rotationErrors.isEmpty())
         {
            snprintf(message, bufferSize, "\n{\"timestamp\":\"" TIMESTAMP_FORMAT_SPECIFIER "\",\"severity\":\"info\",\"tag\":\"logger\",\"message\":\"Log file truncated\"}\n",
                   FormatLogTimestamp(timestamp));
            _write(s_logFileHandle, message, strlen(message));
         }
         else
         {
            snprintf(message, bufferSize, "\n{\"timestamp\":\"" TIMESTAMP_FORMAT_SPECIFIER "\",\"severity\":\"error\",\"tag\":\"logger\",\"message\":\"Log file cannot be rotated (detailed error list is following)\"}\n",
                   FormatLogTimestamp(timestamp));
            _write(s_logFileHandle, message, strlen(message));
            for(int i = 0; i < rotationErrors.size(); i++)
            {
               msg_buffer_t escapedText(LOCAL_MSG_BUFFER_SIZE);
               EscapeForJSON(rotationErrors.get(i), escapedText);
               snprintf(message, bufferSize, "\n{\"timestamp\":\"" TIMESTAMP_FORMAT_SPECIFIER "\",\"severity\":\"error\",\"tag\":\"logger\",\"message\":\"" TIMESTAMP_FORMAT_SPECIFIER "\"}\n", timestamp, escapedText.buffer());
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
   s_lastRotationAttempt = time(nullptr);

	if (needLock)
	{
		s_mutexLogAccess.unlock();
		if (s_rotationHook != nullptr)
		   s_rotationHook();
   }
	else
	{
	   // Run rotation hook in background thread
      // This is needed to avoid deadlock if rotation hook tries to write to log
	   NxLogRotationHook rotationHook = s_rotationHook;
      if (rotationHook != nullptr)
      {
         ThreadCreate(
            [rotationHook]() -> void
            {
               rotationHook();
            });
      }
	}

	MemFree(buffer);
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
static void BackgroundWriterThread()
{
   bool stop = false;
   while(!stop)
   {
      stop = s_writerStopCondition.wait(1000);

      // Check for new day start
      time_t t = time(nullptr);
	   if ((s_logFileHandle != -1) && (s_rotationMode == NXLOG_ROTATION_DAILY) && (t >= s_currentDayStart + 86400))
	   {
		   RotateLog(false);
	   }

      s_mutexLogAccess.lock();
      if (!s_logBuffer.isEmpty())
      {
         size_t buflen = s_logBuffer.length();
         char *data = s_logBuffer.getUTF8String();
         s_logBuffer.clear();
         s_mutexLogAccess.unlock();

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
         s_mutexLogAccess.unlock();
      }
   }
}

/**
 * Background writer thread - stdout
 */
static void BackgroundWriterThreadStdOut()
{
   bool stop = false;
   while(!stop)
   {
      stop = s_writerStopCondition.wait(1000);

      s_mutexLogAccess.lock();
      if (!s_logBuffer.isEmpty())
      {
         char *data = s_logBuffer.getUTF8String();
         s_logBuffer.clear();
         s_mutexLogAccess.unlock();

         _write(STDOUT_FILENO, data, strlen(data));
         MemFree(data);
      }
      else
      {
         s_mutexLogAccess.unlock();
      }
   }
}

/**
 * Initialize log
 */
bool LIBNETXMS_EXPORTABLE nxlog_open(const TCHAR *logName, UINT32 flags)
{
	s_flags = flags & 0x7FFFFFFF;
   if (s_flags & NXLOG_USE_SYSLOG)
   {
#ifdef _WIN32
      s_eventLogHandle = RegisterEventSource(NULL, logName);
		if (s_eventLogHandle != NULL)
			s_flags |= NXLOG_IS_OPEN;
#else
#ifdef UNICODE
		WideCharToMultiByteSysLocale(logName, s_syslogName, 64);
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
         s_writerThread = ThreadCreateEx(BackgroundWriterThreadStdOut);
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
            s_writerThread = ThreadCreateEx(BackgroundWriterThread);
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
            s_writerStopCondition.set();
            ThreadJoin(s_writerThread);
            s_writerThread = INVALID_THREAD_HANDLE;
            s_writerStopCondition.reset();
         }
      }
      else
      {
         if (s_flags & NXLOG_BACKGROUND_WRITER)
         {
            s_writerStopCondition.set();
            ThreadJoin(s_writerThread);
            s_writerThread = INVALID_THREAD_HANDLE;
            s_writerStopCondition.reset();
         }

         if (s_logFileHandle != -1)
         {
            _close(s_logFileHandle);
            s_logFileHandle = -1;
         }
      }
	   s_flags &= ~NXLOG_IS_OPEN;
   }
}

/**
 * Colorize log message
 */
static StringBuffer ColorizeMessage(const TCHAR *message)
{
   StringBuffer out;
   int state = 0;
   for(const TCHAR *p = message; *p != 0; p++)
   {
      switch(state)
      {
         case 0:  // normal
            if (*p == '"')
            {
               state = 1;
               out.append(_T("\x1b[36;1m"));
            }
            else if (*p == '[')
            {
               state = 2;
               out.append(_T("\x1b[32m"));
            }
            else if ((*p >= '0') && (*p <= '9') && ((p == message) || _istspace(*(p - 1)) || (*(p - 1) == '=') || (*(p - 1) == '(') || (*(p - 1) == '/')))
            {
               state = ((*p == '0') && (*(p + 1) == 'x')) ? 4 : 3;
               out.append(_T("\x1b[34;1m"));
               if (state == 4)
               {
                  out.append(*p);
                  p++;  // shift to 'x'
               }
            }
            out.append(*p);
            break;
         case 1:  // double quotes
            out.append(*p);
            if (*p == '"')
            {
               state = 0;
               out.append(_T("\x1b[0m"));
            }
            break;
         case 2:  // square brackets
            out.append(*p);
            if (*p == ']')
            {
               state = 0;
               out.append(_T("\x1b[0m"));
            }
            break;
         case 3:
            if (!((*p >= '0') && (*p <= '9')) && (*p != '.'))
            {
               state = 0;
               out.append(_T("\x1b[0m"));
            }
            out.append(*p);
            break;
         case 4:
            if (!(((*p >= '0') && (*p <= '9')) || ((*p >= 'A') && (*p <= 'Z')) || ((*p >= 'a') && (*p <= 'z'))))
            {
               state = 0;
               out.append(_T("\x1b[0m"));
            }
            out.append(*p);
            break;
         default:
            out.append(*p);
            break;
      }
   }
   if (state != 0)
      out.append(_T("\x1b[0m"));
   return out;
}

/**
 * Write log to console (assume that lock already set)
 */
static void WriteLogToConsole(int16_t severity, const TCHAR *timestamp, const TCHAR *tag, const TCHAR *message)
{
   const TCHAR *loglevel;
   switch(severity)
   {
      case NXLOG_ERROR:
         loglevel = _T("\x1b[31;1m*E*\x1b[0m \x1b[33m[");
         break;
      case NXLOG_WARNING:
         loglevel = _T("\x1b[33;1m*W*\x1b[0m \x1b[33m[");
         break;
      case NXLOG_INFO:
         loglevel = _T("\x1b[32;1m*I*\x1b[0m \x1b[33m[");
         break;
      case NXLOG_DEBUG:
         loglevel = _T("\x1b[34;1m*D*\x1b[0m \x1b[33m[");
         break;
      default:
         loglevel = _T("*?* \x1b[33m[");
         break;
   }

   TCHAR tagf[20];
   s_consoleWriter(_T("\x1b[36m%s\x1b[0m %s%s]\x1b[0m %s\n"), timestamp, loglevel, FormatTag(tag, tagf), ColorizeMessage(message).cstr());
}

/**
 * Write record to log file (text format)
 */
static void WriteLogToFileAsText(int16_t severity, const TCHAR *tag, const TCHAR *message)
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

   s_mutexLogAccess.lock();

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

   s_mutexLogAccess.unlock();
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

   msg_buffer_t escapedTag(LOCAL_MSG_BUFFER_SIZE), escapedMessage(LOCAL_MSG_BUFFER_SIZE);
   size_t tagLen = EscapeForJSON(CHECK_NULL_EX(tag), escapedTag);
   size_t messageLen = EscapeForJSON(message, escapedMessage);

   msg_buffer_t json(tagLen + messageLen + 128);
   TCHAR timestamp[64];
   _tcscpy(json, _T("{\"timestamp\":\""));
   _tcscat(json, FormatLogTimestamp(timestamp));
   _tcscat(json, _T("\",\"severity\":\""));
   _tcscat(json, loglevel);
   _tcscat(json, _T("\",\"tag\":\""));
   _tcscat(json, escapedTag);
   _tcscat(json, _T("\",\"message\":\""));
   _tcscat(json, escapedMessage);
   _tcscat(json, _T("\"}\n"));

   s_mutexLogAccess.lock();

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
      time_t t = time(nullptr);
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
         if (static_cast<uint64_t>(st.st_size) >= s_maxLogSize)
            RotateLog(false);
      }
   }

   if (s_flags & NXLOG_PRINT_TO_STDOUT)
      WriteLogToConsole(severity, timestamp, tag, message);

   s_mutexLogAccess.unlock();
}

/**
 * Write record to log file (JSON format)
 */
static inline void WriteLogToFile(int16_t severity, const TCHAR *tag, const TCHAR *message)
{
   if (s_flags & NXLOG_JSON_FORMAT)
      WriteLogToFileAsJSON(severity, tag, message);
   else
      WriteLogToFileAsText(severity, tag, message);
}

/**
 * Write log record - internal implementation
 */
static void WriteLog(int16_t severity, const TCHAR *tag, const TCHAR *format, va_list args)
{
   if (s_debugWriter != nullptr)
   {
      va_list args2;
      va_copy(args2, args);
      s_mutexLogAccess.lock();
      s_debugWriter(tag, format, args2);
      s_mutexLogAccess.unlock();
      va_end(args2);
   }

   if (!(s_flags & NXLOG_IS_OPEN))
      return;

   if (s_flags & NXLOG_USE_SYSLOG)
   {
      msg_buffer_t message(LOCAL_MSG_BUFFER_SIZE);
      FormatString(message, format, args);

#ifdef _WIN32
      const TCHAR *strings = message.buffer();
      ReportEvent(s_eventLogHandle,
         (severity == NXLOG_DEBUG) ? EVENTLOG_INFORMATION_TYPE : severity,
         0, 1000, nullptr, 1, 0, &strings, nullptr);
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
      if (tag != nullptr)
      {
         char mbtag[64];
         wchar_to_mb(tag, -1, mbtag, 64);
         mbtag[63] = 0;
         syslog(level, "[%s] %s", mbtag, mbmsg);
      }
      else
      {
         syslog(level, "%s", mbmsg);
      }
      MemFree(mbmsg);
#else
      if (tag != nullptr)
         syslog(level, "[%s] %s", tag, message.buffer());
      else
         syslog(level, "%s", message.buffer());
#endif
#endif   /* _WIN32 */
      if (s_flags & NXLOG_PRINT_TO_STDOUT)
      {
         s_mutexLogAccess.lock();
         TCHAR timestamp[64];
         WriteLogToConsole(severity, FormatLogTimestamp(timestamp), tag, message);
         s_mutexLogAccess.unlock();
      }
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
      s_mutexLogAccess.lock();
      if (tag != NULL)
         _ftprintf(stderr, _T("<%d>[%s] "), level, FormatTag(tag, tagf));
      else
         _ftprintf(stderr, _T("<%d> "), level);
      _vftprintf(stderr, format, args);
      _fputtc(_T('\n'), stderr);
      fflush(stderr);
      s_mutexLogAccess.unlock();
   }
   else
   {
      msg_buffer_t message(LOCAL_MSG_BUFFER_SIZE);
      FormatString(message, format, args);
      WriteLogToFile(severity, tag, message);
   }
}

/**
 * Write message to log with tag
 */
void LIBNETXMS_EXPORTABLE nxlog_write(int16_t severity, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   WriteLog(severity, nullptr, format, args);
   va_end(args);
}

/**
 * Write message to log with tag
 */
void LIBNETXMS_EXPORTABLE nxlog_write2(int16_t severity, const TCHAR *format, va_list args)
{
   WriteLog(severity, nullptr, format, args);
}

/**
 * Write message to log with tag
 */
void LIBNETXMS_EXPORTABLE nxlog_write_tag(int16_t severity, const TCHAR *tag, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   WriteLog(severity, tag, format, args);
   va_end(args);
}

/**
 * Write message to log with tag
 */
void LIBNETXMS_EXPORTABLE nxlog_write_tag2(int16_t severity, const TCHAR *tag, const TCHAR *format, va_list args)
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
   WriteLog(NXLOG_DEBUG, nullptr, format, args);
   va_end(args);
}

/**
 * Write debug message
 */
void LIBNETXMS_EXPORTABLE nxlog_debug2(int level, const TCHAR *format, va_list args)
{
   if (level > nxlog_get_debug_level_tag(_T("*")))
      return;

   WriteLog(NXLOG_DEBUG, nullptr, format, args);
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
         msg_buffer_t message(LOCAL_MSG_BUFFER_SIZE);
         FormatString(message, altMessage, args2);
         va_end(args2);

         s_mutexLogAccess.lock();
         TCHAR timestamp[64];
         WriteLogToConsole(level, FormatLogTimestamp(timestamp), nullptr, message);
         s_mutexLogAccess.unlock();
      }

      const TCHAR **strings = (stringCount > 0) ? (const TCHAR **)alloca(sizeof(TCHAR*) * stringCount) : nullptr;
      for (int i = 0; i < stringCount; i++)
         strings[i] = va_arg(args, TCHAR*);
      ReportEvent(s_eventLogHandle, level, 0, msg, nullptr, stringCount, 0, strings, nullptr);
   }
   else
   {
      WriteLog(level, nullptr, altMessage, args);
   }
#else
   WriteLog(level, nullptr, altMessage, args);
#endif
   va_end(args);
}
