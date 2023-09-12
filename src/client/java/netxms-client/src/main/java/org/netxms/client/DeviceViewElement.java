/**
 * NetXMS - open source network management system
 * Copyright (C) 2013-2023 Raden Solutions
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
package org.netxms.client;

import org.netxms.base.NXCPMessage;

/**
 * Device view element
 */
public class DeviceViewElement
{
   public static final int BACKGROUND = 0x0001;
   public static final int BORDER = 0x0002;

   public int x;
   public int y;
   public int width;
   public int height;
   public int flags;
   public int backgroundColor;
   public int borderColor;
   public String imageName;
   public String commands;

   /**
    * Create element from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   protected DeviceViewElement(NXCPMessage msg, long baseId)
   {
      x = msg.getFieldAsInt32(baseId);
      y = msg.getFieldAsInt32(baseId + 1);
      width = msg.getFieldAsInt32(baseId + 2);
      height = msg.getFieldAsInt32(baseId + 3);
      flags = msg.getFieldAsInt32(baseId + 4);
      backgroundColor = msg.getFieldAsInt32(baseId + 5);
      borderColor = msg.getFieldAsInt32(baseId + 6);
      imageName = msg.getFieldAsString(baseId + 7);
      commands = msg.getFieldAsString(baseId + 8);
   }
}
