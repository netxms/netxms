/*
** NetXMS subagent for AIX
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
   if (pipe == nullptr)
      return SYSINFO_RC_ERROR;

   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("VERSION"), DCI_DT_STRING, _T("Version"), true);
   value->addColumn(_T("VENDOR"), DCI_DT_STRING, _T("Vendor"));
   value->addColumn(_T("DATE"), DCI_DT_STRING, _T("Install Date"));
   value->addColumn(_T("URL"), DCI_DT_STRING, _T("URL"));
   value->addColumn(_T("DESCRIPTION"), DCI_DT_STRING, _T("Description"));

   const char *data[8];
   while(true)
   {
      char line[1024];
      if (fgets(line, 1024, pipe) == NULL)
         break;

      memset(data, 0, sizeof(data));
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
         data[i] = curr;
         if (ptr == NULL)
            break;
         curr = ptr + 1;
      }

      if ((data[7] != NULL) && (*(data[0]) != 0))
      {
         value->addRow();

         // use name if product type is "RPM" and fileset name otherwise
         value->set(0, *(data[6]) == 'R' ? data[0] : data[1]);
         value->set(1, data[2]);
         value->set(5, data[7]);
      }
   }
   pclose(pipe);

   return SYSINFO_RC_SUCCESS;
}
