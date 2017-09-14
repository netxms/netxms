/* 
** nxapush - command line tool used to push DCI values to NetXMS server
**           via local NetXMS agent
** Copyright (C) 2006-2016 Raden Solutions
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
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nxproc.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

/**
 * Pipe handle
 */
static NamedPipe *s_pipe = NULL;

/**
 * Data to send
 */
static StringMap *s_data = new StringMap;

/**
 * options
 */
static int s_optVerbose = 1;
static UINT32 s_optObjectId = 0;
static time_t s_timestamp = 0;

/**
 * Values parser - clean string and split by '='
 */
static BOOL AddValue(TCHAR *pair)
{
	BOOL ret = FALSE;
	TCHAR *p = pair;
	TCHAR *value = NULL;

	for (p = pair; *p != 0; p++)
	{
		if (*p == _T('=') && value == NULL)
		{
			value = p;
		}
		if (*p == 0x0D || *p == 0x0A)
		{
			*p = 0;
			break;
		}
	}

	if (value != NULL)
	{
		*value++ = 0;
		s_data->set(pair, value);
		ret = TRUE;
	}

	return ret;
}

/**
 * Initialize and connect to the agent
 */
static BOOL Startup()
{
   s_pipe = NamedPipe::connect(_T("nxagentd.push"));
   if (s_pipe == NULL)
   {
      if (s_optVerbose > 1)
         _tprintf(_T("ERROR: named pipe connection failed\n"));
      return FALSE;
   }

	if (s_optVerbose > 2)
		_tprintf(_T("Connected to NetXMS agent\n"));

	return TRUE;
}

/**
 * Send all DCIs
 */
static bool Send()
{
	NXCPMessage msg;
	msg.setCode(CMD_PUSH_DCI_DATA);
   msg.setField(VID_OBJECT_ID, s_optObjectId);
   msg.setFieldFromTime(VID_TIMESTAMP, s_timestamp);
   s_data->fillMessage(&msg, VID_NUM_ITEMS, VID_PUSH_DCI_DATA_BASE);

	// Send message to pipe
	NXCP_MESSAGE *rawMsg = msg.serialize();
	bool success = s_pipe->write(rawMsg, ntohl(rawMsg->size));

	free(rawMsg);
	return success;
}

/**
 * Disconnect and cleanup
 */
static BOOL Teardown()
{
	delete s_pipe;
	delete s_data;
	if (s_optVerbose > 2)
		_tprintf(_T("Disconnected from NetXMS agent\n"));
	return TRUE;
}

/**
 * Command line options
 */
#if HAVE_DECL_GETOPT_LONG
static struct option longOptions[] =
{
	{ (char *)"help",           no_argument,       NULL,        'h'},
	{ (char *)"object",         required_argument, NULL,        'o'},
	{ (char *)"quiet",          no_argument,       NULL,        'q'},
	{ (char *)"timestamp-unix", required_argument, NULL,        't'},
	{ (char *)"timestamp-text", required_argument, NULL,        'T'},
	{ (char *)"verbose",        no_argument,       NULL,        'v'},
	{ (char *)"version",        no_argument,       NULL,        'V'},
	{ NULL, 0, NULL, 0}
};
#endif

#define SHORT_OPTIONS "ho:qt:T:vV"

/**
 * Show online help
 */
static void usage(char *argv0)
{
	_tprintf(
_T("NetXMS Agent PUSH  Version ") NETXMS_VERSION_STRING _T("\n")
_T("Copyright (c) 2006-2013 Raden Solutions\n\n")
_T("Usage: %hs [OPTIONS] [@batch_file] [values]\n")
_T("  \n")
_T("Options:\n")
#if HAVE_GETOPT_LONG
_T("  -h, --help                  Display this help message.\n")
_T("  -o, --object <id>           Push data on behalf of object with given id.\n")
_T("  -q, --quiet                 Suppress all messages.\n")
_T("  -t, --timestamp-unix <time> Specify timestamp for data as UNIX timestamp.\n")
_T("  -T, --timestamp-text <time> Specify timestamp for data as YYYYMMDDhhmmss.\n")
_T("  -v, --verbose               Enable verbose messages. Add twice for debug\n")
_T("  -V, --version               Display version information.\n\n")
#else
_T("  -h             Display this help message.\n")
_T("  -o <id>        Push data on behalf of object with given id.\n")
_T("  -q             Suppress all messages.\n")
_T("  -t <time>      Specify timestamp for data as UNIX timestamp.\n")
_T("  -T <time>      Specify timestamp for data as YYYYMMDDhhmmss.\n")
_T("  -v             Enable verbose messages. Add twice for debug\n")
_T("  -V             Display version information.\n\n")
#endif
_T("Notes:\n")
_T("  * Values should be given in the following format:\n")
_T("    dci=value\n")
_T("    where dci can be specified by it's name\n")
_T("  * Name of batch file cannot contain character = (equality sign)\n")
_T("\n")
_T("Examples:\n")
_T("  Push two values:\n")
_T("      nxapush PushParam1=1 PushParam2=4\n\n")
_T("  Push values from file:\n")
_T("      nxapush @file\n")
	, argv0);
}

