/* 
** NetXMS - Network Management System
** Command line client
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: cmdline.cpp
**
**/

#include "nxcmd.h"
#include <string.h>


//
// Show command line prompt
// IBM mainframe style :)
//

static void Prompt(DWORD dwLastError)
{
   if (dwLastError == RCC_SUCCESS)
      printf("Ready;\n");
   else
      printf("Ready(%d); %s\n", dwLastError, NXCGetErrorText(dwLastError));
}


//
// Process command entered from command line in standalone mode
// Return TRUE if command was "down"
//

static BOOL ProcessCommand(char *pszCmdLine, DWORD *pdwResult)
{
   *pdwResult = RCC_SUCCESS;
   return FALSE;
}


//
// Main command receiving loop
//

void CommandLoop(void)
{
   char *ptr, szCommand[256];
   DWORD dwResult;

   // Load oblects from server
   printf("Loading objects...\n");
   dwResult = NXCSyncObjects(g_hSession);
   if (dwResult == RCC_SUCCESS)
   {
      printf("Loading user database...\n");
      dwResult = NXCLoadUserDB(g_hSession);
   }

   while(1)
   {
      Prompt(dwResult);
#if USE_READLINE
      ptr = readline("> ");
#else
      printf("> ");
      fflush(stdout);
      fgets(szCommand, 255, stdin);
      ptr = strchr(szCommand, '\n');
      if (ptr != NULL)
         *ptr = 0;
      ptr = szCommand;
#endif

      // Process Ctrl+D from readline
      if (ptr == NULL)
         break;

      if (ProcessCommand(ptr, &dwResult))
         break;

#if USE_READLINE
      add_history(ptr);
      free(ptr);
#endif

      //dwResult = RCC_SUCCESS;
   }
}
