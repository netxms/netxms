/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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

import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * Describes license problem in extension module.
 */
public class LicenseProblem
{
   private int id;
   private Date timestamp;
   private String component;
   private String type;
   private String description;
   
   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base filed ID
    */
   protected LicenseProblem(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId);
      timestamp = msg.getFieldAsDate(baseId + 1);
      component = msg.getFieldAsString(baseId + 2);
      type = msg.getFieldAsString(baseId + 3);
      description = msg.getFieldAsString(baseId + 4);
   }

   /**
    * Get problem ID (unique within server instance).
    *
    * @return problem ID
    */
   public int getId()
   {
      return id;
   }

   /**
    * Get problem registration timestamp.
    *
    * @return problem registration timestamp
    */
   public Date getTimestamp()
   {
      return timestamp;
   }

   /**
    * Get component name.
    *
    * @return component name
    */
   public String getComponent()
   {
      return component;
   }

   /**
    * Get problem type (component specific).
    *
    * @return component specific problem type
    */
   public String getType()
   {
      return type;
   }

   /**
    * Get problem description.
    *
    * @return problem description
    */
   public String getDescription()
   {
      return description;
   }
}
