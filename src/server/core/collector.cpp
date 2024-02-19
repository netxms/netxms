/*
** NetXMS - Network Management System
** Copyright (C) 2024 Raden Solutions
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
** File: collector.cpp
**
**/

#include "nxcore.h"

/**
 * Load from database
 */
bool Collector::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   m_id = id;

   if (!loadCommonProperties(hdb))
      return false;

   if (!super::loadFromDatabase(hdb, id))
      return false;

   if (!AutoBindTarget::loadFromDatabase(hdb, id))
      return false;

   if (!Pollable::loadFromDatabase(hdb, id))
      return false;

   if (!m_isDeleted)
      ContainerBase::loadFromDatabase(hdb, id);

   return true;
}

/**
 * Save to database
 */
bool Collector::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success)
      success = ContainerBase::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OTHER))
      success = AutoBindTarget::saveToDatabase(hdb);

   return success;
}

/**
 * Delete from database
 */
bool Collector::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);

   if (success)
      success = ContainerBase::deleteFromDatabase(hdb);

   if (success)
      success = AutoBindTarget::deleteFromDatabase(hdb);

   return success;
}

/**
 * Return STATUS_NORMAL as additional status so that empty collector will have NORMAL status instead of UNKNOWN.
 */
int Collector::getAdditionalMostCriticalStatus()
{
   return STATUS_NORMAL;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Collector::showThresholdSummary() const
{
   return true;
}

//TODO: add autobind function
