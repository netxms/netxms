/* 
** nxapush - command line tool used to push DCI values to NetXMS server
**           via local NetXMS agent
** Copyright (C) 2006-2025 Raden Solutions
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
#include <netxms_getopt.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxapush)

/**
 * Pipe handle
 */
static NamedPipe *s_pipe = nullptr;

/**
 * Data to send
 */
static StringMap *s_data = new StringMap;

/**
 * options
 */
static int s_optVerbose = 1;
static uint32_t s_optObjectId = 0;
static Timestamp s_timestamp = Timestamp::fromMilliseconds(0);
static bool s_localCache = false;
static bool s_statsiteFormat = false;

/**
 * Values parser (statsite format)
 */
static bool AddValueStatSite(TCHAR *input)
{
   bool ret = false;
   TCHAR *p = input;
   TCHAR *value = nullptr;
   TCHAR *timestamp = nullptr;

   for(; *p != 0; p++)
   {
      if (*p == _T('|'))
      {
         if (value == nullptr)
         {
            value = p;
         }
         else if (timestamp == nullptr)
         {
            timestamp = p;
         }
      }
      if (*p == 0x0D || *p == 0x0A)
      {
         *p = 0;
         break;
      }
   }

   if (timestamp != nullptr)
   {
      *timestamp++ = 0;
   }
   if (value != nullptr)
   {
      *value++ = 0;
      s_data->set(input, value);
      ret = true;
   }

   return ret;
}

/**
 * Values parser - clean string and split by '='
 */
static bool AddValueNative(TCHAR *pair)
{
	bool ret = false;
	TCHAR *p = pair;
	TCHAR *value = nullptr;

	for(; *p != 0; p++)
	{
		if (*p == _T('=') && value == nullptr)
		{
			value = p;
		}
		if (*p == 0x0D || *p == 0x0A)
		{
			*p = 0;
			break;
		}
	}

	if (value != nullptr)
	{
		*value++ = 0;
		s_data->set(pair, value);
		ret = true;
	}

	return ret;
}

/**
 * Add value
 */
inline bool AddValue(TCHAR *input)
{
   return s_statsiteFormat ? AddValueStatSite(input) : AddValueNative(input);
}

/**
 * Initialize and connect to the agent
 */
static bool Startup()
{
   s_pipe = NamedPipe::connect(_T("nxagentd.push"));
   if (s_pipe == nullptr)
   {
      if (s_optVerbose > 1)
         _tprintf(_T("ERROR: named pipe connection failed\n"));
      return false;
   }

	if (s_optVerbose > 2)
		_tprintf(_T("Connected to NetXMS agent\n"));

	return true;
}

/**
 * Send all DCIs
 */
static bool Send()
{
	NXCPMessage msg(CMD_PUSH_DCI_DATA, 0);
   msg.setField(VID_OBJECT_ID, s_optObjectId);
   msg.setField(VID_LOCAL_CACHE, s_localCache);
   msg.setField(VID_TIMESTAMP_MS, s_timestamp);
   s_data->fillMessage(&msg, VID_PUSH_DCI_DATA_BASE, VID_NUM_ITEMS);

	// Send message to pipe
	NXCP_MESSAGE *rawMsg = msg.serialize();
	bool success = s_pipe->write(rawMsg, ntohl(rawMsg->size));

	MemFree(rawMsg);
	return success;
}

/**
 * Disconnect and cleanup
 */
static bool Teardown()
{
	delete s_pipe;
	delete s_data;
	if (s_optVerbose > 2)
		_tprintf(_T("Disconnected from NetXMS agent\n"));
	return true;
}

/**
 * Command line options
 */
#if HAVE_GETOPT_LONG
static struct option longOptions[] =
{
	{ (char *)"help",           no_argument,       nullptr,        'h'},
   { (char *)"local-cache",    no_argument,       nullptr,        'l'},
	{ (char *)"object",         required_argument, nullptr,        'o'},
	{ (char *)"quiet",          no_argument,       nullptr,        'q'},
   { (char *)"statsite",       no_argument,       nullptr,        's'},
	{ (char *)"timestamp-unix", required_argument, nullptr,        't'},
	{ (char *)"timestamp-text", required_argument, nullptr,        'T'},
	{ (char *)"verbose",        no_argument,       nullptr,        'v'},
	{ (char *)"version",        no_argument,       nullptr,        'V'},
	{ nullptr, 0, nullptr, 0}
};
#endif

#define SHORT_OPTIONS "hlo:qst:T:vV"

/**
 * Show online help
 */
