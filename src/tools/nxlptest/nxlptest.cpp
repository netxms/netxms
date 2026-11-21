/* 
** NetXMS - Network Management System
** NetXMS Log Parser Testing Utility
** Copyright (C) 2009-2025 Victor Kirhenshtein
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
** File: nxlptest.cpp
**
**/

#include "nxlptest.h"
#include <netxms_getopt.h>
#include <netxms-version.h>

#ifndef _WIN32
#include <signal.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxlptest)

/**
 * Help text
 */
static TCHAR m_helpText[] =
   _T("NetXMS Log Parsing Tester  Version ") NETXMS_VERSION_STRING _T("\n")
   _T("Copyright (c) 2009-2026 Victor Kirhenshtein\n\n")
   _T("Usage:\n")
   _T("   nxlptest [options] parser\n\n")
   _T("Where valid options are:\n")
   _T("   -D level   : Set debug level\n")
   _T("   -f file    : Input file (overrides parser settings)\n")
   _T("   -h         : Show this help\n")
   _T("   -i         : Read records from standard input until end of file instead of monitoring file\n")
   _T("   -o offset  : Start offset in file\n")
#ifdef _WIN32
   _T("   -s         : Use VSS snapshots (overrides parser settings)\n")
#endif
   _T("   -v         : Show version and exit\n")
   _T("\n");

/**
 * Debug writer
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
   if (tag != nullptr)
      _tprintf(_T("[%s] "), tag);
   _vtprintf(format, args);
   _fputtc(_T('\n'), stdout);
}

/**
 * File parsing thread
 */
static void ParserThread(LogParser *parser, off_t startOffset)
{
	parser->monitorFile(startOffset);
}

/**
 * Convert record read from standard input and pass it to parser
 */
static void ProcessInputRecord(LogParser *parser, char *record, size_t len, bool utf8)
{
   if ((len > 0) && (record[len - 1] == '\n'))
      record[--len] = 0;
   if ((len > 0) && (record[len - 1] == '\r'))
      record[--len] = 0;

#ifdef UNICODE
   WCHAR *text = utf8 ? WideStringFromUTF8String(record) : WideStringFromMBStringSysLocale(record);
   parser->matchLine(text, _T("stdin"));
   MemFree(text);
#else
   if (utf8)
   {
      char *text = MBStringFromUTF8String(record);
      parser->matchLine(text, _T("stdin"));
      MemFree(text);
   }
   else
   {
      parser->matchLine(record, _T("stdin"));
   }
#endif
}

/**
 * Read records from standard input until end of file and pass them to parser
 */
static void ProcessStandardInput(LogParser *parser, bool utf8)
{
   size_t bufferSize = 4096;
   char *buffer = MemAllocStringA(bufferSize);
   size_t len = 0;
   while(fgets(&buffer[len], static_cast<int>(bufferSize - len), stdin) != nullptr)
   {
      len += strlen(&buffer[len]);
      if ((len == bufferSize - 1) && (buffer[len - 1] != '\n'))
      {
         // Record does not fit into buffer
         bufferSize += 4096;
         buffer = MemRealloc(buffer, bufferSize);
         continue;
      }
      ProcessInputRecord(parser, buffer, len, utf8);
      len = 0;
   }
   if (len > 0)
      ProcessInputRecord(parser, buffer, len, utf8);  // last record without end of line marker
   MemFree(buffer);
}

#ifndef _WIN32

bool s_stop = false;

static void OnBreak(int sig)
{
   s_stop = true;
}

#endif

/**
 * main()
 */
