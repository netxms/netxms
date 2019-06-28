/*
** NetXMS subagent for AIX
** Copyright (C) 2003-2019 Victor Kirhenshtein
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

#include "aix_subagent.h"

/**
 * Handler for System.InstalledProducts table
 */
LONG H_InstalledProducts(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   if (access("/usr/bin/lslpp", X_OK) != 0)
      return SYSINFO_RC_UNSUPPORTED;

   FILE *pipe = popen("/usr/bin/lslpp -Lqc", "r");
   if (pipe == NULL)
      return SYSINFO_RC_ERROR;

   value->addColumn(_T("NAME"));
   value->addColumn(_T("VERSION"));
   value->addColumn(_T("VENDOR"));
   value->addColumn(_T("DATE"));
   value->addColumn(_T("URL"));
   value->addColumn(_T("DESCRIPTION"));

   static int columnMapping[8] = { 0, -1, 1, -1, -1, -1, -1, 5 };
   while(1)
   {
      char line[1024];
      if (fgets(line, 1024, pipe) == NULL)
         break;

      char *curr = line;
		for(int i = 0; i < 8; i++)
		{
			char *ptr = strchr(curr, ':');
			if (ptr != NULL)
			{
				*ptr = 0;
			}
			else
			{
				ptr = strchr(curr, '\n');
				if (ptr != NULL)
				{
					*ptr = 0;
					ptr = NULL;
				}
			}

         if (columnMapping[i] != -1)
            value->set(columnMapping[i], curr);

         if (ptr == NULL)
            break;
         curr = ptr + 1;
		}
	}
	pclose(pipe);

	return SYSINFO_RC_SUCCESS;
}
