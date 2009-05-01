/*
** NetXMS - Network Management System
** Log Parsing Library
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

#include "libnxlp.h"


//
// Constants
//

#define READ_BUFFER_SIZE      4096


//
// Generate current file name
// 

static void GenerateCurrentFileName(LogParser *parser, char *fname)
{
	time_t t;
	struct tm *ltm;
#if HAVE_LOCALTIME_R
	struct tm buffer;
#endif

	t = time(NULL);
#if HAVE_LOCALTIME_R
	ltm = localtime_r(&t, &buffer);
#else
	ltm = localtime(&t);
#endif
	_tcsftime(fname, MAX_PATH, parser->getFileName(), ltm);
}


//
// Parse new log records
//

static void ParseNewRecords(LogParser *parser, int fh)
{
   char *ptr, *eptr, buffer[READ_BUFFER_SIZE];
   int bytes, bufPos = 0;

   do
   {
      if ((bytes = read(fh, &buffer[bufPos], READ_BUFFER_SIZE - bufPos)) > 0)
      {
         bytes += bufPos;
         for(ptr = buffer;; ptr = eptr + 1)
         {
            bufPos = ptr - buffer;
            eptr = (char *)memchr(ptr, '\n', bytes - bufPos);
            if (eptr == NULL)
            {
					bufPos = bytes - bufPos;
               memmove(buffer, ptr, bufPos);
               break;
            }
				if (*(eptr - 1) == '\r')
					*(eptr - 1) = 0;
				else
					*eptr = 0;
				parser->matchLine(ptr);
         }
      }
      else
      {
         bytes = 0;
      }
   } while(bytes == READ_BUFFER_SIZE);
}


//
// File parser thread
//

bool LogParser::monitorFile(CONDITION stopCondition, bool *stopFlag, void (*logger)(int, const TCHAR *, ...))
{
	char fname[MAX_PATH], temp[MAX_PATH];
	struct stat st;
	size_t size;
	int err, fh;

	if ((m_fileName == NULL) || (stopFlag == NULL))
		return false;


	if (logger != NULL)
		logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: parser thread for file \"%s\" started"), m_fileName);

	while(!(*stopFlag))
	{
		GenerateCurrentFileName(this, fname);
		if (stat(fname, &st) == 0)
		{
			fh = open(fname, O_RDONLY);
			if (fh != -1)
			{
				size = st.st_size;
				lseek(fh, 0, SEEK_END);

				if (logger != NULL)
					logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: file \"%s\" successfully opened"), m_fileName);

				while(!(*stopFlag))
				{
					ThreadSleep(1);

					// Check if file name was changed
					GenerateCurrentFileName(this, temp);
					if (_tcscmp(temp, fname))
					{
						if (logger != NULL)
							logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: file name change for \"%s\" (\"%s\" -> \"%s\")"),
						          m_fileName, fname, temp);
						break;
					}
					
					if (fstat(fh, &st) < 0)
						break;
						
					if (st.st_size != size)
					{
						if (st.st_size < size)
						{
							// File was cleared, start from the beginning
							lseek(fh, 0, SEEK_SET);
							if (logger != NULL)
								logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: file \"%s\" st_size != size"), m_fileName);
						}
						size = st.st_size;
						if (logger != NULL)
							logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: new data avialable in file \"%s\""), m_fileName);
						ParseNewRecords(this, fh);
					}
				}
				close(fh);
			}
		}
		else
		{
			if (ConditionWait(stopCondition, 10000))
				break;
		}
	}

	if (logger != NULL)
		logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: parser thread for file \"%s\" stopped"), m_fileName);
	return true;
}
