/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
**
** File: winfile.cpp
**
**/

#include "libnxlp.h"

#define LOG if (logger != NULL) logger


//
// Constants
//

#define READ_BUFFER_SIZE      4096


//
// Parse new log records
//

static void ParseNewRecords(LogParser *parser, HANDLE hFile, void (*logger)(int, const TCHAR *, ...))
{
   char *ptr, *eptr, buffer[READ_BUFFER_SIZE];
   DWORD bytes, bufPos = 0;

   do
   {
      if (ReadFile(hFile, &buffer[bufPos], READ_BUFFER_SIZE - bufPos, &bytes, NULL))
      {
         bytes += bufPos;
         for(ptr = buffer;; ptr = eptr + 1)
         {
            bufPos = (DWORD)(ptr - buffer);
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
// Monitor file changes and parse line by line
//

bool LogParser::monitorFile(HANDLE stopEvent, void (*logger)(int, const TCHAR *, ...), bool readFromCurrPos)
{
   HANDLE hFile, hChange, handles[2];
   LARGE_INTEGER fp, fsnew;
   TCHAR path[MAX_PATH], fname[MAX_PATH], *pch;
	time_t t;
	struct tm *ltm;

	if ((m_fileName == NULL) || (stopEvent == NULL))
		return FALSE;

   nx_strncpy(path, m_fileName, MAX_PATH);
   pch = _tcsrchr(path, _T('\\'));
   if (pch != NULL)
      *pch = 0;

	t = time(NULL);
	ltm = localtime(&t);
	_tcsftime(fname, MAX_PATH, m_fileName, ltm);

   hFile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, OPEN_EXISTING, 0, NULL);
   if (hFile != INVALID_HANDLE_VALUE)
   {
		if (!readFromCurrPos)
		{
			if (logger != NULL)
				logger(EVENTLOG_DEBUG_TYPE, _T("LogParser: parsing existing records in file \"%s\""), m_fileName);
			ParseNewRecords(this, hFile, logger);
		}
      GetFileSizeEx(hFile, &fp);
      CloseHandle(hFile);

      hChange = FindFirstChangeNotification(path, FALSE, FILE_NOTIFY_CHANGE_SIZE);
      if (hChange != INVALID_HANDLE_VALUE)
      {
			LOG(EVENTLOG_DEBUG_TYPE, _T("LogParser: start tracking log file %s"), m_fileName);
			handles[0] = hChange;
			handles[1] = stopEvent;
         while(1)
         {
            if (WaitForMultipleObjects(2, handles, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
					break;

				t = time(NULL);
				ltm = localtime(&t);
				_tcsftime(fname, MAX_PATH, m_fileName, ltm);

				LOG(EVENTLOG_DEBUG_TYPE, _T("LogParser: new data in file %s"), fname);
            hFile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, OPEN_EXISTING, 0, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
               GetFileSizeEx(hFile, &fsnew);
					LOG(EVENTLOG_DEBUG_TYPE, _T("LogParser: file %s oldSize = %I64d newSize = %I64d"), fname, fp.QuadPart, fsnew.QuadPart);
               if (fsnew.QuadPart != fp.QuadPart)
					{
						if (fsnew.QuadPart > fp.QuadPart)
						{
							SetFilePointerEx(hFile, fp, NULL, FILE_BEGIN);
						}
						else
						{
							// Assume that file was rewritten
							fp.QuadPart = 0;
							SetFilePointerEx(hFile, fp, NULL, FILE_BEGIN);
							LOG(EVENTLOG_DEBUG_TYPE, _T("LogParser: file %s - assume overwrite"), fname);
						}
						ParseNewRecords(this, hFile, logger);
						GetFileSizeEx(hFile, &fp);
						LOG(EVENTLOG_DEBUG_TYPE, _T("LogParser: file %s size after parse is %I64d"), fname, fp.QuadPart);
					}
               CloseHandle(hFile);
            }
            FindNextChangeNotification(hChange);
         }

         FindCloseChangeNotification(hChange);
      }
   }
   else
   {
		if (logger != NULL)
			logger(EVENTLOG_ERROR_TYPE, _T("LogParser: Cannot open log file %s"), m_fileName);
   }

	return true;
}
