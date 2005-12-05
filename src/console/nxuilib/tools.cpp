/* 
** NetXMS - Network Management System
** Windows Console
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
** $module: tools.cpp
** Various tools and helper functions
**
**/

#include "stdafx.h"
#include "nxuilib.h"
#include "AlarmSoundDlg.h"


//
// Enable or disable dialog item
//

void NXUILIB_EXPORTABLE EnableDlgItem(CDialog *pWnd, int nCtrl, BOOL bEnable)
{
   CWnd *pCtrl;

   pCtrl = pWnd->GetDlgItem(nCtrl);
   if (pCtrl != NULL)
      pCtrl->EnableWindow(bEnable);
}


//
// Configure alarm sound settings
//

BOOL NXUILIB_EXPORTABLE ConfigureAlarmSounds(ALARM_SOUND_CFG *pCfg)
{
   CAlarmSoundDlg dlg;
   BOOL bResult = FALSE;

   dlg.m_iSoundType = pCfg->nSoundType;
   dlg.m_strSound1 = pCfg->szSound1;
   dlg.m_strSound2 = pCfg->szSound2;
   dlg.m_bIncludeMessage = (pCfg->nFlags & ASF_INCLUDE_MESSAGE) ? TRUE : FALSE;
   dlg.m_bIncludeSeverity = (pCfg->nFlags & ASF_INCLUDE_SEVERITY) ? TRUE : FALSE;
   dlg.m_bIncludeSource = (pCfg->nFlags & ASF_INCLUDE_SOURCE) ? TRUE : FALSE;
   dlg.m_bNotifyOnAck = (pCfg->nFlags & ASF_VOICE_ON_ACK) ? TRUE : FALSE;
   if (dlg.DoModal() == IDOK)
   {
      pCfg->nSoundType = dlg.m_iSoundType;
      nx_strncpy(pCfg->szSound1, (LPCTSTR)dlg.m_strSound1, MAX_PATH);
      nx_strncpy(pCfg->szSound2, (LPCTSTR)dlg.m_strSound2, MAX_PATH);
      pCfg->nFlags = 0;
      if (dlg.m_bIncludeMessage)
         pCfg->nFlags |= ASF_INCLUDE_MESSAGE;
      if (dlg.m_bIncludeSeverity)
         pCfg->nFlags |= ASF_INCLUDE_SEVERITY;
      if (dlg.m_bIncludeSource)
         pCfg->nFlags |= ASF_INCLUDE_SOURCE;
      if (dlg.m_bNotifyOnAck)
         pCfg->nFlags |= ASF_VOICE_ON_ACK;
      bResult = TRUE;
   }
   return bResult;
}


//
// Save alarm sound configuration to registry
//

BOOL NXUILIB_EXPORTABLE SaveAlarmSoundCfg(ALARM_SOUND_CFG *pCfg, TCHAR *pszKey)
{
   HKEY hKey;
   DWORD dwTemp;

   if (RegCreateKeyEx(HKEY_CURRENT_USER, pszKey, 0, NULL, 
                      REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                      NULL, &hKey, &dwTemp) != ERROR_SUCCESS)
      return FALSE;

   dwTemp = pCfg->nSoundType;
   RegSetValueEx(hKey, _T("SoundType"), 0, REG_DWORD, (BYTE *)&dwTemp, sizeof(DWORD));
   dwTemp = pCfg->nFlags;
   RegSetValueEx(hKey, _T("Flags"), 0, REG_DWORD, (BYTE *)&dwTemp, sizeof(DWORD));
   RegSetValueEx(hKey, _T("Sound1"), 0, REG_EXPAND_SZ, (BYTE *)pCfg->szSound1,
                 _tcslen(pCfg->szSound1) * sizeof(TCHAR) + sizeof(TCHAR));
   RegSetValueEx(hKey, _T("Sound2"), 0, REG_EXPAND_SZ, (BYTE *)pCfg->szSound2,
                 _tcslen(pCfg->szSound2) * sizeof(TCHAR) + sizeof(TCHAR));

   RegCloseKey(hKey);
   return TRUE;
}


