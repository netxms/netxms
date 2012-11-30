/* 
** nxappget - command line tool for reading metrics from application agents
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

#include <appagent.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif


//
// options
//

static int optVerbose = 1;
static int optBatchSize = 0;

#if HAVE_DECL_GETOPT_LONG
static struct option longOptions[] =
{
	{ (char *)"version",   no_argument,       NULL,        'V' },
	{ (char *)"help",      no_argument,       NULL,        'h' },
	{ (char *)"verbose",   no_argument,       NULL,        'v' },
	{ (char *)"quiet",     no_argument,       NULL,        'q' },
	{ NULL, 0, NULL, 0 }
};
#endif

#define SHORT_OPTIONS "Vhvq"

/**
 * Show online help
 */
static void usage(char *argv0)
{
	_tprintf(
_T("NetXMS Application Agent Connector  Version ") NETXMS_VERSION_STRING _T("\n")
_T("Copyright (c) 2006-2012 Raden Solutions\n\n")
_T("Usage: %hs agent_name metric_name\n")
_T("  \n")
_T("Options:\n")
#if HAVE_GETOPT_LONG
_T("  -V, --version              Display version information.\n")
_T("  -h, --help                 Display this help message.\n")
_T("  -v, --verbose              Enable verbose messages. Add twice for debug\n")
_T("  -q, --quiet                Suppress all messages.\n\n")
#else
_T("  -V             Display version information.\n")
_T("  -h             Display this help message.\n")
_T("  -v             Enable verbose messages. Add twice for debug\n")
_T("  -q             Suppress all messages.\n\n")
#endif
	, argv0);
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
		case 'V': // version
			_tprintf(_T("nxappget (") NETXMS_VERSION_STRING _T(")\n"));
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
		case '?':
			exit(3);
			break;
		}
	}
	
	if (argc - optind < 2)
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

#ifdef UNICODE
	WCHAR *appName = WideStringFromMBString(argv[optind]);
	WCHAR *metricName = WideStringFromMBString(argv[optind + 1]);
#else
	char *appName = argv[optind];
	char *metricName = argv[optind + 1];
#endif
	HPIPE hPipe;
	if (AppAgentConnect(appName, &hPipe))
	{
		TCHAR value[256];
		int rcc = AppAgentGetMetric(hPipe, metricName, value, 256);
		if (rcc == APPAGENT_RCC_SUCCESS)
		{
			_tprintf(_T("%s\n"), value);
		}
		else
		{
			_tprintf(_T("ERROR: agent error %d\n"), rcc);
		}
		AppAgentDisconnect(hPipe);
	}
	else
	{
		if (optVerbose > 0)
			_tprintf(_T("ERROR: Cannot connect to application agent\n\n"));
		ret = 2;
	}
#ifdef UNICODE
	free(appName);
	free(metricName);
#endif

	return ret;
}
