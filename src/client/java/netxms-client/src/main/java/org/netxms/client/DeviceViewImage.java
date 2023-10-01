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
 * Image referenced by device view
 */
public class DeviceViewImage
{
   public String name;
   public byte[] data;

   /**
    * Create image object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   protected DeviceViewImage(NXCPMessage msg, long baseId)
   {
      name = msg.getFieldAsString(baseId);
      data = msg.getFieldAsBinary(baseId + 1);
   }
}
