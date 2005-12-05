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
}
