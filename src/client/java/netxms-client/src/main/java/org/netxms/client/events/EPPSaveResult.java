/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.events;

import java.util.ArrayList;
import java.util.List;

/**
 * Result of EPP save operation with optimistic concurrency.
 */
public class EPPSaveResult
{
   private boolean success;
   private int newVersion;
   private List<EPPConflict> conflicts;

   /**
    * Create a successful result.
    *
    * @param newVersion new policy version after successful save
    * @return successful save result
    */
   public static EPPSaveResult success(int newVersion)
   {
      EPPSaveResult result = new EPPSaveResult();
      result.success = true;
      result.newVersion = newVersion;
      result.conflicts = new ArrayList<>();
      return result;
   }

   /**
    * Create a conflict result.
    *
    * @param serverVersion current server version
    * @param conflicts list of conflicts
    * @return conflict result
    */
   public static EPPSaveResult conflict(int serverVersion, List<EPPConflict> conflicts)
   {
      EPPSaveResult result = new EPPSaveResult();
      result.success = false;
      result.newVersion = serverVersion;
      result.conflicts = conflicts;
      return result;
   }

   /**
    * Private constructor - use static factory methods.
    */
   private EPPSaveResult()
   {
   }

   /**
    * Check if save was successful.
    *
    * @return true if save succeeded
    */
   public boolean isSuccess()
   {
      return success;
   }

   /**
    * Get new policy version (after successful save) or current server version (on conflict).
    *
    * @return policy version
    */
   public int getNewVersion()
   {
      return newVersion;
   }

   /**
    * Get list of conflicts (empty if save was successful).
    *
    * @return list of conflicts
    */
   public List<EPPConflict> getConflicts()
   {
      return conflicts;
   }
}
