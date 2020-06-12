/*
** NetXMS - Network Management System
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

#ifndef _WIN32
   TCHAR fullName[MAX_PATH];

   if (_tcschr(name, _T('/')) == NULL)
   {
      // Assume that module name without path given
      // Try to load it from pkglibdir
      TCHAR libdir[MAX_PATH];
      GetNetXMSDirectory(nxDirLib, libdir);
      _sntprintf(fullName, MAX_PATH, _T("%s/%s"), libdir, name);
   }
   else
   {
      _tcslcpy(fullName, name, MAX_PATH);
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
					   nxlog_write(NXLOG_INFO, _T("Helpdesk link module %s (version %s) loaded successfully"), s_link->getName(), s_link->getVersion());
                  g_flags |= AF_HELPDESK_LINK_ACTIVE;
               }
				   else
				   {
					   nxlog_write(NXLOG_ERROR, _T("Initialization of helpdesk link module %s failed"), s_link->getName());
                  delete_and_null(s_link);
					   DLClose(hModule);
				   }
				}
				else
				{
               nxlog_write(NXLOG_ERROR, _T("Initialization of helpdesk link module \"%s\" failed"), name);
					DLClose(hModule);
				}
         }
         else
         {
            nxlog_write(NXLOG_ERROR, _T("Helpdesk link module \"%s\" cannot be loaded because of API version mismatch (module: %d; server: %d)"),
                     name, *apiVersion, NDDRV_API_VERSION);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write(NXLOG_ERROR, _T("Unable to find entry point in helpdesk link module \"%s\""), name);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write(NXLOG_ERROR, _T("Unable to load module \"%s\" (%s)"), name, errorText);
   }
}

/**
 * Create helpdesk issue
 */
uint32_t CreateHelpdeskIssue(const TCHAR *description, TCHAR *hdref)
{
   if (s_link == nullptr)
      return RCC_NO_HDLINK;

   return s_link->openIssue(description, hdref);
}

/**
 * Add comment to helpdesk issue
 */
uint32_t AddHelpdeskIssueComment(const TCHAR *hdref, const TCHAR *text)
{
   if (s_link == nullptr)
      return RCC_NO_HDLINK;

   return s_link->addComment(hdref, text);
}

/**
 * Create helpdesk issue
 */
uint32_t GetHelpdeskIssueUrl(const TCHAR *hdref, TCHAR *url, size_t size)
{
   if (s_link == nullptr)
      return RCC_NO_HDLINK;

   return s_link->getIssueUrl(hdref, url, size) ? RCC_SUCCESS : RCC_HDLINK_INTERNAL_ERROR;
}
