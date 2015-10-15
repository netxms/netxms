/*
** NetXMS - Network Management System
** Copyright (C) 2015 Alex Kirhenshtein
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
** File: maintenance_job.cpp
**
**/

#include "nxcore.h"

static void ScheduledMaintenance(const ScheduleParameters *params, bool enter)
{
   if(params->m_objectId == 0)
   {
      DbgPrintf(4, _T("MaintenanceJob: object ID is 0"));
      return;
   }

   NetObj *object = FindObjectById(params->m_objectId);
   if (object != NULL)
   {
      if (object->checkAccessRights(params->m_userId, OBJECT_ACCESS_CONTROL))
      {
         if (enter)
         {
            object->enterMaintenanceMode();
         }
         else
         {
            object->leaveMaintenanceMode();
         }
      }
      else
      {
         DbgPrintf(4, _T("MaintenanceJob: Access to node %s denied"), object->getName());
      }
   }
   else
   {
      DbgPrintf(4, _T("MaintenanceJob: object %d not found"), object->getId());
   }
}

static void MaintenanceEnter(const ScheduleParameters *params)
{
   ScheduledMaintenance(params, true);
}

static void MaintenanceLeave(const ScheduleParameters *params)
{
   ScheduledMaintenance(params, false);
}

void InitMaintenanceJobScheduler()
{
   RegisterSchedulerTaskHandler(_T("Maintenance.Enter"), MaintenanceEnter, SYSTEM_ACCESS_SCHEDULE_MAINTENANCE);
   RegisterSchedulerTaskHandler(_T("Maintenance.Leave"), MaintenanceLeave, SYSTEM_ACCESS_SCHEDULE_MAINTENANCE);
}
