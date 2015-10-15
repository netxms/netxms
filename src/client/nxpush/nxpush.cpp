/* 
** nxpush - command line tool used to push DCI values to NetXMS server
** Copyright (C) 2006-2015 Alex Kirhenshtein
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
#include <nxclient.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

/**
 * Session handle
 */
static NXCSession *s_session = NULL;

/**
 * Data to send
 */
static ObjectArray<NXCPushData> s_data(16, 16, true);

/**
 * options
 */
static int s_optVerbose = 1;
static const char *s_optHost = NULL;
static const char *s_optUser = "guest";
static const char *s_optPassword = "";
static bool s_optEncrypt = false;
static int s_optBatchSize = 0;
static time_t s_timestamp = 0;

/**
 * Split IDs, cleanup and add to the send s_data
 */
static BOOL AddValuePair(char *name, char *value)
{
	BOOL ret = TRUE;
	UINT32 dciId = 0;
	UINT32 nodeId = 0;
	char *dciName = NULL;
	char *nodeName = NULL;

	nodeName = name;
	dciName = strchr(name, ':');
	if (dciName != NULL)
	{
		*dciName++ = 0;

		nodeId = strtoul(nodeName, NULL, 10);
		dciId = strtoul(dciName, NULL, 10);
	}
	else
	{
		ret = FALSE;
	}

	if (ret == TRUE)
	{
		if (s_optVerbose > 2)
		{
			_tprintf(_T("AddValuePair: dciID=\"%d\", nodeName=\"%hs\", dciName=\"%hs\", value=\"%hs\"\n"),
				dciId, nodeName, dciName, value);
		}

      NXCPushData *p = new NXCPushData();
		if (p != NULL)
		{
			p->dciId = dciId;
			p->nodeId = nodeId;
			if (dciId > 0)
			{
				p->dciName = NULL;
			}
			else
			{
#ifdef UNICODE
				p->dciName = WideStringFromMBString(dciName);
#else
				p->dciName = strdup(dciName);
#endif
			}

			if (nodeId > 0)
			{
				p->nodeName = NULL;
			}
			else
			{
#ifdef UNICODE
				p->nodeName= WideStringFromMBString(nodeName);
#else
				p->nodeName = strdup(nodeName);
#endif
			}

#ifdef UNICODE
			p->value = WideStringFromMBString(value);
#else
			p->value = strdup(value);
#endif
         s_data.add(p);
		}
		else
		{
			if (s_optVerbose > 0)
			{
				_tprintf(_T("new failed!: %s\n"), _tcserror(errno));
			}
		}
	}

	return ret;
}

/**
 * Values parser - clean string and split by '='
 */
static BOOL AddValue(char *pair)
{
	BOOL ret = FALSE;
	char *p = pair;
	char *value = NULL;

   if (s_optVerbose > 1)
      _tprintf(_T("AddValue(): %hs\n"), pair);

	for (p = pair; *p != 0; p++)
	{
		if (*p == '=' && value == NULL)
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

		AddValuePair(pair, value);

		ret = TRUE;
	}

	return ret;
}

/**
 * Callback function for debug messages from client library
 */
static void DebugCallback(const TCHAR *pMsg)
{
	_tprintf(_T("NXCL: %s\n"), pMsg);
}

/**
 * Initialize client library and connect to the server
 */
static BOOL Startup()
{
	BOOL ret = FALSE;
	UINT32 dwResult;

#ifdef _WIN32
	WSADATA wsaData;

	if (WSAStartup(2, &wsaData) != 0)
	{
		if (s_optVerbose > 0)
		{
			_tprintf(_T("Unable to initialize Windows sockets\n"));
		}
		return FALSE;
	}
#endif

	if (!NXCInitialize())
	{
		if (s_optVerbose > 0)
		{
			_tprintf(_T("Failed to initialize NetXMS client library\n"));
		}
	}
	else
	{
		if (s_optVerbose > 2)
		{
			NXCSetDebugCallback(DebugCallback);

			_tprintf(_T("Connecting to \"%hs\" as \"%hs\"; encryption is %s\n"), s_optHost, s_optUser,
				s_optEncrypt ? _T("enabled") : _T("disabled"));
		}

#ifdef UNICODE
		WCHAR *wHost = WideStringFromMBString(s_optHost);
		WCHAR *wUser = WideStringFromMBString(s_optUser);
		WCHAR *wPassword = WideStringFromMBString(s_optPassword);
#define _HOST wHost
#define _USER wUser
#define _PASSWD wPassword
#else
#define _HOST s_optHost
#define _USER s_optUser
#define _PASSWD s_optPassword
#endif

      s_session = new NXCSession();
      static UINT32 protocolVersions[] = { CPV_INDEX_PUSH };
      dwResult = s_session->connect(_HOST, _USER, _PASSWD, s_optEncrypt ? NXCF_ENCRYPT : 0, _T("nxpush/") NETXMS_VERSION_STRING,
                                  protocolVersions, sizeof(protocolVersions) / sizeof(UINT32));

#ifdef UNICODE
		free(wHost);
		free(wUser);
		free(wPassword);
#endif
		if (dwResult != RCC_SUCCESS)
		{
			if (s_optVerbose > 0)
			{
				_tprintf(_T("Unable to connect to the server: %s\n"), NXCGetErrorText(dwResult));
			}
		}
		else
		{
         s_session->setCommandTimeout(5 * 1000);
			ret = TRUE;
		}
      NXCShutdown();
	}

	return ret;
}

