/* 
** libnetxms - Common NetXMS utility library
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: shvar.cpp
**
**/

#ifndef _WIN32
#include "libnetxms.h"

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>


//
// Static data
//

static int m_iBlockId = -1;     // ID of shared memory block
static void *m_pSharedMem = NULL;
static int m_iMode = MODE_SHM;


//
// Initialize shared variables
//

BOOL LIBNETXMS_EXPORTABLE NxInitSharedData(int iMode, int iSize)
{
   BOOL bResult = FALSE;

   m_iMode = iMode;
   if (iMode == MODE_SHM)
   {
      m_iBlockId = shmget(IPC_PRIVATE, iSize, IPC_CREAT | S_IRUSR | S_IWUSR);
      if (m_iBlockId != -1)
      {
         m_pSharedMem = shmat(m_iBlockId, NULL, 0);
         if (m_pSharedMem == NULL)
         {
            shmctl(m_iBlockId, IPC_RMID, NULL);
            m_iBlockId = -1;
         }
         else
         {
            bResult = TRUE;
         }
      }
   }
   else
   {
      /* TODO: add db code */
   }
   return bResult;
}


//
// Destroy shared variables on exit
//

void LIBNETXMS_EXPORTABLE NxDestroySharedData(void)
{
   if (m_iMode == MODE_SHM)
   {
      if (m_pSharedMem != NULL)
         shmdt(m_pSharedMem);
      if (m_iBlockId != -1)
         shmctl(m_iBlockId, IPC_RMID, NULL);
   }
   else
   {
      /* TODO: add db code */
   }
}

#endif   /* _WIN32 */
