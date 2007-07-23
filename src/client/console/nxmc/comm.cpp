/* 
** NetXMS - Network Management System
** Portable management console
** Copyright (C) 2007 Victor Kirhenshtein
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
** File: comm.cpp
**
**/

#include "nxmc.h"
#include "logindlg.h"


//
// Login data
//

struct LOGIN_DATA
{
	DWORD flags;
	int objectCacheMode;		// 0 = disabled, 1 = clear current, 2 = use existing
	TCHAR server[MAX_DB_STRING];
	TCHAR login[MAX_DB_STRING];
	TCHAR password[MAX_DB_STRING];
	nxBusyDialog *dlg;
};


//
// Client library event handler
//

static void ClientEventHandler(NXC_SESSION session, DWORD event, DWORD code, void *arg)
{
	switch(event)
	{
		case NXC_EVENT_NOTIFICATION:
			switch(code)
			{
				case NX_NOTIFY_NEW_ALARM:
				case NX_NOTIFY_ALARM_DELETED:
				case NX_NOTIFY_ALARM_CHANGED:
				case NX_NOTIFY_ALARM_TERMINATED:
					NXMCUpdateAlarms(code, (NXC_ALARM *)arg);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}


//
// Login thread
//

static THREAD_RESULT THREAD_CALL LoginThread(void *arg)
{
	LOGIN_DATA *data = ((LOGIN_DATA *)arg);
	DWORD rcc;

	rcc = NXCConnect(data->flags, data->server, data->login,
	                 data->password, 0, NULL, NULL, &g_hSession,
	                 _T("NetXMS Console/") NETXMS_VERSION_STRING, NULL);

	// Synchronize objects
	if (rcc == RCC_SUCCESS)
	{
		BYTE serverId[8];
		TCHAR serverIdAsText[32];
		wxString cacheFile;

		NXCSetEventHandler(g_hSession, ClientEventHandler);

		data->dlg->SetStatusText(_T("Synchronizing objects..."));
		if (data->objectCacheMode != 0)
		{
			NXCGetServerID(g_hSession, serverId);
			BinToStr(serverId, 8, serverIdAsText);
			cacheFile = wxStandardPaths::Get().GetUserDataDir();
			cacheFile += FS_PATH_SEPARATOR;
			cacheFile += _T("cache.");
			cacheFile += serverIdAsText;
			cacheFile += _T(".");
			cacheFile += data->login;
			if (data->objectCacheMode == 1)
				wxRemoveFile(cacheFile);
			wxLogDebug(_T("Using object cache file: %s"), cacheFile.c_str());
		}
      rcc = NXCSubscribe(g_hSession, NXC_CHANNEL_OBJECTS);
      if (rcc == RCC_SUCCESS)
         rcc = NXCSyncObjectsEx(g_hSession, (data->objectCacheMode != 0) ? cacheFile.c_str() : NULL, TRUE);
	}

   if (rcc == RCC_SUCCESS)
   {
      data->dlg->SetStatusText(_T("Loading user database..."));
      rcc = NXCLoadUserDB(g_hSession);
   }

   if (rcc == RCC_SUCCESS)
   {
      DWORD dwServerTS, dwLocalTS;
      wxString mibFile;
      BOOL bNeedDownload;

      data->dlg->SetStatusText(_T("Loading and initializing MIB files..."));
      mibFile = wxStandardPaths::Get().GetUserDataDir();
      mibFile += FS_PATH_SEPARATOR;
      mibFile += _T("netxms.mib");
      if (SNMPGetMIBTreeTimestamp(mibFile.c_str(), &dwLocalTS) == SNMP_ERR_SUCCESS)
      {
         if (NXCGetMIBFileTimeStamp(g_hSession, &dwServerTS) == RCC_SUCCESS)
         {
            bNeedDownload = (dwServerTS > dwLocalTS);
         }
         else
         {
            bNeedDownload = FALSE;
         }
      }
      else
      {
         bNeedDownload = TRUE;
      }

      if (bNeedDownload)
      {
         rcc = NXCDownloadMIBFile(g_hSession, mibFile.c_str());
         if (rcc != RCC_SUCCESS)
         {
            wxGetApp().ShowClientError(rcc, _T("Error downloading MIB file from server: %s"));
            rcc = RCC_SUCCESS;
         }
      }

      if (rcc == RCC_SUCCESS)
      {
         if (SNMPLoadMIBTree(mibFile.c_str(), &g_mibRoot) == SNMP_ERR_SUCCESS)
         {
            g_mibRoot->SetName(_T("[root]"));
         }
         else
         {
            g_mibRoot = new SNMP_MIBObject(0, _T("[root]"));
         }
      }
   }

   if (rcc == RCC_SUCCESS)
   {
      data->dlg->SetStatusText(_T("Loading event information..."));
      rcc = NXCLoadEventDB(g_hSession);
   }

	// Synchronizing alarms
	if (rcc == RCC_SUCCESS)
	{
		NXC_ALARM *list;
		DWORD count;

		data->dlg->SetStatusText(_T("Synchronizing alarms..."));
		rcc = NXCLoadAllAlarms(g_hSession, FALSE, &count, &list);
		if (rcc == RCC_SUCCESS)
		{
			NXMCInitAlarms(count, list);
			safe_free(list);
			rcc = NXCSubscribe(g_hSession, NXC_CHANNEL_ALARMS);
		}
	}

	// Disconnect if some of post-login operations was failed
	if (rcc != RCC_SUCCESS)
	{
		NXCDisconnect(g_hSession);
		g_hSession = NULL;
	}

	data->dlg->ReportCompletion(rcc);
	return THREAD_OK;
}


//
// Do login
//

DWORD DoLogin(nxLoginDialog &dlgLogin)
{
	LOGIN_DATA data;
	nxBusyDialog dlgWait(wxGetApp().GetMainFrame(), _T("Connecting to server..."));

	data.flags = 0;
	if (dlgLogin.m_isMatchVersion)
		data.flags |= NXCF_EXACT_VERSION_MATCH;
	if (dlgLogin.m_isEncrypt)
		data.flags |= NXCF_ENCRYPT;
	if (dlgLogin.m_isCacheDisabled)
	{
		data.objectCacheMode = 0;
	}
	else if (dlgLogin.m_isClearCache)
	{
		data.objectCacheMode = 1;
	}
	else
	{
		data.objectCacheMode = 2;
	}
	nx_strncpy(data.server, dlgLogin.m_server, MAX_DB_STRING);
	nx_strncpy(data.login, dlgLogin.m_login, MAX_DB_STRING);
	nx_strncpy(data.password, dlgLogin.m_password, MAX_DB_STRING);
	data.dlg = &dlgWait;
	wxLogDebug(_T("Attempting to establish connection with server: HOST=%s LOGIN=%s PASSWD=%s FLAGS=%04X"),
	           data.server, data.login, data.password, data.flags);
	ThreadCreate(LoginThread, 0, &data);
	dlgWait.ShowModal();
	return dlgWait.m_rcc;
}
