/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.events;

import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * Incident comment
 */
public class IncidentComment
{
   private long id;
   private long incidentId;
   private Date creationTime;
   private int userId;
   private String text;
   private boolean aiGenerated;

   /**
    * Create incident comment from NXCP message
    *
    * @param msg Source NXCP message
    * @param baseId Base variable ID for parsing
    */
   public IncidentComment(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt64(baseId);
      incidentId = msg.getFieldAsInt64(baseId + 1);
      creationTime = msg.getFieldAsDate(baseId + 2);
      userId = msg.getFieldAsInt32(baseId + 3);
      text = msg.getFieldAsString(baseId + 4);
      aiGenerated = msg.getFieldAsBoolean(baseId + 5);
   }

   /**
    * Get comment ID
    *
    * @return comment ID
    */
   public long getId()
   {
      return id;
   }

   /**
    * Get incident ID
    *
    * @return incident ID
    */
   public long getIncidentId()
   {
      return incidentId;
   }

   /**
    * Get creation time
    *
    * @return creation time
    */
   public Date getCreationTime()
   {
      return creationTime;
   }

   /**
    * Get user ID of comment author
    *
    * @return user ID
    */
   public int getUserId()
   {
      return userId;
   }

   /**
    * Get comment text
    *
    * @return comment text
    */
   public String getText()
   {
      return text;
   }

   /**
    * @return the aiGenerated
    */
   public boolean isAiGenerated()
   {
      return aiGenerated;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "IncidentComment [id=" + id + ", incidentId=" + incidentId + ", creationTime=" + creationTime + ", userId=" + userId + ", text=" + text + ", aiGenerated=" + aiGenerated + "]";
   }
}