/**
 * Send all DCIs
 */
static BOOL Send()
{
	BOOL ret = TRUE;
	UINT32 errIdx;
	
	int i, size;
	int batches = 1;

	if (s_optBatchSize == 0)
	{
		s_optBatchSize = s_data.size();
	}

	batches = s_data.size() / s_optBatchSize;
	if (s_data.size() % s_optBatchSize != 0)
	{
		batches++;
	}

	for (i = 0; i < batches; i++)
	{
		size = min(s_optBatchSize, s_data.size() - (s_optBatchSize * i));

		if (s_optVerbose > 1)
		{
			_tprintf(_T("Sending batch #%d with %d records\n"), i + 1, size);
		}

		if (s_optVerbose > 2)
		{
			for (int j = 0; j < size; j++)
			{
            NXCPushData *rec = s_data.get(s_optBatchSize * i + j);
				_tprintf(_T("Record #%d: \"%s\" for %d(%s):%d(%s)\n"),
					(s_optBatchSize * i) + j + 1,
					rec->value,
					rec->nodeId,
					rec->nodeName != NULL ? rec->nodeName : _T("n/a"),
					rec->dciId,
					rec->dciName != NULL ? rec->dciName : _T("n/a"));
			}
		}

      UINT32 dwResult = ((DataCollectionController *)(s_session->getController(CONTROLLER_DATA_COLLECTION)))->pushData(&s_data, s_timestamp, &errIdx);
		if (dwResult != RCC_SUCCESS)
		{
			if (s_optVerbose > 0)
			{
				_tprintf(_T("Push failed at record #%d (#%d in batch): %s.\n"),
					(i * s_optBatchSize) + errIdx + 1,
					errIdx + 1,
					NXCGetErrorText(dwResult));
			}

			ret = FALSE;
			break;
		}
		else
		{
			if (s_optVerbose > 1)
			{
				_tprintf(_T("Done.\n"));
			}
		}
	}

	return ret;
}

/**
 * Disconnect and cleanup
 */
static BOOL Teardown()
{
	if (s_session != NULL)
	{
      delete s_session;
	}

	for(int i = 0; i < s_data.size(); i++)
	{
      NXCPushData *d = s_data.get(i);
		safe_free(d->dciName);
		safe_free(d->nodeName);
		safe_free(d->value);
	}

	return TRUE;
}

/**
 * Command line options
 */
#if HAVE_DECL_GETOPT_LONG
static struct option longOptions[] =
{
	{ (char *)"batchsize",      required_argument, NULL,        'b' },
#ifndef _WIN32
	{ (char *)"codepage",       required_argument, NULL,        'c' },
#endif
	{ (char *)"encrypt",        no_argument,       NULL,        'e' },
	{ (char *)"help",           no_argument,       NULL,        'h' },
	{ (char *)"host",           required_argument, NULL,        'H' },
	{ (char *)"password",       required_argument, NULL,        'P' },
	{ (char *)"quiet",          no_argument,       NULL,        'q' },
	{ (char *)"timestamp-unix", required_argument, NULL,        't' },
	{ (char *)"timestamp-text", required_argument, NULL,        'T' },
	{ (char *)"user",           required_argument, NULL,        'u' },
	{ (char *)"verbose",        no_argument,       NULL,        'v' },
	{ (char *)"version",        no_argument,       NULL,        'V' },
	{ NULL, 0, NULL, 0 }
};
#endif

#ifdef _WIN32
#define SHORT_OPTIONS "b:ehH:P:qt:T:u:vV"
#else
#define SHORT_OPTIONS "b:c:ehH:P:qt:T:u:vV"
#endif

/**
 * Print usage info
 */
