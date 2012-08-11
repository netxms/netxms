/*
** NetXMS - Network Management System
** Log Parsing Library
*** Copyright (C) 2003-2011 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
** File: file.cpp
**
**/

#include "libnxlp.h"

#ifdef _NETWARE
#include <fsio.h>
#endif

#ifdef _WIN32
#include <share.h>

#define read _read
#define close _close

#endif


//
// Constants
//

#define READ_BUFFER_SIZE      4096


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
            bufPos = (int)(ptr - buffer);
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

bool LogParser::monitorFile(CONDITION stopCondition, void (*logger)(int, const char *, ...), bool readFromCurrPos)
{
	char fname[MAX_PATH], temp[MAX_PATH];
	struct stat st, stn;
	size_t size;
	int fh;
	bool readFromStart = !readFromCurrPos;

	if (m_fileName == NULL)
		return false;

	if (logger != NULL)
		logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: parser thread for file \"%s\" started"), m_fileName);

	while(1)
	{
		ExpandFileName(getFileName(), fname, MAX_PATH);
		if (stat(fname, &st) == 0)
		{
#ifdef _WIN32
			fh = _sopen(fname, O_RDONLY, _SH_DENYNO);
#else
			fh = open(fname, O_RDONLY);
#endif
			if (fh != -1)
			{
				setStatus(LPS_RUNNING);
				if (logger != NULL)
					logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: file \"%s\" (pattern \"%s\") successfully opened"), fname, m_fileName);

				size = st.st_size;
				if (readFromStart)
				{
					if (logger != NULL)
						logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: parsing existing records in file \"%s\""), fname);
					ParseNewRecords(this, fh);
					readFromStart = false;
				}
				else
				{
					lseek(fh, 0, SEEK_END);
				}

				while(1)
				{
					if (ConditionWait(stopCondition, 5000))
						goto stop_parser;

					// Check if file name was changed
					ExpandFileName(getFileName(), temp, MAX_PATH);
					if (_tcscmp(temp, fname))
					{
						if (logger != NULL)
							logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: file name change for \"%s\" (\"%s\" -> \"%s\")"),
						          m_fileName, fname, temp);
						readFromStart = true;
						break;
					}

#ifdef _NETWARE
					if (fgetstat(fh, &st, ST_SIZE_BIT | ST_NAME_BIT) < 0)
					{
						if (logger != NULL)
							logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: fgetstat(%d) failed, errno=%d"), fh, errno);
						readFromStart = true;
						break;
					}
#else					
					if (fstat(fh, &st) < 0)
					{
						if (logger != NULL)
							logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: fstat(%d) failed, errno=%d"), fh, errno);
						readFromStart = true;
						break;
					}
#endif

					if (stat(fname, &stn) < 0)
					{
						if (logger != NULL)
							logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: stat(%s) failed, errno=%d"), fname, errno);
						readFromStart = true;
						break;
					}
					
					if (st.st_size != stn.st_size)
					{
						if (logger != NULL)
							logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: file size differs for stat(%d) and fstat(%s), assume file rename"), fh, fname);
						readFromStart = true;
						break;
					}
						
					if ((size_t)st.st_size != size)
					{
						if ((size_t)st.st_size < size)
						{
							// File was cleared, start from the beginning
							lseek(fh, 0, SEEK_SET);
							if (logger != NULL)
								logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: file \"%s\" st_size != size"), fname);
						}
						size = (size_t)st.st_size;
						if (logger != NULL)
							logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: new data avialable in file \"%s\""), fname);
						ParseNewRecords(this, fh);
					}
				}
				close(fh);
			}
			else
			{
				setStatus(LPS_OPEN_ERROR);
			}
		}
		else
		{
			setStatus(LPS_NO_FILE);
			if (ConditionWait(stopCondition, 10000))
				break;
		}
	}

stop_parser:
	if (logger != NULL)
		logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: parser thread for file \"%s\" stopped"), m_fileName);
	return true;
}
