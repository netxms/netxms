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
   if (object == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"ScheduledMaintenance: object [%u] not found", parameters->m_objectId);
      return;
   }

   if (!object->checkAccessRights(parameters->m_userId, OBJECT_ACCESS_MAINTENANCE))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"ScheduledMaintenance: Access to object %s [%u] denied", object->getName(), object->getId());
      return;
   }

   // Maintenance state checks are only valid for objects that track maintenance mode themselves;
   // for containers maintenance enter/leave recurses into child objects and state check is done there
   if (enter)
   {
      if (object->isMaintenanceApplicable() && object->isInMaintenanceMode())
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"ScheduledMaintenance: object %s [%u] is already in maintenance mode, scheduled maintenance entry ignored",
                  object->getName(), object->getId());
         return;
      }

      object->enterMaintenanceMode(parameters->m_userId, parameters->m_comments);

      // Task's persistent data may contain maintenance window duration in minutes;
      // if it does, schedule automatic exit from maintenance mode at window end
      if (parameters->m_persistentData != nullptr)
      {
         wchar_t *eptr;
         uint32_t duration = wcstoul(parameters->m_persistentData, &eptr, 10);
         if ((duration > 0) && (*eptr == 0))
         {
            AddOneTimeScheduledTask(L"Maintenance.Leave", time(nullptr) + static_cast<time_t>(duration) * 60, nullptr,
                     nullptr, parameters->m_userId, parameters->m_objectId, SYSTEM_ACCESS_FULL, parameters->m_comments, nullptr, true);
            nxlog_debug_tag(DEBUG_TAG, 5, L"ScheduledMaintenance: scheduled automatic maintenance exit for object %s [%u] in %u minutes",
                     object->getName(), object->getId(), duration);
         }
         else if (*parameters->m_persistentData != 0)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, L"ScheduledMaintenance: invalid maintenance duration \"%s\" for object %s [%u]",
                     parameters->m_persistentData, object->getName(), object->getId());
         }
      }
   }
   else
   {
      if (object->isMaintenanceApplicable() && !object->isInMaintenanceMode())
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"ScheduledMaintenance: object %s [%u] is not in maintenance mode, scheduled maintenance exit ignored",
                  object->getName(), object->getId());
         return;
      }

      object->leaveMaintenanceMode(parameters->m_userId);
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
