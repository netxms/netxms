/**
 * NetXMS - open source network management system
 * Copyright (C) 2013-2025 Raden Solutions
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
 * AI tool
 */
public class AiTool
{
   private String name;
   private String description;
   private boolean disabled;

   /**
    * Create tool object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base ID for tool fields
    */
   public AiTool(NXCPMessage msg, long baseId)
   {
      name = msg.getFieldAsString(baseId);
      description = msg.getFieldAsString(baseId + 1);
      disabled = msg.getFieldAsBoolean(baseId + 2);
   }

   /**
    * Get tool name.
    *
    * @return tool name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get tool description.
    *
    * @return tool description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * Check if this tool is disabled by administrator.
    *
    * @return true if disabled
    */
   public boolean isDisabled()
   {
      return disabled;
   }
}
