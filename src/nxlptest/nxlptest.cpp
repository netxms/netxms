/* 
** NetXMS - Network Management System
** NetXMS Log Parser Testing Utility
** Copyright (C) 2009 Victor Kirhenshtein
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


//
// Static data
//

static CONDITION m_stopCondition = INVALID_CONDITION_HANDLE;


//
// Help text
//

static TCHAR m_helpText[] =
   _T("NetXMS Log Parsing Tester  Version ") NETXMS_VERSION_STRING _T("\n")
   _T("Copyright (c) 2009-2012 Victor Kirhenshtein\n\n")
   _T("Usage:\n")
   _T("   nxlptest [options] parser\n\n")
   _T("Where valid options are:\n")
	_T("   -f file    : Input file (overrides parser settings)\n")
   _T("   -h         : Show this help\n")
	_T("   -i         : Uses standard input instead of file defined in parser\n" )
	_T("   -t level   : Set trace level (overrides parser settings)\n")
   _T("   -v         : Show version and exit\n")
   _T("\n");


//
// Trace callback
//

static void TraceCallback(const TCHAR *format, va_list args)
{
	_vtprintf(format, args);
	_puttc(_T('\n'), stdout);
}


//
// Logger callback
//

static void LoggerCallback(int level, const TCHAR *format, ...)
{
	va_list args;

	va_start(args, format);
	TraceCallback(format, args);
	va_end(args);
}


//
// File parsing thread
//

static THREAD_RESULT THREAD_CALL ParserThread(void *arg)
{
	((LogParser *)arg)->monitorFile(m_stopCondition, LoggerCallback);
	return THREAD_OK;
}


//
// main()
//

int main(int argc, char *argv[])
{
	int rc = 0, ch, traceLevel = -1;
	BYTE *xml;
	UINT32 size;
	TCHAR *inputFile = NULL;
	LogParser *parser;

   // Parse command line
   opterr = 1;
	while((ch = getopt(argc, argv, "f:hit:v")) != -1)
   {
		switch(ch)
		{
			case 'h':
				_tprintf(m_helpText);
            return 0;
         case 'v':
				_tprintf(_T("NetXMS Log Parsing Tester  Version ") NETXMS_VERSION_STRING _T("\n")
				         _T("Copyright (c) 2009-2012 Victor Kirhenshtein\n\n"));
            return 0;
			case 'f':
#ifdef UNICODE
				inputFile = WideStringFromMBString(optarg);
#else
				inputFile = optarg;
#endif
				break;
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

#ifdef UNICODE
	WCHAR *wname = WideStringFromMBString(argv[optind]);
	xml = LoadFile(wname, &size);
	free(wname);
#else
	xml = LoadFile(argv[optind], &size);
#endif
	if (xml != NULL)
	{
		TCHAR errorText[1024];
		THREAD thread;

		parser = new LogParser;
		if (parser->createFromXml((const char *)xml, size, errorText, 1024))
		{
			parser->setTraceCallback(TraceCallback);
			if (traceLevel != -1)
				parser->setTraceLevel(traceLevel);
			if (inputFile != NULL)
				parser->setFileName(inputFile);

			m_stopCondition = ConditionCreate(TRUE);
			thread = ThreadCreateEx(ParserThread, 0, parser);
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
#endif
			ConditionSet(m_stopCondition);
			ThreadJoin(thread);
			ConditionDestroy(m_stopCondition);
		}
		else
		{
			_tprintf(_T("ERROR: invalid parser definition file (%s)\n"), errorText);
		}
		delete parser;
		free(xml);
	}
	else
	{
		_tprintf(_T("ERROR: unable to load parser definition file (%s)\n"), _tcserror(errno));
		rc = 2;
	}

	return rc;
}
