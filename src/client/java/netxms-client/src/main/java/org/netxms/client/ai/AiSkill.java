/**
 * NetXMS - open source network management system
 * Copyright (C) 2013-2026 Raden Solutions
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
package org.netxms.client.ai;

import org.netxms.base.NXCPMessage;

/**
 * AI skill
 */
public class AiSkill
{
   private String name;
   private String description;
   private boolean disabled;
   private boolean supportsWorker;
   private String defaultMode;

   /**
    * Create skill object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base ID for skill fields
    */
   public AiSkill(NXCPMessage msg, long baseId)
   {
      name = msg.getFieldAsString(baseId);
      description = msg.getFieldAsString(baseId + 1);
      disabled = msg.getFieldAsBoolean(baseId + 2);
      supportsWorker = msg.getFieldAsBoolean(baseId + 3);
      defaultMode = msg.getFieldAsString(baseId + 4);
   }

   /**
    * Get skill name.
    *
    * @return skill name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get skill description.
    *
    * @return skill description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * Check if this skill is disabled by administrator.
    *
    * @return true if disabled
    */
   public boolean isDisabled()
   {
      return disabled;
   }

   /**
    * Check if this skill can be run in a worker.
    *
    * @return true if skill can be run in a worker
    */
   public boolean isWorkerSupported()
   {
      return supportsWorker;
   }

   /**
    * Get default execution mode.
    *
    * @return default execution mode ("load" or "worker")
    */
   public String getDefaultMode()
   {
      return defaultMode;
   }
}
