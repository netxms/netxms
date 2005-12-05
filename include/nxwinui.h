/* 
** NetXMS - Network Management System
** Windows UI Library
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
** $module: nxwinui.h
**
**/

#ifndef _nxwinui_h_
#define _nxwinui_h_

#ifdef NXUILIB_EXPORTS
#define NXUILIB_EXPORTABLE __declspec(dllexport)
#else
#define NXUILIB_EXPORTABLE __declspec(dllimport)
#endif


//
// Alarm sounds configuration
//

typedef struct
{
   BYTE nSoundType;
   BYTE nFlags;
   TCHAR szSound1[MAX_PATH];
   TCHAR szSound2[MAX_PATH];
} ALARM_SOUND_CFG;


//
// Alarm sound configuration flags and sound types
//

#define AST_NONE        0
#define AST_SOUND       1
#define AST_VOICE       2

#define ASF_INCLUDE_SEVERITY  0x01
#define ASF_INCLUDE_SOURCE    0x02
#define ASF_INCLUDE_MESSAGE   0x04
#define ASF_VOICE_ON_ACK      0x08


//
// Exportable classes
//

#include "../src/console/nxuilib/resource.h"
#include "../src/console/nxuilib/LoginDialog.h"


//
// Functions
//

void NXUILIB_EXPORTABLE EnableDlgItem(CDialog *pWnd, int nCtrl, BOOL bEnable);

void NXUILIB_EXPORTABLE SpeakerInit(void);
void NXUILIB_EXPORTABLE SpeakerShutdown(void);
BOOL NXUILIB_EXPORTABLE SpeakText(TCHAR *pszText);

BOOL NXUILIB_EXPORTABLE ConfigureAlarmSounds(ALARM_SOUND_CFG *pCfg);


#endif
