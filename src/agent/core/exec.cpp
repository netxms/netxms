/* $Id: exec.cpp,v 1.18 2007-04-19 05:38:21 victor Exp $ */

/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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
** File: exec.cpp
**
**/

#include "nxagentd.h"

#ifdef _WIN32
#define popen _popen
#else
#include <sys/wait.h>
#endif


//
// Execute external command
//

#ifndef _WIN32 // unix-only hack
static THREAD_RESULT THREAD_CALL Waiter(void *arg)
{
	pid_t pid = *((pid_t *)arg);
	waitpid(pid, NULL, 0);
	free(arg);
	return THREAD_OK;
}

static THREAD_RESULT THREAD_CALL Worker(void *arg)
{ 
	char *cmd = (char *)arg;
	if (cmd == NULL)
	{
		return THREAD_OK;
	}

	char *pCmd[128];
	char *pTmp = cmd;
	int state = 0;
	int nCount = 0;

	pCmd[nCount++] = pTmp;
	int nLen = strlen(pTmp);
	for (int i = 0; i < nLen; i++)
	{
		switch(pTmp[i])
		{
			case ' ':
				if (state == 0)
				{
					pTmp[i] = 0;
					if (pTmp[i + 1] != 0)
					{
						pCmd[nCount++] = pTmp + i + 1;
					}
				}
				break;
			case '"':
				state == 0 ? state++ : state--;

				memmove(pTmp + i, pTmp + i + 1, nLen - i);
				i--;
				break;
			case '\\':
				if (pTmp[i+1] == '"')
				{
					memmove(pTmp + i, pTmp + i + 1, nLen - i);
				}
				break;
			default:
				break;
		}
	}
	pCmd[nCount] = NULL;

	// DO EXEC
	pid_t p;
	p = fork();
	switch(p)
	{
		case -1: // error
			break;
		case 0: // child
			{
				int sd = open("/dev/null", O_RDWR);
				if (sd == -1)
				{
					sd = open("/dev/null", O_RDONLY);
				}
				close(0); close(1); close(2);
				dup2(sd, 0); dup2(sd, 1); dup2(sd, 2);
				close(sd);
				execv(pCmd[0], pCmd);
				_exit(127);
			}
			break;
		default: // parent
			{
				pid_t *pp = (pid_t *)malloc(sizeof(pid_t));
				*pp = p;
				ThreadCreate(Waiter, 0, (void *)pp);
			}
			break;
	}

	free(cmd);

	return THREAD_OK;
}
#endif


DWORD ExecuteCommand(char *pszCommand, NETXMS_VALUES_LIST *pArgs)
{
   char *pszCmdLine, *sptr;
   DWORD i, dwSize, dwRetCode = ERR_SUCCESS;

   DebugPrintf(INVALID_INDEX, "EXEC: Expanding command \"%s\"", pszCommand);

   // Substitute $1 .. $9 with actual arguments
   if (pArgs != NULL)
   {
      dwSize = (DWORD)strlen(pszCommand) + 1;
      pszCmdLine = (char *)malloc(dwSize);
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
						iArgLength = (int)strlen(pArgs->ppStringList[dwArg]);
						dwSize += iArgLength;
						pszCmdLine = (char *)realloc(pszCmdLine, dwSize);
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

   DebugPrintf(INVALID_INDEX, "EXEC: Executing \"%s\"", pszCmdLine);
#if defined(_WIN32)
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   // Fill in process startup info structure
   memset(&si, 0, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   si.dwFlags = 0;

   // Create new process
   if (!CreateProcess(NULL, pszCmdLine, NULL, NULL, FALSE, 
                      CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
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
	if (!ThreadCreate(Worker, 0, (void *)strdup(pszCmdLine)))
	{
   	dwRetCode = ERR_EXEC_FAILED;
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
   iSize = (int)strlen(pszArg);
   pszCmdLine = (char *)malloc(iSize);
   for(sptr = &pszArg[1], i = 0; *sptr != 0; sptr++)
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
               iArgLength = (int)strlen(szBuffer);
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
	if (*pszArg == 'E')
	{
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
									CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
			{
				// Wait for process termination and close all handles
				if (WaitForSingleObject(pi.hProcess, g_dwExecTimeout) == WAIT_OBJECT_0)
				{
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
					// Timeout waiting for external process to complete, kill it
					TerminateProcess(pi.hProcess, 127);
					WriteLog(MSG_PROCESS_KILLED, EVENTLOG_WARNING_TYPE, "s", pszCmdLine);
					iStatus = SYSINFO_RC_ERROR;
				}

				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
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
	}
	else
	{
#endif

#ifdef _NETWARE
   /* TODO: add NetWare code here */
	iStatus = SYSINFO_RC_UNSUPPORTED;
#else // UNIX or Windows
		iStatus = SYSINFO_RC_ERROR;
		{
			FILE *hPipe;

			if ((hPipe = popen(pszCmdLine, "r")) != NULL)
			{
				char *pTmp;
				int nRet;

				nRet = fread(pValue, 1, MAX_RESULT_LENGTH - 1, hPipe);
				fclose(hPipe);
				if (nRet > 0)
				{
					pValue[MAX_RESULT_LENGTH - 1] = 0;
					if ((pTmp = strchr(pValue, '\n')) != NULL)
					{
						*pTmp = 0;
					}
					iStatus = SYSINFO_RC_SUCCESS;
				}
				else
				{
					iStatus = SYSINFO_RC_ERROR;
				}
			}
			else
			{
				WriteLog(MSG_CREATE_PROCESS_FAILED, EVENTLOG_ERROR_TYPE, "se",
				         pszCmdLine, errno);
				iStatus = SYSINFO_RC_ERROR;
			}
		}
#endif

#ifdef _WIN32
	}
#endif

   free(pszCmdLine);
   return iStatus;
}


//
// Execute external command via shell
//

DWORD ExecuteShellCommand(char *pszCommand, NETXMS_VALUES_LIST *pArgs)
{
   char *pszCmdLine, *sptr;
   DWORD i, dwSize, dwRetCode = ERR_SUCCESS;

   DebugPrintf(INVALID_INDEX, "SH_EXEC: Expanding command \"%s\"", pszCommand);

   // Substitute $1 .. $9 with actual arguments
   if (pArgs != NULL)
   {
      dwSize = (DWORD)strlen(pszCommand) + 1;
      pszCmdLine = (char *)malloc(dwSize);
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
                  iArgLength = (int)strlen(pArgs->ppStringList[dwArg]);
                  dwSize += iArgLength;
                  pszCmdLine = (char *)realloc(pszCmdLine, dwSize);
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

   DebugPrintf(INVALID_INDEX, "SH_EXEC: Executing \"%s\"", pszCmdLine);

   if (system(pszCmdLine) == 0)
      dwRetCode = ERR_SUCCESS;
   else
      dwRetCode = ERR_EXEC_FAILED;

   // Cleanup
   if (pArgs != NULL)
      free(pszCmdLine);

   return dwRetCode;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.17  2007/04/19 05:24:25  victor
Minor changes

Revision 1.16  2007/04/18 21:13:40  alk
exec scheme changed


*/
