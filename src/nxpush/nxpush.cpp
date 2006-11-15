/* $Id: nxpush.cpp,v 1.11 2006-11-15 22:42:14 victor Exp $ */

/* 
** nxpush - command line tool used to push DCI values to NetXMS server
** Copyright (C) 2006 Alex Kirhenshtein
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
#include <nxclapi.h>

#if HAVE_GETOPT_LONG && !HAVE_DECL_GETOPT_LONG
#include <getopt.h>
#endif


//
// Global variables
//

NXC_SESSION hSession = NULL;
NXC_DCI_PUSH_DATA *queue = NULL;
int queueSize = 0;


//
// Forward declarations
//

BOOL AddValuePair(char *name, char *value);
BOOL AddValue(char *pair);
BOOL Startup(void);
BOOL Send(void);
BOOL Teardown(void);


//
// options
//

static int optVerbose = 1;
static char *optHost = NULL;
static char *optUser = "guest";
static char *optPassword = "";
static BOOL optEncrypt = FALSE;
static int optBatchSize = 0;

#if HAVE_GETOPT_LONG
static struct option longOptions[] =
{
	{"version",   no_argument,       NULL,        'V'},
	{"help",      no_argument,       NULL,        'h'},
	{"verbose",   no_argument,       NULL,        'v'},
	{"quiet",     no_argument,       NULL,        'q'},
	{"user",      required_argument, NULL,        'u'},
	{"password",  required_argument, NULL,        'P'},
	{"encrypt",   required_argument, NULL,        'e'},
	{"host",      required_argument, NULL,        'H'},
	{"batchsize", required_argument, NULL,        'b'},
	{NULL, 0, NULL, 0}
};
#endif

#define SHORT_OPTIONS "Vhvqu:P:eH:b:"

//
//
//
static void usage(char *argv0)
{
	printf(
"NetXMS PUSH  Version " NETXMS_VERSION_STRING "\n"
"Copyright (c) 2006 Alex Kirhenshtein\n\n"
"Usage: %s [OPTIONS] [server] [@batch_file] [values]\n"
"  \n"
"Options:\n"
#if HAVE_GETOPT_LONG
"  -V, --version              Display version information.\n"
"  -h, --help                 Display this help message.\n"
"  -v, --verbose              Enable verbose messages. Add twice for debug\n"
"  -q, --quiet                Suppress all messages.\n\n"
"  -u, --user     <user>      Login to server as user. Default is \"guest\".\n"
"  -P, --password <password>  Specify user's password. Default is empty.\n"
"  -e, --encrypt              Encrypt session.\n"
"  -H, --host     <host>      Server address.\n\n"
#else
"  -V             Display version information.\n"
"  -h             Display this help message.\n"
"  -v             Enable verbose messages. Add twice for debug\n"
"  -q             Suppress all messages.\n\n"
"  -u <user>      Login to server as user. Default is \"guest\".\n"
"  -P <password>  Specify user's password. Default is empty.\n"
"  -e             Encrypt session.\n"
"  -H <host>      Server address.\n\n"
#endif
"Notes:\n"
"  * Values should be given in the following format:\n"
"    node:dci=value\n"
"    where node and dci can be specified either by ID or name\n"
"  * First parameter will be used as \"host\" if -H/--host is unset\n"
"\n"
"Examples:\n"
"  Push two values to server 10.0.0.1 as user \"sender\" with password \"passwd\":\n"
"      nxpush -H 10.0.0.1 -u sender -P passwd 10:24=1 10:PushParam=4\n\n"
"  Push values from file to server 10.0.0.1 as user \"guest\" without password:\n"
"      nxpush 10.0.0.1 @file\n"
	, argv0);
}

//
// Entry point
//
int main(int argc, char *argv[])
{
	int ret = 0;
	int c;

	opterr = 0;
#if HAVE_GETOPT_LONG
	while ((c = getopt_long(argc, argv, SHORT_OPTIONS, longOptions, NULL)) != -1)
#else
	while ((c = getopt(argc, argv, SHORT_OPTIONS)) != -1)
#endif
	{
		switch(c)
		{
		case 'V': // version
			printf("nxpush (" NETXMS_VERSION_STRING ")\n");
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
			printf("Not enough arguments\n\n");
#if HAVE_GETOPT_LONG
			printf("Try `%s --help' for more information.\n", argv[0]);
#else
			printf("Try `%s -h' for more information.\n", argv[0]);
#endif
		}
		exit(1);
	}

	//
	// Parse
	//
	if (optind < argc)
	{
		while (optind < argc)
		{
			char *p = argv[optind];

			if ((*p == '@' && *p != 0) || *p == '-')
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
						printf("Cannot open \"%s\": %s\n", p + 1, strerror(errno));
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

	if (queueSize > 0)
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
			printf("No valid pairs found; nothing to send\n");
			ret = 2;
		}
	}
	Teardown();

	return ret;
}

//
// Values parser - clean string and split by '='
//
BOOL AddValue(char *pair)
{
	BOOL ret = FALSE;
	char *p = pair;
	char *value = NULL;

	for (p = pair; *p != NULL; p++)
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

//
// Split IDs, cleanup and add to the send queue
//
BOOL AddValuePair(char *name, char *value)
{
	BOOL ret = TRUE;
	DWORD dciId = 0;
	DWORD nodeId = 0;
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
			printf("AddValuePair: dciID=\"%d\", nodeName=\"%s\", dciName=\"%s\", value=\"%s\"\n",
				dciId, nodeName, dciName, value);
		}

		NXC_DCI_PUSH_DATA *p = (NXC_DCI_PUSH_DATA *)realloc(
			queue, sizeof(NXC_DCI_PUSH_DATA) * (queueSize + 1));
		if (p != NULL)
		{
			queue = p;
			p[queueSize].dwId = dciId;
			p[queueSize].dwNodeId = nodeId;
			if (dciId > 0)
			{
				p[queueSize].pszName = NULL;
			}
			else
			{
				p[queueSize].pszName = strdup(dciName);
			}

			if (nodeId > 0)
			{
				p[queueSize].pszNodeName = NULL;
			}
			else
			{
				p[queueSize].pszNodeName = strdup(nodeName);
			}

			p[queueSize].pszValue = strdup(value);

			queueSize++;
		}
		else
		{
			if (optVerbose > 0)
			{
				printf("realloc failed!: %s\n", strerror(errno));
			}
		}
	}

	return ret;
}

//
// Callback function for debug messages from client library
//
static void DebugCallback(char *pMsg)
{
	printf("NXCL: %s\n", pMsg);
}

//
// Initialize client library and connect to the server
//
BOOL Startup(void)
{
	BOOL ret = FALSE;
	DWORD dwResult;

#ifdef _WIN32
	WSADATA wsaData;

	if (WSAStartup(2, &wsaData) != 0)
	{
		if (optVerbose > 0)
		{
			printf("Unable to initialize Windows sockets\n");
		}
		return FALSE;
	}
#endif

	if (!NXCInitialize())
	{
		if (optVerbose > 0)
		{
			printf("Failed to initialize NetXMS client library\n");
		}
	}
	else
	{
		if (optVerbose > 2)
		{
			NXCSetDebugCallback(DebugCallback);

			printf("Connecting to \"%s\" as \"%s\"; encryption is %s\n", optHost, optUser,
				optEncrypt == TRUE ? "enabled" : "disabled");
		}

		dwResult = NXCConnect(optHost, optUser, optPassword, &hSession,
					"nxpush/" NETXMS_VERSION_STRING, FALSE, optEncrypt);
		if (dwResult != RCC_SUCCESS)
		{
			if (optVerbose > 0)
			{
				printf("Unable to connect to the server: %s\n", NXCGetErrorText(dwResult));
			}
		}
		else
		{
			NXCSetCommandTimeout(hSession, 5 * 1000);
			ret = TRUE;
		}
	}

	return ret;
}

//
// Send all DCIs
//
BOOL Send(void)
{
	BOOL ret = TRUE;
	DWORD errIdx;
	
	int i, size;
	int batches = 1;

	if (optBatchSize == 0)
	{
		optBatchSize = queueSize;
	}

	batches = queueSize / optBatchSize;
	if (queueSize % optBatchSize != 0)
	{
		batches++;
	}

	for (i = 0; i < batches; i++)
	{
		size = min(optBatchSize, queueSize - (optBatchSize * i));

		if (optVerbose > 1)
		{
			printf("Sending batch #%d with %d records\n", i + 1, size);
		}

		if (optVerbose > 2)
		{
			for (int j = 0; j < size; j++)
			{
				NXC_DCI_PUSH_DATA *rec = &queue[(optBatchSize * i) + j];
				printf("Record #%d: \"%s\" for %d(%s):%d(%s)\n",
					(optBatchSize * i) + j + 1,
					rec->pszValue,
					rec->dwNodeId,
					rec->pszNodeName != NULL ? rec->pszNodeName : "n/a",
					rec->dwId,
					rec->pszName != NULL ? rec->pszName : "n/a");
			}
		}

		DWORD dwResult = NXCPushDCIData(hSession, size, &queue[optBatchSize * i], &errIdx);
		if (dwResult != RCC_SUCCESS)
		{
			if (optVerbose > 0)
			{
				printf("Push failed at record #%d (#%d in batch): %s.\n",
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
				printf("Done.\n");
			}
		}
	}

	return ret;
}

//
// Disconnect and cleanup
//
BOOL Teardown(void)
{
	if (hSession != NULL)
	{
		NXCDisconnect(hSession);
	}

	if (queue != NULL)
	{
		for (int i = 0; i < queueSize; i++)
		{
			if (queue[i].pszName != NULL) free(queue[i].pszName);
			if (queue[i].pszNodeName != NULL) free(queue[i].pszNodeName);
			if (queue[i].pszValue != NULL) free(queue[i].pszValue);
		}
		free(queue);
		queue = NULL;
		queueSize = 0;
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.10  2006/11/08 13:04:54  victor
Minor fix

Revision 1.9  2006/11/08 13:03:35  victor
Added check for getopt_long() presense

Revision 1.8  2006/11/08 11:43:15  victor
Minor changes

Revision 1.7  2006/11/08 09:05:05  victor
- Implemented node name resolution for NXCPushDCIData
- Help for nxpush improved

Revision 1.6  2006/11/08 00:08:30  alk
minor changes;
error codes corected - 0=ok, 1=wrong params, 2=no valid data, 3=unable to send

Revision 1.5  2006/11/07 23:54:08  alk
fixed bug in file loader

Revision 1.4  2006/11/07 23:28:06  alk
-b/--batchsize added; default is 0 (unlimited)

Revision 1.3  2006/11/07 23:10:56  alk
nxpush working(?)
node/dci separator changed from '.' to ':'
node id/name now required for operation

Revision 1.2  2006/11/07 15:45:09  victor
- Implemented frontend for push items
- Other minor changes

Revision 1.1  2006/11/07 11:10:32  victor
- nxpush moved and added to common netxms.dsw file
- unfinished discovery configurator in console

Revision 1.2  2006/11/07 00:08:04  alk
posilhed a bit; complete(?) set of command line switches;

Revision 1.1  2006/11/06 22:27:33  alk
initial import of nxpush


*/