int main(int argc, char *argv[])
{
	int rc = 0;
	TCHAR *inputFile = nullptr;
   off_t startOffset = -1;
   bool useStandardInput = false;
#ifdef _WIN32
   bool vssSnapshots = false;
#endif

   InitNetXMSProcess(true);

   // Parse command line
   opterr = 1;
   int ch;
   while((ch = getopt(argc, argv, "D:f:hio:sv")) != -1)
   {
      switch(ch)
      {
         case 'D':
            nxlog_set_debug_level(strtol(optarg, nullptr, 0));
            break;
         case 'h':
            _tprintf(_T("%s"), m_helpText);
            return 0;
         case 'v':
            _tprintf(_T("NetXMS Log Parsing Tester  Version ") NETXMS_VERSION_STRING _T("\n")
                     _T("Copyright (c) 2009-2026 Victor Kirhenshtein\n\n"));
            return 0;
			case 'f':
#ifdef UNICODE
				inputFile = WideStringFromMBStringSysLocale(optarg);
#else
				inputFile = optarg;
#endif
				break;
         case 'i':
            useStandardInput = true;
            break;
         case 'o':
            startOffset = strtol(optarg, nullptr, 0);
            break;
#ifdef _WIN32
         case 's':
            vssSnapshots = true;
            break;
#endif
         case '?':
            return 1;
         default:
            break;
		}
   }

   if (argc - optind < 1)
   {
      _tprintf(_T("Required arguments missing\n"));
#ifdef UNICODE
      MemFree(inputFile);
#endif
      return 1;
   }

   if (useStandardInput && ((inputFile != nullptr) || (startOffset != -1)))
   {
      _tprintf(_T("Options -f and -o cannot be used together with -i\n"));
#ifdef UNICODE
      MemFree(inputFile);
#endif
      return 1;
   }

   nxlog_set_debug_writer(DebugWriter);

   InitLogParserLibrary();

	size_t size;
	BYTE *xml = LoadFileA(argv[optind], &size);
	if (xml != nullptr)
	{
		TCHAR errorText[1024];

      ObjectArray<LogParser> *parsers = LogParser::createFromXml((const char *)xml, size, errorText, 1024); 
		if ((parsers != nullptr) && (parsers->size() > 0))
		{
         LogParser *parser = parsers->get(0);
         for(int i = 1; i < parsers->size(); i++)
            delete parsers->get(i);

			parser->setCallback(
			   [] (const LogParserCallbackData& data) -> void
			   {
               nxlog_debug_tag(_T("parser"), 3, _T("Parser match (eventCode=%u eventName=%s eventTag=%s) \"%s\""), data.eventCode, CHECK_NULL_EX(data.eventName), CHECK_NULL_EX(data.eventTag), CHECK_NULL_EX(data.originalText));
			   });

         if (useStandardInput)
         {
            int encoding = parser->getFileEncoding();
            if ((encoding == LP_FCP_ACP) || (encoding == LP_FCP_AUTO) || (encoding == LP_FCP_UTF8))
            {
               _tprintf(_T("Reading records from standard input\nDebug level: %d\n\n"), nxlog_get_debug_level());
               ProcessStandardInput(parser, encoding == LP_FCP_UTF8);
               _tprintf(_T("\nRecords processed: %d\nRecords matched: %d\n"), parser->getProcessedRecordsCount(), parser->getMatchedRecordsCount());
            }
            else
            {
               _tprintf(_T("ERROR: file encoding defined in parser is not supported for standard input (only ACP and UTF-8 are supported)\n"));
               rc = 1;
            }
         }
         else
         {
            if (inputFile != nullptr)
               parser->setFileName(inputFile);
#ifdef _WIN32
            if (vssSnapshots)
               parser->setSnapshotMode(true);
#endif

            THREAD thread = ThreadCreateEx(ParserThread, parser, startOffset);
#ifdef _WIN32
            _tprintf(_T("Parser started. Press ESC to stop.\nFile: %s\nDebug level: %d\n\n"), parser->getFileName(), nxlog_get_debug_level());
            while(1)
            {
               ch = _getch();
               if (ch == 27)
                  break;
            }
#else
            _tprintf(_T("Parser started. Press Ctrl+C to stop.\nFile: %s\nDebug level: %d\n\n"), parser->getFileName(), nxlog_get_debug_level());

            signal(SIGINT, OnBreak);

            sigset_t signals;
            sigemptyset(&signals);
            sigaddset(&signals, SIGINT);
            pthread_sigmask(SIG_UNBLOCK, &signals, nullptr);

            while(!s_stop)
               ThreadSleepMs(500);
#endif
            parser->stop();
            ThreadJoin(thread);
         }
         delete parser;
		}
		else
		{
			_tprintf(_T("ERROR: invalid parser definition file (%s)\n"), errorText);
			rc = 1;
		}
		MemFree(xml);
      delete parsers;
	}
	else
	{
		_tprintf(_T("ERROR: unable to load parser definition file (%s)\n"), _tcserror(errno));
		rc = 2;
	}

   CleanupLogParserLibrary();
#ifdef UNICODE
      MemFree(inputFile);
#endif
	return rc;
}
