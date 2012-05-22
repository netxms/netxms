/* 
** nxapush - command line tool used to push DCI values to NetXMS server
**           via local NetXMS agent
** Copyright (C) 2006-2012 Raden Solutions
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

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/un.h>
#endif

#ifndef SUN_LEN
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

//
// Static variables
//

#ifdef _WIN32
static HANDLE s_hPipe = NULL;
#else
static int s_hPipe = -1;
#endif
static StringMap *s_data = new StringMap;


//
// Forward declarations
//

BOOL AddValue(char *pair);
BOOL Startup();
BOOL Send();
BOOL Teardown();


//
// options
//

static int optVerbose = 1;
static int optBatchSize = 0;

#if HAVE_DECL_GETOPT_LONG
static struct option longOptions[] =
{
	{"version",   no_argument,       NULL,        'V'},
	{"help",      no_argument,       NULL,        'h'},
	{"verbose",   no_argument,       NULL,        'v'},
	{"quiet",     no_argument,       NULL,        'q'},
	{"batchsize", required_argument, NULL,        'b'},
	{NULL, 0, NULL, 0}
};
#endif

#define SHORT_OPTIONS "Vhvqb:"

//
//
//
static void usage(char *argv0)
{
	printf(
"NetXMS Agent PUSH  Version " NETXMS_VERSION_STRING "\n"
"Copyright (c) 2006-2012 Raden Solutions\n\n"
"Usage: %s [OPTIONS] [@batch_file] [values]\n"
"  \n"
"Options:\n"
#if HAVE_GETOPT_LONG
"  -V, --version              Display version information.\n"
"  -h, --help                 Display this help message.\n"
"  -v, --verbose              Enable verbose messages. Add twice for debug\n"
"  -q, --quiet                Suppress all messages.\n\n"
#else
"  -V             Display version information.\n"
"  -h             Display this help message.\n"
"  -v             Enable verbose messages. Add twice for debug\n"
"  -q             Suppress all messages.\n\n"
#endif
"Notes:\n"
"  * Values should be given in the following format:\n"
"    dci=value\n"
"    where dci can be specified by it's name\n"
"  * Name of batch file cannot contain character = (equality sign)\n"
"\n"
"Examples:\n"
"  Push two values:\n"
"      nxapush PushParam1=1 PushParam2=4\n\n"
"  Push values from file:\n"
"      nxapush @file\n"
	, argv0);
}

//
// Entry point
//
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
		case 'V': // version
			printf("nxapush (" NETXMS_VERSION_STRING ")\n");
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
		case 'b': // batch size
			optBatchSize = atoi(optarg); // 0 == unlimited
			break;
		case '?':
			exit(3);
			break;
		}
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

	if (s_data->getSize() > 0)
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
		s_data->set(pair, value);
		ret = TRUE;
	}

	return ret;
}

//
// Initialize client library and connect to the server
//
BOOL Startup()
{
#ifdef _WIN32
	s_hPipe = CreateFile(_T("\\\\.\\pipe\\nxagentd.push"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (s_hPipe == INVALID_HANDLE_VALUE)
		return FALSE;

	DWORD pipeMode = PIPE_READMODE_MESSAGE;
	SetNamedPipeHandleState(s_hPipe, &pipeMode, NULL, NULL);
#else
	s_hPipe = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s_hPipe == -1)
		return FALSE;

	struct sockaddr_un remote;
	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, "/tmp/.nxagentd.push");
	if (connect(s_hPipe, (struct sockaddr *)&remote, SUN_LEN(&remote)) == -1)
	{
		close(s_hPipe);
		s_hPipe = -1;
		return FALSE;
	}
#endif

	if (optVerbose > 2)
		_tprintf(_T("Connected to NetXMS agent\n"));

	return TRUE;
}

//
// Send all DCIs
//
BOOL Send()
{
	BOOL success = FALSE;

	CSCPMessage msg;
	msg.SetCode(CMD_PUSH_DCI_DATA);
   msg.SetVariable(VID_NUM_ITEMS, s_data->getSize());
	for(DWORD i = 0, varId = VID_PUSH_DCI_DATA_BASE; i < s_data->getSize(); i++)
	{
		msg.SetVariable(varId++, s_data->getKeyByIndex(i));
		msg.SetVariable(varId++, s_data->getValueByIndex(i));
		if (optVerbose > 2)
			printf("Record #%d: \"%s\" = \"%s\"\n", i + 1, s_data->getKeyByIndex(i), s_data->getValueByIndex(i));
	}

	// Send response to pipe
	CSCP_MESSAGE *rawMsg = msg.CreateMessage();
#ifdef _WIN32
	DWORD bytes;
	if (!WriteFile(s_hPipe, rawMsg, ntohl(rawMsg->dwSize), &bytes, NULL))
		goto cleanup;
	if (bytes != ntohl(rawMsg->dwSize))
		goto cleanup;
#else
	int bytes = SendEx(s_hPipe, rawMsg, ntohl(rawMsg->dwSize), 0, NULL); 
	if (bytes != (int)ntohl(rawMsg->dwSize))
		goto cleanup;
#endif

	success = TRUE;

cleanup:
	free(rawMsg);
	return success;
}

//
// Disconnect and cleanup
//
BOOL Teardown()
{
#ifdef _WIN32
	if (s_hPipe != NULL)
	{
		CloseHandle(s_hPipe);
	}
#else
	if (s_hPipe != -1)
	{
		close(s_hPipe);
	}
#endif
	delete s_data;
	return TRUE;
}
