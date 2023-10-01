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

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Device view information
 */
public class DeviceView
{
   public List<DeviceViewElement> elements;
   public List<DeviceViewImage> images;

   /**
    * Create device view object from NXCP message.
    *
    * @param msg NXCP message
    */
   protected DeviceView(NXCPMessage msg)
   {
      int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      elements = new ArrayList<>();
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         elements.add(new DeviceViewElement(msg, fieldId));
         fieldId += 20;
      }

      count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_IMAGES);
      images = new ArrayList<>();
      fieldId = NXCPCodes.VID_IMAGE_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         images.add(new DeviceViewImage(msg, fieldId));
         fieldId += 10;
      }
   }
}
