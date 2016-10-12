/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Alarm category template
 */
public class AlarmCategory
{
   // Alarm category fields
   public static final int MODIFY_ALARM_CATEGORY = 0x0001;
   public static final int MODIFY_ACCESS_CONTROL = 0x0002;
   
   private long id;
   private String name;
   private String description;
   private Long[] accessControl;

   /**
    * Create new empty alarm category.
    * 
    * @param id Alarm category ID
    */
   public AlarmCategory(long id)
   {
      this.id = id;
      name = "";
      description = "";
      accessControl = new Long[0];
   }

   /**
    * Create alarm category from NXCP message
    * 
    * @param msg NXCP message
    */
   public AlarmCategory(final NXCPMessage msg)
   {
      id = msg.getFieldAsInt64(NXCPCodes.VID_CATEGORY_ID);
      name = msg.getFieldAsString(NXCPCodes.VID_CATEGORY_NAME);
      description = msg.getFieldAsString(NXCPCodes.VID_CATEGORY_DESCRIPTION);
      accessControl = msg.getFieldAsUInt32ArrayEx(NXCPCodes.VID_ALARM_CATEGORY_ACL);
   }

   /**
    * Create alarm category from NXCP message
    * 
    * @param msg NXCP message
    */
   public AlarmCategory(final NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt64(baseId);
      name = msg.getFieldAsString(baseId + 1);
      description = msg.getFieldAsString(baseId + 2);
      accessControl = msg.getFieldAsUInt32ArrayEx(baseId + 3);
   }

   /**
    * Fill NXCP message with alarm category data
    */
   public void fillMessage(final NXCPMessage msg)
   {
      msg.setFieldInt32(NXCPCodes.VID_CATEGORY_ID, (int)id);
      msg.setField(NXCPCodes.VID_CATEGORY_NAME, name);
      msg.setField(NXCPCodes.VID_CATEGORY_DESCRIPTION, description);
      msg.setField(NXCPCodes.VID_ALARM_CATEGORY_ACL, accessControl);
   }

   /**
    * Set all attributes from another event template object.
    * 
    * @param src Original event template object
    */
   public void setAll(final AlarmCategory src)
   {
      id = src.id;
      name = src.name;
      description = src.description;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @param description the description to set
    */
   public void setDescription(String description)
   {
      this.description = description;
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @param code the code to set
    */
   public void setId(long id)
   {
      this.id = id;
   }
   
   /**
    * @return Access List
    */
   public Long[] getAccessControl()
   {
      return accessControl;
   }
   
   /**
    * @param accessControl the access control to set
    */
   public void setAccessControl(Long[] accessControl)
   {
      this.accessControl = accessControl;
   }
}
