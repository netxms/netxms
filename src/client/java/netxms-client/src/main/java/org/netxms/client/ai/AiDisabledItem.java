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
 * Disabled AI item that is not currently registered (admin-added arbitrary name)
 */
public class AiDisabledItem
{
   private String name;
   private char type; // 'S' = skill, 'F' = tool

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base ID for fields
    */
   public AiDisabledItem(NXCPMessage msg, long baseId)
   {
      name = msg.getFieldAsString(baseId);
      String typeStr = msg.getFieldAsString(baseId + 1);
      type = (typeStr != null && !typeStr.isEmpty()) ? typeStr.charAt(0) : 'F';
   }

   /**
    * Create with explicit values.
    *
    * @param name item name
    * @param type item type ('S' for skill, 'F' for tool)
    */
   public AiDisabledItem(String name, char type)
   {
      this.name = name;
      this.type = type;
   }

   /**
    * Get item name.
    *
    * @return item name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get item type.
    *
    * @return 'S' for skill, 'F' for tool
    */
   public char getType()
   {
      return type;
   }
}
