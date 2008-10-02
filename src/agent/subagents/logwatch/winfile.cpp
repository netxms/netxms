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
** File: winfile.cpp
**
**/

#include "logwatch.h"


//
// Constants
//

#define READ_BUFFER_SIZE      4096


//
// Parse new log records
//

static void ParseNewRecords(LogParser *parser, HANDLE hFile)
{
   char *ptr, *eptr, buffer[READ_BUFFER_SIZE];
   DWORD bytes, bufPos = 0;

   do
   {
      if (ReadFile(hFile, buffer, READ_BUFFER_SIZE - bufPos, &bytes, NULL))
      {
         bytes += bufPos;
         for(ptr = buffer;; ptr = eptr + 2)
         {
            eptr = strstr(ptr, "\r\n");
            if (eptr == NULL)
            {
               bufPos = ptr - buffer;
               memmove(buffer, ptr, bytes - bufPos);
               break;
            }
            *eptr = 0;
				parser->MatchLine(ptr);
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

THREAD_RESULT THREAD_CALL ParserThreadFile(void *arg)
{
	LogParser *parser = (LogParser *)arg;
   HANDLE hFile, hChange, handles[2];
   LARGE_INTEGER fp, fsnew;
   TCHAR path[MAX_PATH], fname[MAX_PATH], *pch;
	time_t t;
	struct tm *ltm;

   nx_strncpy(path, parser->GetFileName(), MAX_PATH);
   pch = _tcsrchr(path, _T('\\'));
   if (pch != NULL)
      *pch = 0;

	t = time(NULL);
	ltm = localtime(&t);
	_tcsftime(fname, MAX_PATH, parser->GetFileName(), ltm);

   hFile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, OPEN_EXISTING, 0, NULL);
   if (hFile != INVALID_HANDLE_VALUE)
   {
      GetFileSizeEx(hFile, &fp);
      CloseHandle(hFile);

      hChange = FindFirstChangeNotification(path, FALSE, FILE_NOTIFY_CHANGE_SIZE);
      if (hChange != INVALID_HANDLE_VALUE)
      {
		   NxWriteAgentLog(EVENTLOG_DEBUG_TYPE, _T("LogWatch: start tracking log file %s"), parser->GetFileName());
			handles[0] = hChange;
			handles[1] = g_hCondShutdown;
         while(1)
         {
            if (WaitForMultipleObjects(2, handles, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
					break;

				t = time(NULL);
				ltm = localtime(&t);
				_tcsftime(fname, MAX_PATH, parser->GetFileName(), ltm);

            hFile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, OPEN_EXISTING, 0, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
               GetFileSizeEx(hFile, &fsnew);
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
						}
						ParseNewRecords(parser, hFile);
						GetFileSizeEx(hFile, &fp);
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
		NxWriteAgentLog(EVENTLOG_ERROR_TYPE, _T("LogWatch: Cannot open log file %s"), parser->GetFileName());
   }

	return THREAD_OK;
}
