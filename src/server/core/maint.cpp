/*
** NetXMS - Network Management System
** Copyright (C) 2015-2020 Raden Solutions
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
** File: maint.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("obj.maint")

/**
 * Execute scheduled maintenance task
 */
static void ScheduledMaintenance(const shared_ptr<ScheduledTaskParameters>& parameters, bool enter)
{
   shared_ptr<NetObj> object = FindObjectById(parameters->m_objectId);
   if (object != nullptr)
   {
      if (object->checkAccessRights(parameters->m_userId, OBJECT_ACCESS_MAINTENANCE))
      {
         if (enter)
         {
            object->enterMaintenanceMode(parameters->m_userId, parameters->m_comments);
         }
         else
         {
            object->leaveMaintenanceMode(parameters->m_userId);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ScheduledMaintenance: Access to object %s [%u] denied"), object->getName(), object->getId());
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ScheduledMaintenance: object [%u] not found"), parameters->m_objectId);
   }
}

/**
 * Scheduled task handler - enter maintenance mode
 */
void MaintenanceModeEnter(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   ScheduledMaintenance(parameters, true);
}

/**
 * Scheduled task handler - leave maintenance mode
 */
void MaintenanceModeLeave(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   ScheduledMaintenance(parameters, false);
}
