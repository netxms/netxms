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
   TCHAR name[MAX_PATH], errorText[256];

   ConfigReadStr(_T("HelpDeskLink"), name, MAX_PATH, _T("none"));
   if ((name[0] == 0) || !_tcsicmp(name, _T("none")))
   {
      DbgPrintf(2, _T("Helpdesk link disabled"));
      return;
   }

#if !defined(_WIN32) && !defined(_NETWARE)
   TCHAR fullName[MAX_PATH];

   if (_tcschr(name, _T('/')) == NULL)
   {
      // Assume that module name without path given
      // Try to load it from pkglibdir
      const TCHAR *homeDir = _tgetenv(_T("NETXMS_HOME"));
      if (homeDir != NULL)
      {
         _sntprintf(fullName, MAX_PATH, _T("%s/lib/netxms/%s"), homeDir, name);
      }
      else
      {
         _sntprintf(fullName, MAX_PATH, _T("%s/%s"), PKGLIBDIR, name);
      }
   }
   else
   {
      nx_strncpy(fullName, name, MAX_PATH);
   }
   HMODULE hModule = DLOpen(fullName, errorText);
#else
   HMODULE hModule = DLOpen(name, errorText);
#endif

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
					   nxlog_write(MSG_HDLINK_LOADED, EVENTLOG_INFORMATION_TYPE, "ss", s_link->getName(), s_link->getVersion());
                  g_flags |= AF_HELPDESK_LINK_ACTIVE;
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
					nxlog_write(MSG_HDLINK_INIT_FAILED, EVENTLOG_ERROR_TYPE, "s", name);
					DLClose(hModule);
				}
         }
         else
         {
            nxlog_write(MSG_HDLINK_API_VERSION_MISMATCH, EVENTLOG_ERROR_TYPE, "sdd", name, NDDRV_API_VERSION, *apiVersion);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write(MSG_NO_HDLINK_ENTRY_POINT, EVENTLOG_ERROR_TYPE, "s", name);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write(MSG_DLOPEN_FAILED, EVENTLOG_ERROR_TYPE, "ss", name, errorText);
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

/**
 * Add comment to helpdesk issue
 */
UINT32 AddHelpdeskIssueComment(const TCHAR *hdref, const TCHAR *text)
{
   if (s_link == NULL)
      return RCC_NO_HDLINK;

   return s_link->addComment(hdref, text);
}

/**
 * Create helpdesk issue
 */
UINT32 GetHelpdeskIssueUrl(const TCHAR *hdref, TCHAR *url, size_t size)
{
   if (s_link == NULL)
      return RCC_NO_HDLINK;

   return s_link->getIssueUrl(hdref, url, size) ? RCC_SUCCESS : RCC_HDLINK_INTERNAL_ERROR;
}
