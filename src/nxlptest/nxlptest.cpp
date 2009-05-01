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

static char m_helpText[] =
   "NetXMS Log Parsing Tester  Version " NETXMS_VERSION_STRING "\n"
   "Copyright (c) 2009 Victor Kirhenshtein\n\n"
   "Usage:\n"
   "   nxlptest [options] parser\n\n"
   "Where valid options are:\n"
	"   -f file    : Input file (overrides parser settings)\n"
   "   -h         : Show this help\n"
	"   -i         : Uses standard input instead of file defined in parser\n" 
	"   -t level   : Set trace level (overrides parser settings)\n"
   "   -v         : Show version and exit\n"
   "\n";


//
// Trace callback
//

static void TraceCallback(const TCHAR *format, va_list args)
{
	vprintf(format, args);
	putc('\n', stdout);
}


//
// Logger callback
//

static void LoggerCallback(int level, const TCHAR *format, ...)
{
	va_list args;

	va_start(args, format);
	//vprintf(format, args);
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
	DWORD size;
	char *inputFile = NULL;
	LogParser *parser;

   // Parse command line
   opterr = 1;
	while((ch = getopt(argc, argv, "f:hit:v")) != -1)
   {
		switch(ch)
		{
			case 'h':
				printf(m_helpText);
            return 0;
         case 'v':
				printf("NetXMS Log Parsing Tester  Version " NETXMS_VERSION_STRING "\n"
				       "Copyright (c) 2009 Victor Kirhenshtein\n\n");
            return 0;
			case 'f':
				inputFile = optarg;
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
      printf("Required arguments missing\n");
      return 1;
   }

	xml = LoadFile(argv[optind], &size);
	if (xml != NULL)
	{
		char errorText[1024];
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
			printf("Parser started. Press ESC to stop.\nFile: %s\nTrace level: %d\n\n",
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
			printf("ERROR: invalid parser definition file (%s)\n", errorText);
		}
		delete parser;
		free(xml);
	}
	else
	{
		printf("ERROR: unable to load parser definition file (%s)\n", strerror(errno));
		rc = 2;
	}

	return rc;
}