static void usage(char *argv0)
{
	_tprintf(
_T("NetXMS Agent PUSH  Version ") NETXMS_VERSION_STRING _T("\n")
_T("Copyright (c) 2006-2025 Raden Solutions\n\n")
_T("Usage: %hs [OPTIONS] [@batch_file] [values]\n")
_T("       %hs [OPTIONS] -\n")
_T("  \n")
_T("Options:\n")
#if HAVE_GETOPT_LONG
_T("  -h, --help                  Display this help message.\n")
_T("  -l, --local-cache           Push to agent's local cache.\n")
_T("  -o, --object <id>           Push data on behalf of object with given id.\n")
_T("  -q, --quiet                 Suppress all messages.\n")
_T("  -s, --statsite              Use statsite sink format.\n")
_T("  -t, --timestamp-unix <time> Specify timestamp for data as UNIX timestamp.\n")
_T("  -T, --timestamp-text <time> Specify timestamp for data as YYYYMMDDhhmmss.\n")
_T("  -v, --verbose               Enable verbose messages. Add twice for debug\n")
_T("  -V, --version               Display version information.\n\n")
#else
_T("  -h             Display this help message.\n")
_T("  -l             Push to agent's local cache.\n")
_T("  -o <id>        Push data on behalf of object with given id.\n")
_T("  -q             Suppress all messages.\n")
_T("  -s             Use statsite sink format.\n")
_T("  -t <time>      Specify timestamp for data as UNIX timestamp.\n")
_T("  -T <time>      Specify timestamp for data as YYYYMMDDhhmmss.\n")
_T("  -v             Enable verbose messages. Add twice for debug\n")
_T("  -V             Display version information.\n\n")
#endif
_T("Notes:\n")
_T("  * Values should be given in format\n")
_T("    dci=value\n")
_T("    or (if statsite sink format is selected):\n")
_T("    dci|value|timestamp\n")
_T("    where dci can be specified by it's name\n")
_T("  * Name of batch file cannot contain character = (equality sign)\n")
_T("  * Use - character in place of values to read from standard input\n")
_T("\n")
_T("Examples:\n")
_T("  Push two values:\n")
_T("      nxapush PushParam1=1 PushParam2=4\n\n")
_T("  Push values from file:\n")
_T("      nxapush @file\n")
	, argv0, argv0);
}

/**
 * Debug writer
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
   if (tag != nullptr)
      _tprintf(_T("<%s> "), tag);
   _vtprintf(format, args);
   _fputtc(_T('\n'), stdout);
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
	InitNetXMSProcess(true);
   nxlog_set_debug_writer(DebugWriter);

	opterr = 0;
	int c;
#if HAVE_GETOPT_LONG
	while ((c = getopt_long(argc, argv, SHORT_OPTIONS, longOptions, nullptr)) != -1)
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
         case 'l': // local cache
            s_localCache = true;
            break;
		   case 'o': // object ID
			   s_optObjectId = strtoul(optarg, nullptr, 0);
			   break;
		   case 'q': // quiet
			   s_optVerbose = 0;
			   break;
         case 's': // statsite sink format
            s_statsiteFormat = true;
            break;
		   case 't': // timestamp as UNIX time
			   s_timestamp = Timestamp::fromTime((time_t)strtoull(optarg, nullptr, 0));
			   break;
		   case 'T': // timestamp as YYYYMMDDhhmmss
			   s_timestamp = Timestamp::fromTime(ParseDateTimeA(optarg, 0));
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
		return 1;
	}

   nxlog_set_debug_level(s_optVerbose);

	// Parse
   while (optind < argc)
   {
      char *p = argv[optind];

      if (((*p == '@') && (strchr(p, '=') == nullptr)) || (*p == '-'))
      {
         FILE *fileHandle = stdin;

         if (*p != '-')
         {
            fileHandle = fopen(p + 1, "r");
         }

         if (fileHandle != nullptr)
         {
            char buffer[1024];

            while(fgets(buffer, sizeof(buffer), fileHandle) != nullptr)
            {
#ifdef UNICODE
               WCHAR *wvalue = WideStringFromMBStringSysLocale(buffer);
               AddValue(wvalue);
               MemFree(wvalue);
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
         WCHAR *wvalue = WideStringFromMBStringSysLocale(argv[optind]);
         AddValue(wvalue);
         MemFree(wvalue);
#else
         AddValue(argv[optind]);
#endif
      }

      optind++;
   }

   int ret = 0;
	if (s_data->size() > 0)
	{
		if (s_optVerbose > 1)
			_tprintf(_T("%d data pair%s to send\n"), s_data->size(), (s_data->size() == 1) ? _T("") : _T("s"));
		if (Startup())
		{
			if (!Send())
			{
				ret = 3;
			}
		}
		else
		{
		   ret = 2;
		}
	}
	else
	{
		if (s_optVerbose > 0)
		{
			_tprintf(_T("No valid pairs found; nothing to send\n"));
			ret = 1;
		}
	}
	Teardown();

	return ret;
}
