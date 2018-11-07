/*
** NetXMS subagent for GNU/Linux
** Copyright (C) 2013 Victor Kirhenshtein
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

#include "freebsd_subagent.h"

/**
 * Handler for System.InstalledProducts table
 */
LONG H_InstalledProducts(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   const char *command;
   if (access("/usr/sbin/pkg", X_OK) == 0)
   {
		command = "/usr/sbin/pkg query '@@@ #%n|%v||%t|%w|%e\\n'";
   }
   else
   {
  		return SYSINFO_RC_UNSUPPORTED;
   }

	FILE *pipe = popen(command, "r");
	if (pipe == NULL)
		return SYSINFO_RC_ERROR;

	value->addColumn(_T("NAME"));
	value->addColumn(_T("VERSION"));
	value->addColumn(_T("VENDOR"));
	value->addColumn(_T("DATE"));
	value->addColumn(_T("URL"));
	value->addColumn(_T("DESCRIPTION"));

	while(1)
	{
		char line[1024];
		if (fgets(line, 1024, pipe) == NULL)
			break;

		if (memcmp(line, "@@@", 3))
			continue;

		value->addRow();
		char *curr = strchr(&line[3], '#');
		if (curr != NULL)
			curr++;
		else
			curr = &line[3];
		for(int i = 0; i < 6; i++)
		{
			char *ptr = strchr(curr, '|');
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

			// Remove architecture from package name if it is the same as
			// OS architecture or package is architecture-independent
			if (i == 0)
			{
			   char *pa = strrchr(curr, ':');
			   if (pa != NULL)
			   {
			      if (!strcmp(pa, ":all") || !strcmp(pa, ":noarch") || !strcmp(pa, ":(none)") || (strstr(arch, pa) != NULL))
			         *pa = 0;
			   }
			}

#ifdef UNICODE
			value->setPreallocated(i, WideStringFromMBString(curr));
#else
			value->set(i, curr);
#endif
			if (ptr == NULL)
				break;
			curr = ptr + 1;
		}
	}
	pclose(pipe);

	return SYSINFO_RC_SUCCESS;
}
