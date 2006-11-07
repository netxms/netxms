/* $Id: nxpush.cpp,v 1.4 2006-11-07 23:28:06 alk Exp $ */

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

#define SHORT_OPTIONS "Vhvqu:P:eH:b:"

//
//
//
static void usage(char *argv0)
{
	printf(
"Usage: %s [OPTIONS] [server] [@batch_file] [values]\n"
"  \n"
"Options:\n"
"  -V, --version    Display version information.\n"
"  -h, --help       Display this help message.\n"
"  -v, --verbose    Enable debug messages.\n"
"  -q, --quiet      Suppress all messages.\n\n"
"  -u, --user       Login to server as user. Default is \"guest\".\n"
"  -P, --password   Specify user's password. Default is empty.\n"
"  -e, --encrypt    Encrypt session.\n"
"  -H, --host       Server address.\n\n"
"Notes:\n"
"  First parameter will be used as \"server\" if -H/--host is unset\n"
"  ...\n"
"\n\n"
"Examples:\n"
"  ...\n"
	, argv0);
}

//
// Entry point
//
int main(int argc, char *argv[])
{
	int c;

	opterr = 0;
	while ((c = getopt_long(argc, argv, SHORT_OPTIONS, longOptions, NULL)) != -1)
	{
		switch(c)
		{
		case 'V': // version
			printf("nxpush (" NETXMS_VERSION_STRING ")\n");
			exit(0);
			break;
		case 'h': // help
			usage(argv[0]);
			exit(0);
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
			printf("Try `%s --help' for more information.\n", argv[0]);
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
			Send();
		}
	}
	else
	{
		if (optVerbose > 0)
		{
			printf("No valid pairs found; nothing to send\n");
		}
	}
	Teardown();

	return 0;
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
		if (optVerbose > 1)
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
				p[queueSize].pszName = dciName;
			}

			if (nodeId > 0)
			{
				p[queueSize].pszNodeName = NULL;
			}
			else
			{
				p[queueSize].pszNodeName = nodeName;
			}

			p[queueSize].pszValue = value;

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
		if (optVerbose > 1)
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

		if (NXCPushDCIData(hSession, size, &queue[optBatchSize * i], &errIdx) != RCC_SUCCESS)
		{
			if (optVerbose > 0)
			{
				printf("Push failed at record %d.\n", errIdx);
			}
		}
		else
		{
			if (optVerbose > 1)
			{
				printf("Done.\n");
			}
		}
	}

	return TRUE;
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
		free(queue);
		queue = NULL;
		queueSize = 0;
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
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
