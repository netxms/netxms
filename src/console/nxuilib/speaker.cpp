/* 
** NetXMS - Network Management System
** Windows Console
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
** $module: speaker.cpp
** Text to speach interface
**
**/

#include "stdafx.h"
#include "nxuilib.h"
#include <nxqueue.h>
#include <sphelper.h>


//
// Static data
//

static Queue *m_pSpeakerQueue = NULL;


//
// Speaker thread
//

static void SpeakerThread(void *pArg)
{
   TCHAR *pszText;
   ISpVoice *pVoice = NULL;
   HRESULT hRes;
#ifndef UNICODE
   WCHAR wszBuffer[4096];
#endif

   hRes =  CoInitializeEx(NULL, COINIT_MULTITHREADED); // Initialize COM.
   if (SUCCEEDED(hRes))
   {
      hRes = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL,
                              IID_ISpVoice, (void **)&pVoice);
      if (SUCCEEDED(hRes))
      {
         m_pSpeakerQueue = new Queue;
         while(1)
         {
            pszText = (TCHAR *)m_pSpeakerQueue->GetOrBlock();
            if (pszText == INVALID_POINTER_VALUE)
               break;

#ifdef UNICODE
            pVoice->Speak(pszText, 0, NULL);
#else
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszText, -1, wszBuffer, 4096);
            pVoice->Speak(wszBuffer, 0, NULL);
#endif
            free(pszText);
         }
         pVoice->Release();
         delete_and_null(m_pSpeakerQueue);
      }
      CoUninitialize();
   }
}


//
// Initialize speaker
//

void NXUILIB_EXPORTABLE SpeakerInit(void)
{
   _beginthread(SpeakerThread, 0, NULL);
}


//
// Shutdown speaker
//

void NXUILIB_EXPORTABLE SpeakerShutdown(void)
{
   if (m_pSpeakerQueue != NULL)
   {
      m_pSpeakerQueue->Clear();
      m_pSpeakerQueue->Put(INVALID_POINTER_VALUE);
   }
}


//
// Queue text for processing by speaker
//

BOOL NXUILIB_EXPORTABLE SpeakText(TCHAR *pszText)
{
   if (m_pSpeakerQueue == NULL)
      return FALSE;

   m_pSpeakerQueue->Put(_tcsdup(pszText));
   return TRUE;
}
