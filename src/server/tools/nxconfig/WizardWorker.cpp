/* 
** NetXMS - Network Management System
** Server Configurator for Windows
** Copyright (C) 2005 Victor Kirhenshtein
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
** $module: WizardWorker.cpp
** Worker thread for configuration wizard
**
**/

#include "stdafx.h"
#include "nxconfig.h"


//
// Static data
//

static HWND m_hStatusWnd = NULL;


//
// Create configuration file
//

static BOOL CreateConfigFile(WIZARD_CFG_INFO *pc)
{
   BOOL bResult = FALSE;

   PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Creating configuration file"));

   Sleep(5000);
   bResult = TRUE;

   PostMessage(m_hStatusWnd, WM_STAGE_COMPLETED, bResult, 0);
   return bResult;
}


//
// Worker thread
//

static DWORD __stdcall WorkerThread(void *pArg)
{
   WIZARD_CFG_INFO *pc = (WIZARD_CFG_INFO *)pArg;
   BOOL bResult;

   bResult = CreateConfigFile(pc);

   // Load database driver
   if (bResult)
   {
      PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Loading database driver"));
      _tcsncpy(g_szDbDriver, pc->m_szDBDriver, MAX_PATH);
      _tcsncpy(g_szDbServer, pc->m_szDBServer, MAX_PATH);
      bResult = DBInit(FALSE, FALSE, FALSE);
      PostMessage(m_hStatusWnd, WM_STAGE_COMPLETED, bResult, 0);
   }

   // Create database if requested

   // Notify UI that job is finished
   PostMessage(m_hStatusWnd, WM_JOB_FINISHED, bResult, 0);
   return 0;
}


//
// Worker thread starter
//

void StartWizardWorkerThread(HWND hWnd, WIZARD_CFG_INFO *pc)
{
   HANDLE hThread;
   DWORD dwThreadId;

   m_hStatusWnd = hWnd;
   hThread = CreateThread(NULL, 0, WorkerThread, pc, 0, &dwThreadId);
   if (hThread != NULL)
      CloseHandle(hThread);
}