//
// Load alarm sound configuration from registry
//

BOOL NXUILIB_EXPORTABLE LoadAlarmSoundCfg(ALARM_SOUND_CFG *pCfg, TCHAR *pszKey)
{
   HKEY hKey;
   DWORD dwTemp, dwSize;

   if (RegOpenKeyEx(HKEY_CURRENT_USER, pszKey, 0,
                    KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
      return FALSE;

   memset(pCfg, 0, sizeof(ALARM_SOUND_CFG));

   dwSize = sizeof(DWORD);
   RegQueryValueEx(hKey, _T("SoundType"), NULL, NULL, (BYTE *)&dwTemp, &dwSize);
   pCfg->nSoundType = (BYTE)dwTemp;

   dwSize = sizeof(DWORD);
   RegQueryValueEx(hKey, _T("Flags"), NULL, NULL, (BYTE *)&dwTemp, &dwSize);
   pCfg->nFlags = (BYTE)dwTemp;

   dwSize = 256 * sizeof(TCHAR);
   RegQueryValueEx(hKey, _T("Sound1"), NULL, NULL, (BYTE *)pCfg->szSound1, &dwSize);

   dwSize = 256 * sizeof(TCHAR);
   RegQueryValueEx(hKey, _T("Sound2"), NULL, NULL, (BYTE *)pCfg->szSound2, &dwSize);

   RegCloseKey(hKey);

   if (pCfg->nSoundType > 2)
      pCfg->nSoundType = 0;
   return TRUE;
}


//
// Play alarm sound according to configuration
//

void NXUILIB_EXPORTABLE PlayAlarmSound(NXC_ALARM *pAlarm, BOOL bNewAlarm,
                                       NXC_SESSION hSession, ALARM_SOUND_CFG *pCfg)
{
   NXC_OBJECT *pObject;
   TCHAR *pszSound, szText[1024];
   int i;
   static TCHAR *m_szAlarmText[] = { _T("INFORMATIONAL ALARM. "), _T("WARNING ALARM. "),
                                     _T("MINOR ALARM. "), _T("MAJOR ALARM. "),
                                     _T("CRITICAL ALARM. ") };

   switch(pCfg->nSoundType)
   {
      case AST_NONE:
         break;
      case AST_SOUND:
         pszSound = bNewAlarm ? pCfg->szSound1 : pCfg->szSound2;
         if (_tcsicmp(pszSound, _T("<none>")))
         {
            for(i = 0; g_pszSoundNames[i] != NULL; i++)
               if (!_tcsicmp(pszSound, g_pszSoundNames[i]))
                  break;
            if (g_pszSoundNames[i] != NULL)
            {
               PlaySound(MAKEINTRESOURCE(g_nSoundId[i]), g_hInstance,
                         SND_ASYNC | SND_NODEFAULT | SND_RESOURCE);
            }
            else
            {
               PlaySound(pszSound, NULL, SND_ASYNC | SND_NODEFAULT | SND_FILENAME);
            }
         }
         break;
      case AST_VOICE:
         if (bNewAlarm)
         {
            pObject = NXCFindObjectById(hSession, pAlarm->dwSourceObject);
            _sntprintf(szText, 1024, _T("%s%s%s%s"),
                       (pCfg->nFlags & ASF_INCLUDE_SEVERITY) ? 
                              m_szAlarmText[pAlarm->wSeverity] : _T(""),
                       (pCfg->nFlags & ASF_INCLUDE_SOURCE) ?
                              ((pObject != NULL) ? 
                                    pObject->szName : _T("unknown node")) : _T(""),
                       (pCfg->nFlags & ASF_INCLUDE_SOURCE) ? _T(": ") : _T(""),
                       (pCfg->nFlags & ASF_INCLUDE_MESSAGE) ? pAlarm->szMessage : _T(""));
         }
         else
         {
            if (pCfg->nFlags & ASF_VOICE_ON_ACK)
               _tcscpy(szText, _T("ALARM ACKNOWLEDGED"));
            else
               szText[0] = 0;
         }
         if (szText[0] != 0)
            SpeakText(szText);
         break;
      default:
         break;
   }
}