static void usage(char *argv0)
{
	_tprintf(
      _T("NetXMS PUSH  Version ") NETXMS_VERSION_STRING _T("\n")
      _T("Copyright (c) 2006-2015 Raden Solutions\n\n")
      _T("Usage: %hs [OPTIONS] [server] [@batch_file] [values]\n")
      _T("  \n")
      _T("Options:\n")
#if HAVE_GETOPT_LONG
      _T("  -b, --batchsize <size>      Batch size (default is to send all data in one batch)\n")
#ifndef _WIN32
      _T("  -c, --codepage <page>       Codepage (default is %hs)\n")
#endif
      _T("  -e, --encrypt               Encrypt session.\n")
      _T("  -h, --help                  Display this help message.\n")
      _T("  -H, --host <host>           Server address.\n")
      _T("  -P, --password <password>   Specify user's password. Default is empty.\n")
      _T("  -q, --quiet                 Suppress all messages.\n")
      _T("  -t, --timestamp-unix <time> Specify timestamp for data as UNIX timestamp.\n")
      _T("  -T, --timestamp-text <time> Specify timestamp for data as YYYYMMDDhhmmss.\n")
      _T("  -u, --user <user>           Login to server as user. Default is \"guest\".\n")
      _T("  -v, --verbose               Enable verbose messages. Add twice for debug\n")
      _T("  -V, --version               Display version information.\n\n")
#else
      _T("  -b <size>      Batch size (default is to send all data in one batch)\n")
      _T("  -c <page>      Codepage (default is %hs)\n")
      _T("  -e             Encrypt session.\n")
      _T("  -h             Display this help message.\n")
      _T("  -H <host>      Server address.\n")
      _T("  -P <password>  Specify user's password. Default is empty.\n")
      _T("  -q             Suppress all messages.\n")
      _T("  -t <time>      Specify timestamp for data as UNIX timestamp.\n")
      _T("  -T <time>      Specify timestamp for data as YYYYMMDDhhmmss.\n")
      _T("  -u <user>      Login to server as user. Default is \"guest\".\n")
      _T("  -v             Enable verbose messages. Add twice for debug\n")
      _T("  -V             Display version information.\n\n")
#endif
      _T("Notes:\n")
      _T("  * Values should be given in the following format:\n")
      _T("    node:dci=value\n")
      _T("    where node and dci can be specified either by ID, object name, DNS name,\n")
      _T("    or IP address. If you wish to specify node by DNS name or IP address,\n")
      _T("    you should prefix it with @ character\n")
      _T("  * First parameter will be used as \"host\" if -H/--host is unset\n")
      _T("  * Name of batch file cannot contain character = (equality sign)\n")
      _T("\n")
      _T("Examples:\n")
      _T("  Push two values to server 10.0.0.1 as user \"sender\" with password \"passwd\":\n")
      _T("      nxpush -H 10.0.0.1 -u sender -P passwd 10:24=1 10:PushParam=4\n\n")
      _T("  Push values from file to server 10.0.0.1 as user \"guest\" without password:\n")
      _T("      nxpush 10.0.0.1 @file\n"), argv0
#ifndef _WIN32
      , ICONV_DEFAULT_CODEPAGE
#endif
      );
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
	int ret = 0;
	int c;

	InitThreadLibrary();

	BOOL bStart = TRUE;
	opterr = 0;
#if HAVE_DECL_GETOPT_LONG
	while ((c = getopt_long(argc, argv, SHORT_OPTIONS, longOptions, NULL)) != -1)
#else
	while ((c = getopt(argc, argv, SHORT_OPTIONS)) != -1)
#endif
	{
		switch(c)
		{
		   case 'b': // batch size
			   s_optBatchSize = atoi(optarg); // 0 == unlimited
			   break;
#ifndef _WIN32
         case 'c':
            SetDefaultCodepage(optarg);
            break;
#endif
		   case 'e': // user
			   s_optEncrypt = true;
			   break;
		   case 'h': // help
			   usage(argv[0]);
			   exit(1);
			   break;
		   case 'H': // host
			   s_optHost = optarg;
			   break;
		   case 'P': // password
			   s_optPassword = optarg;
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
		   case 'u': // user
			   s_optUser = optarg;
			   break;
		   case 'v': // verbose
			   s_optVerbose++;
			   break;
		   case 'V': // version
			   _tprintf(_T("nxpush (") NETXMS_VERSION_STRING _T(")\n"));
			   exit(0);
			   break;
		   case '?':
			   exit(3);
			   break;
		}
	}
	
	if (s_optHost == NULL && optind < argc)
	{
		s_optHost = argv[optind++];
	}

	if (optind == argc)
	{
		if (s_optVerbose > 0)
		{
			_tprintf(_T("Not enough arguments\n\n"));
#if HAVE_GETOPT_LONG
			_tprintf(_T("Try nxpush --help for more information.\n"));
#else
			_tprintf(_T("Try nxpush -h for more information.\n"));
#endif
		}
		exit(1);
	}

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
						AddValue(buffer);
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
						_tprintf(_T("Cannot open \"%hs\": %s\n"), p + 1, _tcserror(errno));
					}
				}
			}
			else
			{
				AddValue(argv[optind]);
			}

			optind++;
		}
	}

	if (s_data.size() > 0)
	{
		if (s_optVerbose > 0)
         _tprintf(_T("%d values to send\n"), s_data.size());
		if (Startup())
		{
		   if (s_optVerbose > 0)
            _tprintf(_T("Connected\n"));
			if (Send())
         {
		      if (s_optVerbose > 0)
               _tprintf(_T("Data sent\n"));
         }
         else
			{
		      if (s_optVerbose > 0)
               _tprintf(_T("Data sending failed\n"));
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
