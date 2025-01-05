/* 
** nxaevent - command line tool used to send events to NetXMS server
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
#include <netxms_getopt.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxaevent)

/**
 * Pipe handle
 */
static NamedPipe *s_pipe = nullptr;

/**
 * Data
 */
static uint32_t s_eventCode = 0;
static TCHAR *s_eventName = nullptr;
static StringList s_parameters;
static StringList s_parameterNames;

/**
 * options
 */
static uint32_t s_optObjectId = 0;
static int s_optVerbose = 1;
static time_t s_timestamp = time(nullptr);
static bool s_optNamed = false;

/**
 * Initialize and connect to the agent
 */
static bool Startup()
{
   s_pipe = NamedPipe::connect(_T("nxagentd.events"));
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
 * Send event
 */
static bool Send()
{
	NXCPMessage msg(CMD_TRAP, 0, 4); // Use protocol version 4
   msg.setField(VID_EVENT_CODE, s_eventCode);
   if (s_eventName != nullptr)
      msg.setField(VID_EVENT_NAME, s_eventName);
   msg.setField(VID_OBJECT_ID, s_optObjectId);
   msg.setFieldFromTime(VID_TIMESTAMP, s_timestamp);
   msg.setField(VID_NUM_ARGS, static_cast<uint16_t>(s_parameters.size()));
   for(int i = 0; i < s_parameters.size(); i++)
      msg.setField(VID_EVENT_ARG_BASE + i, s_parameters.get(i));
   for(int i = 0; i < s_parameterNames.size(); i++)
      msg.setField(VID_EVENT_ARG_NAMES_BASE + i, s_parameterNames.get(i));

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
	{ (char *)"help",             no_argument,       nullptr,        'h'},
   { (char *)"named-parameters", no_argument,       nullptr,        'n'},
	{ (char *)"object",           required_argument, nullptr,        'o'},
	{ (char *)"quiet",            no_argument,       nullptr,        'q'},
	{ (char *)"timestamp-unix",   required_argument, nullptr,        't'},
	{ (char *)"timestamp-text",   required_argument, nullptr,        'T'},
	{ (char *)"verbose",          no_argument,       nullptr,        'v'},
	{ (char *)"version",          no_argument,       nullptr,        'V'},
	{ nullptr, 0, nullptr, 0}
};
#endif

#define SHORT_OPTIONS "hno:qt:T:vV"

/**
 * Show online help
 */
static void ShowUsage(char *argv0)
{
	_tprintf(
_T("NetXMS Agent Event Sender  Version ") NETXMS_VERSION_STRING _T("\n")
_T("Copyright (c) 2006-2025 Raden Solutions\n\n")
_T("Usage: %hs [OPTIONS] event_code [parameters]\n")
_T("  \n")
_T("Options:\n")
#if HAVE_GETOPT_LONG
_T("  -h, --help                  Display this help message.\n")
_T("  -n, --named-parameters      Parameters are provided in named format: name=value.\n")
_T("  -o, --object <id>           Send event on behalf of object with given id.\n")
_T("  -q, --quiet                 Suppress all messages.\n")
_T("  -t, --timestamp-unix <time> Specify timestamp for event as UNIX timestamp.\n")
_T("  -T, --timestamp-text <time> Specify timestamp for event as YYYYMMDDhhmmss.\n")
_T("  -v, --verbose               Enable verbose messages. Add twice for debug\n")
_T("  -V, --version               Display version information.\n\n")
#else
_T("  -h             Display this help message.\n")
_T("  -n             Parameters are provided in named format: name=value.\n")
_T("  -o <id>        Send event on behalf of object with given id.\n")
_T("  -q             Suppress all messages.\n")
_T("  -t <time>      Specify timestamp for event as UNIX timestamp.\n")
_T("  -T <time>      Specify timestamp for event as YYYYMMDDhhmmss.\n")
_T("  -v             Enable verbose messages. Add twice for debug\n")
_T("  -V             Display version information.\n\n")
#endif
	, argv0);
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
			   ShowUsage(argv[0]);
			   exit(0);
			   break;
         case 'n': // use named paramer format
            s_optNamed = true;
            break;
		   case 'o': // object ID
		      s_optObjectId = strtoul(optarg, nullptr, 0);
			   break;
		   case 'q': // quiet
			   s_optVerbose = 0;
			   break;
		   case 't': // timestamp as UNIX time
			   s_timestamp = (time_t)strtoull(optarg, nullptr, 0);
			   break;
		   case 'T': // timestamp as YYYYMMDDhhmmss
			   s_timestamp = ParseDateTimeA(optarg, 0);
			   break;
		   case 'v': // verbose
			   s_optVerbose++;
			   break;
		   case 'V': // version
			   _tprintf(_T("nxaevent (") NETXMS_VERSION_STRING _T(")\n"));
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

   // Event name or code
   char *eptr;
   uint32_t code = strtoul(argv[optind], &eptr, 0);
   if (*eptr == 0)
   {
      s_eventCode = code;
   }
   else
   {
#ifdef UNICODE
      s_eventName = WideStringFromMBStringSysLocale(argv[optind]);
#else
      s_eventName = MemCopyStringA(argv[optind]);
#endif
   }

   // Event parameters
   for(int i = optind + 1; i < argc; i++)
   {
#ifdef UNICODE
      TCHAR *param = WideStringFromMBStringSysLocale(argv[i]);
#else
      TCHAR *param = MemCopyString(argv[i]);
#endif
      if (s_optNamed)
      {
         TCHAR *separator = _tcschr(param, '=');
         if (separator != nullptr)
         {
            *separator = 0;
            s_parameters.add(separator + 1);
            s_parameterNames.addPreallocated(param);
         }
         else
         {
            TCHAR buffer[32];
            _sntprintf(buffer, 32, _T("parameter%d"), s_parameterNames.size() + 1);
            s_parameterNames.add(buffer);
            s_parameters.addPreallocated(param);
         }
      }
      else
      {
         s_parameters.addPreallocated(param);
      }
   }

   int ret = 0;
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
	Teardown();

	return ret;
}
