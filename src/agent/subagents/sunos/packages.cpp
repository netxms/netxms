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

#include "sunos_subagent.h"
#include <netxms-regex.h>

struct NameToColumn
{
   int column;
   const char *name;
   const int size;
};

static NameToColumn s_mapping[] = {
   { 0, "PKGINST", 7 },
   { 1, "VERSION", 7 },
   { 2, "VENDOR", 6 },
   { 3, "INSTDATE", 8 },
   { 4, "HOTLINE", 7 },
   { 5, "NAME", 4 },
   { 0, NULL, 0 } 
};

/**
 * Handler for System.InstalledProducts table
 */
LONG H_InstalledProducts(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   if (access("/usr/bin/pkginfo", X_OK) != 0)
      return SYSINFO_RC_UNSUPPORTED;

   FILE *pipe = popen("/usr/bin/pkginfo -l", "r");
   if (pipe == nullptr)
      return SYSINFO_RC_ERROR;

   const char *errptr;
   int erroffset;
   pcre *preg = pcre_compile("[[:blank:]]*([\\w]+):[[:blank:]]*(.*)", PCRE_COMMON_FLAGS_A | PCRE_CASELESS, &errptr, &erroffset, NULL);
   if (preg == nullptr)
   {
      pclose(pipe);
      return SYSINFO_RC_ERROR;
   }

   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("VERSION"), DCI_DT_STRING, _T("Version"), true);
   value->addColumn(_T("VENDOR"), DCI_DT_STRING, _T("Vendor"));
   value->addColumn(_T("DATE"), DCI_DT_STRING, _T("Install Date"));
   value->addColumn(_T("URL"), DCI_DT_STRING, _T("URL"));
   value->addColumn(_T("DESCRIPTION"), DCI_DT_STRING, _T("Description"));

   bool newEntry = true;
   while(true)
   {
      char line[1024];
      if (fgets(line, 1024, pipe) == nullptr)
         break;
      line[strlen(line)-1] = 0;

      if (!strcmp("", line)) //read one package full info
      {
         newEntry = true;
         continue;
      }

      if(newEntry)
      {
         newEntry = false;
         value->addRow();
      }

      int fields[12];
      if (pcre_exec(preg, NULL, line, static_cast<int>(strlen(line)), 0, 0, fields, 12) < 0) // Skip unknown line format
         continue;

      for(int i = 0; s_mapping[i].name != NULL; i++)
         if(!strncmp(s_mapping[i].name, line+fields[2], s_mapping[i].size))
         {
            value->set(s_mapping[i].column, line+fields[4]);
            break;
         }
   }
   pcre_free(preg);
   pclose(pipe);

   return SYSINFO_RC_SUCCESS;
}