/**
 * Debug writer
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *text)
{
   if (tag == NULL)      
      _tprintf(_T("%s\n"), text);
   else
      _tprintf(_T("<%s> %s\n"), tag, text);
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
	int ret = 0;
	int c;

	InitNetXMSProcess(true);
   nxlog_set_debug_writer(DebugWriter);

	opterr = 0;
#if HAVE_DECL_GETOPT_LONG
	while ((c = getopt_long(argc, argv, SHORT_OPTIONS, longOptions, NULL)) != -1)
#else
	while ((c = getopt(argc, argv, SHORT_OPTIONS)) != -1)
#endif
	{
		switch(c)
		{
		   case 'h': // help
			   usage(argv[0]);
			   exit(0);
			   break;
		   case 'o': // object ID
			   s_optObjectId = strtoul(optarg, NULL, 0);
			   break;
		   case 'q': // quiet
			   s_optVerbose = 0;
			   break;
		   case 't': // timestamp as UNIX time
			   s_timestamp = (time_t)strtoull(optarg, NULL, 0);
			   break;
		   case 'T': // timestamp as YYYYMMDDhhmmss
			   s_timestamp = ParseDateTimeA(optarg, 0);
			   break;
		   case 'v': // verbose
			   s_optVerbose++;
			   break;
		   case 'V': // version
			   _tprintf(_T("nxapush (") NETXMS_VERSION_STRING _T(")\n"));
			   exit(0);
			   break;
		   case '?':
			   exit(3);
			   break;
		}
	}
	
	if (optind == argc)
	{
		if (s_optVerbose > 0)
		{
			_tprintf(_T("Not enough arguments\n\n"));
#if HAVE_GETOPT_LONG
			_tprintf(_T("Try `%hs --help' for more information.\n"), argv[0]);
#else
			_tprintf(_T("Try `%hs -h' for more information.\n"), argv[0]);
#endif
		}
		exit(1);
	}

   nxlog_set_debug_level(s_optVerbose);

	// Parse
	if (optind < argc)
	{
		while (optind < argc)
		{
			char *p = argv[optind];

			if (((*p == '@') && (strchr(p, '=') == NULL)) || (*p == '-'))
			{
				FILE *fileHandle = stdin;

				if (*p != '-')
				{
					fileHandle = fopen(p + 1, "r");
				}

				if (fileHandle != NULL)
				{
					char buffer[1024];

					while (fgets(buffer, sizeof(buffer), fileHandle) != NULL)
					{
#ifdef UNICODE
						WCHAR *wvalue = WideStringFromMBString(buffer);
						AddValue(wvalue);
						free(wvalue);
#else
						AddValue(buffer);
#endif
					}

					if (fileHandle != stdin)
					{
						fclose(fileHandle);
					}
				}
				else
				{
					if (s_optVerbose > 0)
					{
						_tprintf(_T("Cannot open \"%hs\": %hs\n"), p + 1, strerror(errno));
					}
				}
			}
			else
			{
#ifdef UNICODE
				WCHAR *wvalue = WideStringFromMBString(argv[optind]);
				AddValue(wvalue);
				free(wvalue);
#else
				AddValue(argv[optind]);
#endif
			}

			optind++;
		}
	}

	if (s_data->size() > 0)
	{
		if (s_optVerbose > 1)
			_tprintf(_T("%d data pair%s to send\n"), s_data->size(), (s_data->size() == 1) ? _T("") : _T("s"));
		if (Startup())
		{
			if (Send() != TRUE)
			{
				ret = 3;
			}
		}
	}
	else
	{
		if (s_optVerbose > 0)
		{
			_tprintf(_T("No valid pairs found; nothing to send\n"));
			ret = 2;
		}
	}
	Teardown();

	return ret;
}
