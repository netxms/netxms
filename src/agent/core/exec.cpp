/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: exec.cpp
**
**/

#include "nxagentd.h"

#ifndef _WIN32
# include <sys/wait.h>
#endif


//
// Execute external command
//

DWORD ExecuteCommand(char *pszCommand, NETXMS_VALUES_LIST *pArgs)
{
   char *pszCmdLine, *sptr;
   DWORD i, iSize, dwRetCode = ERR_SUCCESS;

   DebugPrintf("EXEC: Expanding command \"%s\"", pszCommand);

   // Substitute $1 .. $9 with actual arguments
   if (pArgs != NULL)
   {
      iSize = strlen(pszCommand) + 1;
      pszCmdLine = (char *)malloc(iSize);
      for(sptr = pszCommand, i = 0; *sptr != 0; sptr++)
         if (*sptr == '$')
         {
            sptr++;
            if (*sptr == 0)
               break;   // Single $ character at the end of line
            if ((*sptr >= '1') && (*sptr <= '9'))
            {
               DWORD dwArg = *sptr - '1';

               if (dwArg < pArgs->dwNumStrings)
               {
                  int iArgLength;

                  // Extend resulting line
                  iArgLength = strlen(pArgs->ppStringList[dwArg]);
                  iSize += iArgLength;
                  pszCmdLine = (char *)realloc(pszCmdLine, iSize);
                  strcpy(&pszCmdLine[i], pArgs->ppStringList[dwArg]);
                  i += iArgLength;
               }
            }
            else
            {
               pszCmdLine[i++] = *sptr;
            }
         }
         else
         {
            pszCmdLine[i++] = *sptr;
         }
      pszCmdLine[i] = 0;
   }
   else
   {
      pszCmdLine = pszCommand;
   }

   DebugPrintf("EXEC: Executing \"%s\"", pszCmdLine);
#if defined(_WIN32)
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   // Fill in process startup info structure
   memset(&si, 0, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   si.dwFlags = 0;

   // Create new process
   if (!CreateProcess(NULL, pszCmdLine, NULL, NULL, FALSE, 
                      CREATE_NO_WINDOW | DETACHED_PROCESS,
                      NULL, NULL, &si, &pi))
   {
      WriteLog(MSG_CREATE_PROCESS_FAILED, EVENTLOG_ERROR_TYPE, "se", pszCmdLine, GetLastError());
      dwRetCode = ERR_EXEC_FAILED;
   }
   else
   {
      // Close all handles
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
   }
#elif defined(_NETWARE)
   if (system(pszCmdLine) == 0)
      dwRetCode = ERR_SUCCESS;
   else
      dwRetCode = ERR_EXEC_FAILED;
#else
   /* TODO: add UNIX code here */
   dwRetCode = ERR_EXEC_FAILED;
	{
		int nPid;
		char *pCmd[128];
		int i;
		char *pTmp;
		struct stat st;

		pTmp = pszCmdLine;
		for (i = 0; i < 128; i++)
		{
			pCmd[i] = pTmp;
			pTmp = strchr(pTmp, ' ');
			if (pTmp != NULL)
			{
				*pTmp = 0;
				pTmp++;
			}
			else
			{
				break;
			}
		}
		pCmd[i+1] = 0;

		if (stat(pCmd[0], &st) == 0 && st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
		{
			switch ((nPid = fork()))
			{
			case -1:
				perror("fork()");
				break;
			case 0: // child
				{
					int sd = open("/dev/null", O_RDWR);
					if (sd == -1)
						sd = open("/dev/null", O_RDONLY);
					close(0);
					close(1);
					close(2);
					dup2(sd, 0);
					dup2(sd, 1);
					dup2(sd, 2);
					close(sd);
					execv(pCmd[0], pCmd);
					// should not be reached
					//_exit((errno == EACCES || errno == ENOEXEC) ? 126 : 127);
					_exit(127);
				}
				break;
			default: // parent
				dwRetCode = ERR_SUCCESS;
				break;
			}
		}
	}
#endif

   // Cleanup
   if (pArgs != NULL)
      free(pszCmdLine);

   return dwRetCode;
}



//
// Handler function for external (user-defined) parameters
//

LONG H_ExternalParameter(char *pszCmd, char *pszArg, char *pValue)
{
   char *pszCmdLine, szBuffer[1024], szTempFile[MAX_PATH], *sptr;
   int i, iSize, iStatus;

   // Substitute $1 .. $9 with actual arguments
   iSize = strlen(pszArg) + 1;
   pszCmdLine = (char *)malloc(iSize);
   for(sptr = pszArg, i = 0; *sptr != 0; sptr++)
      if (*sptr == '$')
      {
         sptr++;
         if (*sptr == 0)
            break;   // Single $ character at the end of line
         if ((*sptr >= '1') && (*sptr <= '9'))
         {
            if (NxGetParameterArg(pszCmd, *sptr - '0', szBuffer, 1024))
            {
               int iArgLength;

               // Extend resulting line
               iArgLength = strlen(szBuffer);
               iSize += iArgLength;
               pszCmdLine = (char *)realloc(pszCmdLine, iSize);
               strcpy(&pszCmdLine[i], szBuffer);
               i += iArgLength;
            }
         }
         else
         {
            pszCmdLine[i++] = *sptr;
         }
      }
      else
      {
         pszCmdLine[i++] = *sptr;
      }
   pszCmdLine[i] = 0;

#if defined(_WIN32)
   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   SECURITY_ATTRIBUTES sa;
   HANDLE hOutput;
   DWORD dwBytes;

   // Create temporary file to hold process output
   GetTempPath(MAX_PATH - 1, szBuffer);
   GetTempFileName(szBuffer, "nx", 0, szTempFile);
   sa.nLength = sizeof(SECURITY_ATTRIBUTES);
   sa.lpSecurityDescriptor = NULL;
   sa.bInheritHandle = TRUE;
   hOutput = CreateFile(szTempFile, GENERIC_READ | GENERIC_WRITE, 0, &sa, 
                        CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
   if (hOutput != INVALID_HANDLE_VALUE)
   {
      // Fill in process startup info structure
      memset(&si, 0, sizeof(STARTUPINFO));
      si.cb = sizeof(STARTUPINFO);
      si.dwFlags = STARTF_USESTDHANDLES;
      si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
      si.hStdOutput = hOutput;
      si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

      // Create new process
      if (CreateProcess(NULL, pszCmdLine, NULL, NULL, TRUE, 
                        CREATE_NO_WINDOW | DETACHED_PROCESS,
                        NULL, NULL, &si, &pi))
      {
         // Wait for process termination and close all handles
         WaitForSingleObject(pi.hProcess, INFINITE);
         CloseHandle(pi.hThread);
         CloseHandle(pi.hProcess);

         // Rewind temporary file for reading
         SetFilePointer(hOutput, 0, NULL, FILE_BEGIN);

         // Read process output
         ReadFile(hOutput, pValue, MAX_RESULT_LENGTH - 1, &dwBytes, NULL);
         pValue[dwBytes] = 0;
         sptr = strchr(pValue, '\n');
         if (sptr != NULL)
            *sptr = 0;
         iStatus = SYSINFO_RC_SUCCESS;
      }
      else
      {
         WriteLog(MSG_CREATE_PROCESS_FAILED, EVENTLOG_ERROR_TYPE, "se", pszCmdLine, GetLastError());
         iStatus = SYSINFO_RC_ERROR;
      }

      // Remove temporary file
      CloseHandle(hOutput);
      DeleteFile(szTempFile);
   }
   else
   {
      WriteLog(MSG_CREATE_TMP_FILE_FAILED, EVENTLOG_ERROR_TYPE, "e", GetLastError());
      iStatus = SYSINFO_RC_ERROR;
   }
#elif defined(_NETWARE)
   /* TODO: add NetWare code here */
#else // UNIX
   iStatus = SYSINFO_RC_ERROR;
	{
		FILE *hPipe;
		if ((hPipe = popen(pszCmdLine, "r")) != NULL)
		{
			char *pTmp;

			fread(pValue, 1, MAX_RESULT_LENGTH - 1, hPipe);
			fclose(hPipe);
			if ((pTmp = strchr(pValue, '\n')) != NULL)
			{
				*pTmp = 0;
			}
			iStatus = SYSINFO_RC_SUCCESS;
		}
		else
		{
         WriteLog(MSG_CREATE_PROCESS_FAILED, EVENTLOG_ERROR_TYPE, "se",
					pszCmdLine, errno);
         iStatus = SYSINFO_RC_ERROR;
		}
	}
//EXEC::popen
#endif

   free(pszCmdLine);
   return iStatus;
}
