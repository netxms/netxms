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


//
// Global variables
//

NXCSession *session = NULL;
DataCollectionController *controller = NULL;
ObjectArray<NXCPushData> queue(16, 16, true);


//
// Forward declarations
//

static BOOL AddValuePair(char *name, char *value);
static BOOL AddValue(char *pair);
static BOOL Startup();
static BOOL Send();
static BOOL Teardown();


//
// options
//

static int optVerbose = 1;
static const char *optHost = NULL;
static const char *optUser = "guest";
static const char *optPassword = "";
static BOOL optEncrypt = FALSE;
static int optBatchSize = 0;

#if HAVE_DECL_GETOPT_LONG
static struct option longOptions[] =
{
#ifndef _WIN32
	{ (char *)"codepage",  required_argument, NULL,        'c' },
#endif
	{ (char *)"version",   no_argument,       NULL,        'V' },
	{ (char *)"help",      no_argument,       NULL,        'h' },
	{ (char *)"verbose",   no_argument,       NULL,        'v' },
	{ (char *)"quiet",     no_argument,       NULL,        'q' },
	{ (char *)"user",      required_argument, NULL,        'u' },
	{ (char *)"password",  required_argument, NULL,        'P' },
	{ (char *)"encrypt",   no_argument,       NULL,        'e' },
	{ (char *)"host",      required_argument, NULL,        'H' },
	{ (char *)"batchsize", required_argument, NULL,        'b' },
	{ NULL, 0, NULL, 0 }
};
#endif

#ifdef _WIN32
#define SHORT_OPTIONS "Vhvqu:P:eH:b:"
#else
#define SHORT_OPTIONS "c:Vhvqu:P:eH:b:"
#endif

/**
 * Print usage info
 */
static void usage(char *argv0)
{
	_tprintf(
      _T("NetXMS PUSH  Version ") NETXMS_VERSION_STRING _T("\n")
      _T("Copyright (c) 2006-2011 Alex Kirhenshtein\n\n")
      _T("Usage: %hs [OPTIONS] [server] [@batch_file] [values]\n")
      _T("  \n")
      _T("Options:\n")
#if HAVE_GETOPT_LONG
#ifndef _WIN32
      _T("  -c, --codepage             Codepage (default is %hs)\n")
#endif
      _T("  -V, --version              Display version information.\n")
      _T("  -h, --help                 Display this help message.\n")
      _T("  -v, --verbose              Enable verbose messages. Add twice for debug\n")
      _T("  -q, --quiet                Suppress all messages.\n\n")
      _T("  -u, --user     <user>      Login to server as user. Default is \"guest\".\n")
      _T("  -P, --password <password>  Specify user's password. Default is empty.\n")
      _T("  -e, --encrypt              Encrypt session.\n")
      _T("  -H, --host     <host>      Server address.\n\n")
#else
      _T("  -c             Codepage (default is %hs)\n")
      _T("  -V             Display version information.\n")
      _T("  -h             Display this help message.\n")
      _T("  -v             Enable verbose messages. Add twice for debug\n")
      _T("  -q             Suppress all messages.\n\n")
      _T("  -u <user>      Login to server as user. Default is \"guest\".\n")
      _T("  -P <password>  Specify user's password. Default is empty.\n")
      _T("  -e             Encrypt session.\n")
      _T("  -H <host>      Server address.\n\n")
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
#ifndef _WIN32
         case 'c':
            SetDefaultCodepage(optarg);
            break;
#endif
		   case 'V': // version
			   _tprintf(_T("nxpush (") NETXMS_VERSION_STRING _T(")\n"));
			   exit(0);
			   break;
		   case 'h': // help
			   usage(argv[0]);
			   exit(1);
			   break;
		   case 'v': // verbose
			   optVerbose++;
			   break;
		   case 'q': // quiet
			   optVerbose = 0;
			   break;
		   case 'u': // user
			   optUser = optarg;
			   break;
		   case 'e': // user
			   optEncrypt = TRUE;
			   break;
		   case 'P': // password
			   optPassword = optarg;
			   break;
		   case 'H': // host
			   optHost = optarg;
			   break;
		   case 'b': // batch size
			   optBatchSize = atoi(optarg); // 0 == unlimited
			   break;
		   case '?':
			   exit(3);
			   break;
		}
	}
	
	if (optHost == NULL && optind < argc)
	{
		optHost = argv[optind++];
	}

	if (optind == argc)
	{
		if (optVerbose > 0)
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
					if (optVerbose > 0)
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

	if (queue.size() > 0)
	{
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
		if (optVerbose > 0)
		{
			_tprintf(_T("No valid pairs found; nothing to send\n"));
			ret = 2;
		}
	}
	Teardown();

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
 * Split IDs, cleanup and add to the send queue
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
		if (optVerbose > 2)
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
         queue.add(p);
		}
		else
		{
			if (optVerbose > 0)
			{
				_tprintf(_T("new failed!: %s\n"), _tcserror(errno));
			}
		}
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
		if (optVerbose > 0)
		{
			_tprintf(_T("Unable to initialize Windows sockets\n"));
		}
		return FALSE;
	}
