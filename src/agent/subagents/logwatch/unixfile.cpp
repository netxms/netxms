/*
** NetXMS LogWatch subagent
** Copyright (C) 2008 Victor Kirhenshtein
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
** File: unixfile.cpp
**
**/

#include "logwatch.h"


//
// Generate current file name
// 

static void GenerateCurrentFileName(LogParser *parser, char *fname)
{
	time_t t;
	struct tm *ltm;

	t = time(NULL);
#if HAVE_LOCALTIME_R
	ltm = localtime_r(&t);
#else
	ltm = localtime(&t);
#endif
	_tcsftime(fname, MAX_PATH, parser->GetFileName(), ltm);
}


//
// File parser thread
//

THREAD_RESULT THREAD_CALL ParserThreadFile(void *arg)
{
	LogParser *parser = (LogParser *)arg;
	char fname[MAX_PATH];
	struct stat st;
	int err, fh;
	fd_set rdfs;
	struct timeval tv;

	while(1)
	{
		GenerateCurrentFileName(parser, fname);
		if (stat(fname, &st) == 0)
		{
			fh = open(fname, O_RDONLY);
			if (fh != -1)
			{
				FD_ZERO(&rdfs);
				FD_SET(fh, &rdfs);
				while(1)
				{
					err = select(fh + 1, &rdfs, NULL, NULL, &tv);
					if (err < 0)
						break;
				}
				close(fh);
			}
		}
		else
		{
			ThreadSleep(10);
		}
	}

	return THREAD_OK;
}
