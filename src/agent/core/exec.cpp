/* $Id$ */

/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2009 Victor Kirhenshtein
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
#include <winternl.h>
#define popen _popen
#else
#include <sys/wait.h>
#endif


//
// Information for process starter
//

struct PROCESS_START_INFO
{
	char *cmdLine;
	pid_t *pid;
	CONDITION condProcessStarted;
};


//
// Execute external command
//

#if !defined(_WIN32) && !defined(_NETWARE) /* unix-only hack */
static THREAD_RESULT THREAD_CALL Waiter(void *arg)
{
	pid_t pid = *((pid_t *)arg);
	waitpid(pid, NULL, 0);
	free(arg);
	return THREAD_OK;
}

static THREAD_RESULT THREAD_CALL Worker(void *arg)
{ 
	char *cmd = ((PROCESS_START_INFO *)arg)->cmdLine;
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
			*((PROCESS_START_INFO *)arg)->pid = -1;
			ConditionSet(((PROCESS_START_INFO *)arg)->condProcessStarted);
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
				*((PROCESS_START_INFO *)arg)->pid = p;
				ConditionSet(((PROCESS_START_INFO *)arg)->condProcessStarted);
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


DWORD ExecuteCommand(char *pszCommand, StringList *args, pid_t *pid)
{
   char *pszCmdLine, *sptr;
   DWORD i, dwSize, dwRetCode = ERR_SUCCESS;

   DebugPrintf(INVALID_INDEX, 4, "EXEC: Expanding command \"%s\"", pszCommand);

   // Substitute $1 .. $9 with actual arguments
   if (args != NULL)
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
					int argNum = *sptr - '1';

					if (argNum < args->getSize())
					{
						int iArgLength;

						// Extend resulting line
						iArgLength = (int)strlen(args->getValue(argNum));
						dwSize += iArgLength;
						pszCmdLine = (char *)realloc(pszCmdLine, dwSize);
						strcpy(&pszCmdLine[i], args->getValue(argNum));
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

   DebugPrintf(INVALID_INDEX, 4, "EXEC: Executing \"%s\"", pszCmdLine);
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
      nxlog_write(MSG_CREATE_PROCESS_FAILED, EVENTLOG_ERROR_TYPE, "se", pszCmdLine, GetLastError());
      dwRetCode = ERR_EXEC_FAILED;
   }
   else
   {
		if (pid != NULL)
		{
			HMODULE dllKernel32;

			dllKernel32 = LoadLibrary(_T("KERNEL32.DLL"));
			if (dllKernel32 != NULL)
			{
				DWORD (WINAPI *fpGetProcessId)(HANDLE);

				fpGetProcessId = (DWORD (WINAPI *)(HANDLE))GetProcAddress(dllKernel32, "GetProcessId");
				if (fpGetProcessId != NULL)
				{
					*pid = fpGetProcessId(pi.hProcess);
				}
				else
				{
					HMODULE dllNtdll;

					dllNtdll = LoadLibrary(_T("NTDLL.DLL"));
					if (dllNtdll != NULL)
					{
						NTSTATUS (WINAPI *pfZwQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

						pfZwQueryInformationProcess = (NTSTATUS (WINAPI *)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG))GetProcAddress(dllNtdll, "ZwQueryInformationProcess");
						if (pfZwQueryInformationProcess != NULL)
						{
							PROCESS_BASIC_INFORMATION pbi;
							ULONG len;

							if (pfZwQueryInformationProcess(pi.hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), &len) == 0)
							{
								*pid = (pid_t)pbi.UniqueProcessId;
							}
						}
						FreeLibrary(dllNtdll);
					}
				}
				FreeLibrary(dllKernel32);
			}
		}

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
	PROCESS_START_INFO pi;
	pid_t tempPid;

	pi.cmdLine = strdup(pszCmdLine);
	pi.pid = (pid != NULL) ? pid : &tempPid;
	pi.condProcessStarted = ConditionCreate(TRUE);
	if (ThreadCreate(Worker, 0, &pi))
	{
		if (ConditionWait(pi.condProcessStarted, 5000))
		{
			if (*pi.pid == -1)
		   	dwRetCode = ERR_EXEC_FAILED;
		}
		else
		{
	   	dwRetCode = ERR_EXEC_FAILED;
		}
		ConditionDestroy(pi.condProcessStarted);
	}
	else
	{
   	dwRetCode = ERR_EXEC_FAILED;
	}
#endif

   // Cleanup
   if (args != NULL)
      free(pszCmdLine);

   return dwRetCode;
}


//
// Structure for passing data to popen() worker
//

struct POPEN_WORKER_DATA
{
	int status;
	char *cmdLine;
	char value[MAX_RESULT_LENGTH];
	CONDITION finished;
	CONDITION released;
};


//
// Worker thread for executing external parameter handler
//

static THREAD_RESULT THREAD_CALL POpenWorker(void *arg)
{
	FILE *hPipe;
	POPEN_WORKER_DATA *data = (POPEN_WORKER_DATA *)arg;

	if ((hPipe = popen(data->cmdLine, "r")) != NULL)
	{
		char *pTmp;
		int nRet;

		nRet = (int)fread(data->value, 1, MAX_RESULT_LENGTH - 1, hPipe);
		pclose(hPipe);
	   DebugPrintf(INVALID_INDEX, 4, "H_ExternalParameter/POpenWorker: worker thread pipe read result: %d", nRet);
		if (nRet > 0)
		{
			data->value[min(nRet, MAX_RESULT_LENGTH - 1)] = 0;
			if ((pTmp = strchr(data->value, '\n')) != NULL)
			{
				*pTmp = 0;
			}
			data->status = SYSINFO_RC_SUCCESS;
		}
		else
		{
		   DebugPrintf(INVALID_INDEX, 4, "H_ExternalParameter/POpenWorker: worker thread pipe read error: %s", strerror(errno));
			data->status = SYSINFO_RC_ERROR;
		}
	}
	else
	{
		nxlog_write(MSG_CREATE_PROCESS_FAILED, EVENTLOG_ERROR_TYPE, "se",
				   data->cmdLine, errno);
		data->status = SYSINFO_RC_ERROR;
	}

	// Notify caller that data is available and wait while caller
	// retrieves the data, then destroy object
	ConditionSet(data->finished);
	ConditionWait(data->released, INFINITE);
	ConditionDestroy(data->finished);
	ConditionDestroy(data->released);
	free(data);

	return THREAD_OK;
}


//
// Handler function for external (user-defined) parameters
//

LONG H_ExternalParameter(const char *pszCmd, const char *pszArg, char *pValue)
{
	char *pszCmdLine, szBuffer[1024], szTempFile[MAX_PATH];
	const char *sptr;
	int i, iSize, iStatus;

   DebugPrintf(INVALID_INDEX, 4, "H_ExternalParameter called for \"%s\" \"%s\"", pszCmd, pszArg);

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
            if (AgentGetParameterArg(pszCmd, *sptr - '0', szBuffer, 1024))
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
   DebugPrintf(INVALID_INDEX, 4, "H_ExternalParameter: command line is \"%s\"", pszCmdLine);

#if defined(_WIN32)
	if (*pszArg == 'E')
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		SECURITY_ATTRIBUTES sa;
		HANDLE hOutput;
		DWORD dwBytes;
		char *eptr;

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
					eptr = strchr(pValue, '\r');
					if (eptr != NULL)
						*eptr = 0;
					eptr = strchr(pValue, '\n');
					if (eptr != NULL)
						*eptr = 0;
					iStatus = SYSINFO_RC_SUCCESS;
				}
				else
				{
					// Timeout waiting for external process to complete, kill it
					TerminateProcess(pi.hProcess, 127);
					nxlog_write(MSG_PROCESS_KILLED, EVENTLOG_WARNING_TYPE, "s", pszCmdLine);
					iStatus = SYSINFO_RC_ERROR;
				}

				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
			}
			else
			{
				nxlog_write(MSG_CREATE_PROCESS_FAILED, EVENTLOG_ERROR_TYPE, "se", pszCmdLine, GetLastError());
				iStatus = SYSINFO_RC_ERROR;
			}

			// Remove temporary file
			CloseHandle(hOutput);
			DeleteFile(szTempFile);
		}
		else
		{
			nxlog_write(MSG_CREATE_TMP_FILE_FAILED, EVENTLOG_ERROR_TYPE, "e", GetLastError());
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
		{
			POPEN_WORKER_DATA *data;

			data = (POPEN_WORKER_DATA *)malloc(sizeof(POPEN_WORKER_DATA));
			data->cmdLine = pszCmdLine;
			data->finished = ConditionCreate(TRUE);
			data->released = ConditionCreate(TRUE);
			ThreadCreate(POpenWorker, 0, data);
		   DebugPrintf(INVALID_INDEX, 4, "H_ExternalParameter (shell exec): worker thread created");
			if (ConditionWait(data->finished, g_dwExecTimeout))
			{
				iStatus = data->status;
				if (iStatus == SYSINFO_RC_SUCCESS)
					strcpy(pValue, data->value);
			}
			else
			{
				// Timeout
				iStatus = SYSINFO_RC_ERROR;
			}
			ConditionSet(data->released);	// Allow worker to destroy data
		   DebugPrintf(INVALID_INDEX, 4, "H_ExternalParameter (shell exec): execution status %d", iStatus);
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

DWORD ExecuteShellCommand(char *pszCommand, StringList *args)
{
   char *pszCmdLine, *sptr;
   DWORD i, dwSize, dwRetCode = ERR_SUCCESS;

   DebugPrintf(INVALID_INDEX, 4, "SH_EXEC: Expanding command \"%s\"", pszCommand);

   // Substitute $1 .. $9 with actual arguments
   if (args != NULL)
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
               int argNum = *sptr - '1';

               if (argNum < args->getSize())
               {
                  int iArgLength;

                  // Extend resulting line
						iArgLength = (int)strlen(args->getValue(argNum));
                  dwSize += iArgLength;
                  pszCmdLine = (char *)realloc(pszCmdLine, dwSize);
                  strcpy(&pszCmdLine[i], args->getValue(argNum));
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

   DebugPrintf(INVALID_INDEX, 4, "SH_EXEC: Executing \"%s\"", pszCmdLine);

   if (system(pszCmdLine) == 0)
      dwRetCode = ERR_SUCCESS;
   else
      dwRetCode = ERR_EXEC_FAILED;

   // Cleanup
   if (args != NULL)
      free(pszCmdLine);

   return dwRetCode;
}
