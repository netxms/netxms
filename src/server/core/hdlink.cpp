/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: hdlink.cpp
**
**/

#include "nxcore.h"
#include <hdlink.h>

/**
 * Help desk link object
 */
static HelpDeskLink *s_link = NULL;

/**
 * Load helpdesk link module
 */
void LoadHelpDeskLink()
{
   TCHAR file[MAX_PATH], errorText[256];

   ConfigReadStr(_T("HelpDeskLink"), file, MAX_PATH, _T("none"));
   if ((file[0] == 0) || !_tcsicmp(file, _T("none")))
   {
      DbgPrintf(2, _T("Helpdesk link disabled"));
      return;
   }

   HMODULE hModule = DLOpen(file, errorText);
   if (hModule != NULL)
   {
		int *apiVersion = (int *)DLGetSymbolAddr(hModule, "hdlinkAPIVersion", errorText);
      HelpDeskLink *(* CreateInstance)() = (HelpDeskLink *(*)())DLGetSymbolAddr(hModule, "hdlinkCreateInstance", errorText);

      if ((apiVersion != NULL) && (CreateInstance != NULL))
      {
         if (*apiVersion == HDLINK_API_VERSION)
         {
				s_link = CreateInstance();
				if (s_link != NULL)
				{
               if (s_link->init())
               {
					   nxlog_write(MSG_HDLINK_LOADED, EVENTLOG_INFORMATION_TYPE, "s", s_link->getName());
               }
				   else
				   {
					   nxlog_write(MSG_HDLINK_INIT_FAILED, EVENTLOG_ERROR_TYPE, "s", s_link->getName());
                  delete_and_null(s_link);
					   DLClose(hModule);
				   }
				}
				else
				{
					nxlog_write(MSG_HDLINK_INIT_FAILED, EVENTLOG_ERROR_TYPE, "s", file);
					DLClose(hModule);
				}
         }
         else
         {
            nxlog_write(MSG_HDLINK_API_VERSION_MISMATCH, EVENTLOG_ERROR_TYPE, "sdd", file, NDDRV_API_VERSION, *apiVersion);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write(MSG_NO_HDLINK_ENTRY_POINT, EVENTLOG_ERROR_TYPE, "s", file);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write(MSG_DLOPEN_FAILED, EVENTLOG_ERROR_TYPE, "ss", file, errorText);
   }
}

/**
 * Create helpdesk issue
 */
UINT32 CreateHelpdeskIssue(const TCHAR *description, TCHAR *hdref)
{
   if (s_link == NULL)
      return RCC_NO_HDLINK;

   return s_link->openIssue(description, hdref);
}