#endif

	if (!NXCInitialize())
	{
		if (optVerbose > 0)
		{
			_tprintf(_T("Failed to initialize NetXMS client library\n"));
		}
	}
	else
	{
		if (optVerbose > 2)
		{
			NXCSetDebugCallback(DebugCallback);

			_tprintf(_T("Connecting to \"%hs\" as \"%hs\"; encryption is %s\n"), optHost, optUser,
				optEncrypt ? _T("enabled") : _T("disabled"));
		}

#ifdef UNICODE
		WCHAR *wHost = WideStringFromMBString(optHost);
		WCHAR *wUser = WideStringFromMBString(optUser);
		WCHAR *wPassword = WideStringFromMBString(optPassword);
#define _HOST wHost
#define _USER wUser
#define _PASSWD wPassword
#else
#define _HOST optHost
#define _USER optUser
#define _PASSWD optPassword
#endif

      session = new NXCSession();
      static UINT32 protocolVersions[] = { CPV_INDEX_PUSH };
      dwResult = session->connect(_HOST, _USER, _PASSWD, optEncrypt ? NXCF_ENCRYPT : 0, _T("nxpush/") NETXMS_VERSION_STRING,
                                  protocolVersions, sizeof(protocolVersions) / sizeof(UINT32));

#ifdef UNICODE
		free(wHost);
		free(wUser);
		free(wPassword);
#endif
		if (dwResult != RCC_SUCCESS)
		{
			if (optVerbose > 0)
			{
				_tprintf(_T("Unable to connect to the server: %s\n"), NXCGetErrorText(dwResult));
			}
		}
		else
		{
         session->setCommandTimeout(5 * 1000);
			ret = TRUE;
		}
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

	if (optBatchSize == 0)
	{
		optBatchSize = queue.size();
	}

	batches = queue.size() / optBatchSize;
	if (queue.size() % optBatchSize != 0)
	{
		batches++;
	}

	for (i = 0; i < batches; i++)
	{
		size = min(optBatchSize, queue.size() - (optBatchSize * i));

		if (optVerbose > 1)
		{
			_tprintf(_T("Sending batch #%d with %d records\n"), i + 1, size);
		}

		if (optVerbose > 2)
		{
			for (int j = 0; j < size; j++)
			{
            NXCPushData *rec = queue.get(optBatchSize * i + j);
				_tprintf(_T("Record #%d: \"%s\" for %d(%s):%d(%s)\n"),
					(optBatchSize * i) + j + 1,
					rec->value,
					rec->nodeId,
					rec->nodeName != NULL ? rec->nodeName : _T("n/a"),
					rec->dciId,
					rec->dciName != NULL ? rec->dciName : _T("n/a"));
			}
		}

      UINT32 dwResult = ((DataCollectionController *)(session->getController(CONTROLLER_DATA_COLLECTION)))->pushData(&queue, &errIdx);
		if (dwResult != RCC_SUCCESS)
		{
			if (optVerbose > 0)
			{
				_tprintf(_T("Push failed at record #%d (#%d in batch): %s.\n"),
					(i * optBatchSize) + errIdx + 1,
					errIdx + 1,
					NXCGetErrorText(dwResult));
			}

			ret = FALSE;
			break;
		}
		else
		{
			if (optVerbose > 1)
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
	if (session != NULL)
	{
      delete session;
	}

	for(int i = 0; i < queue.size(); i++)
	{
      NXCPushData *d = queue.get(i);
		safe_free(d->dciName);
		safe_free(d->nodeName);
		safe_free(d->value);
	}

	return TRUE;
}
