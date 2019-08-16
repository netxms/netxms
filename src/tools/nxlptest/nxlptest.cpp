/* 
** NetXMS - Network Management System
** NetXMS Log Parser Testing Utility
** Copyright (C) 2009-2018 Victor Kirhenshtein
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

#ifndef _WIN32
#include <signal.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxlptest)

/**
 * Help text
 */
static TCHAR m_helpText[] =
   _T("NetXMS Log Parsing Tester  Version ") NETXMS_VERSION_STRING _T("\n")
   _T("Copyright (c) 2009-2012 Victor Kirhenshtein\n\n")
   _T("Usage:\n")
   _T("   nxlptest [options] parser\n\n")
   _T("Where valid options are:\n")
   _T("   -D level   : Set debug level\n")
   _T("   -f file    : Input file (overrides parser settings)\n")
   _T("   -h         : Show this help\n")
	_T("   -i         : Use standard input instead of file defined in parser\n" )
#ifdef _WIN32
   _T("   -s         : Use VSS snapshots (overrides parser settings)\n")
#endif
   _T("   -t level   : Set trace level (overrides parser settings)\n")
   _T("   -v         : Show version and exit\n")
   _T("\n");

/**
 * Debug writer
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
   if (tag != NULL)
      _tprintf(_T("[%s] "), tag);
   _vtprintf(format, args);
   _fputtc(_T('\n'), stdout);
}

/**
 * File parsing thread
 */
static THREAD_RESULT THREAD_CALL ParserThread(void *arg)
{
	((LogParser *)arg)->monitorFile();
	return THREAD_OK;
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
	int rc = 0, ch, traceLevel = -1;
	BYTE *xml;
	UINT32 size;
	TCHAR *inputFile = NULL;
#ifdef _WIN32
   bool vssSnapshots = false;
#endif

   InitNetXMSProcess(true);

   // Parse command line
   opterr = 1;
	while((ch = getopt(argc, argv, "D:f:hist:v")) != -1)
   {
		switch(ch)
		{
         case 'D':
            nxlog_set_debug_level(strtol(optarg, NULL, 0));
            break;
			case 'h':
				_tprintf(_T("%s"), m_helpText);
            return 0;
         case 'v':
				_tprintf(_T("NetXMS Log Parsing Tester  Version ") NETXMS_VERSION_STRING _T("\n")
				         _T("Copyright (c) 2009-2017 Victor Kirhenshtein\n\n"));
            return 0;
			case 'f':
#ifdef UNICODE
				inputFile = WideStringFromMBString(optarg);
#else
				inputFile = optarg;
#endif
				break;
#ifdef _WIN32
         case 's':
            vssSnapshots = true;
            break;
#endif
			case 't':
				traceLevel = strtol(optarg, NULL, 0);
				break;
         case '?':
            return 1;
         default:
            break;
		}
   }

   if (argc - optind < 1)
   {
      _tprintf(_T("Required arguments missing\n"));
      return 1;
   }

   nxlog_set_debug_writer(DebugWriter);

   InitLogParserLibrary();

#ifdef UNICODE
	WCHAR *wname = WideStringFromMBString(argv[optind]);
	xml = LoadFile(wname, &size);
	MemFree(wname);
#else
	xml = LoadFile(argv[optind], &size);
#endif
	if (xml != NULL)
	{
		TCHAR errorText[1024];

      ObjectArray<LogParser> *parsers = LogParser::createFromXml((const char *)xml, size, errorText, 1024); 
		if ((parsers != NULL) && (parsers->size() > 0))
		{
         LogParser *parser = parsers->get(0);
         for(int i = 1; i < parsers->size(); i++)
            delete parsers->get(i);
			if (traceLevel != -1)
				parser->setTraceLevel(traceLevel);
			if (inputFile != NULL)
				parser->setFileName(inputFile);
#ifdef _WIN32
         if (vssSnapshots)
            parser->setSnapshotMode(true);
#endif

			THREAD thread = ThreadCreateEx(ParserThread, 0, parser);
#ifdef _WIN32
			_tprintf(_T("Parser started. Press ESC to stop.\nFile: %s\nTrace level: %d\n\n"),
				      parser->getFileName(), parser->getTraceLevel());
			while(1)
			{
				ch = _getch();
				if (ch == 27)
					break;
			}
#else
			_tprintf(_T("Parser started. Press Ctrl+C to stop.\nFile: %s\nTrace level: %d\n\n"),
				      parser->getFileName(), parser->getTraceLevel());

         signal(SIGINT, OnBreak);

         sigset_t signals;
         sigemptyset(&signals);
         sigaddset(&signals, SIGINT);
         pthread_sigmask(SIG_UNBLOCK, &signals, NULL);

			while(!s_stop)
            ThreadSleepMs(500);
#endif
         parser->stop();
         ThreadJoin(thread);
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
	return rc;
}
