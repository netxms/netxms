/**
 * NetXMS - open source network management system
 * Copyright (C) 2022-2024 Raden Solutions
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
package org.netxms.client;

import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * Maintenance journal entry
 */
public class MaintenanceJournalEntry
{
   private long id;
   private long objectId;
   private int author;
   private int lastEditedBy;
   private String description;
   private Date creationTime;
   private Date modificationTime;

   /**
    * Create new event reference from NXCPMessage.
    * 
    * @param msg message
    * @param base base field ID
    */
   public MaintenanceJournalEntry(NXCPMessage msg, long base)
   {
      id = msg.getFieldAsInt32(base);
      objectId = msg.getFieldAsInt32(base + 1);
      author = msg.getFieldAsInt32(base + 2);
      lastEditedBy = msg.getFieldAsInt32(base + 3);
      description = msg.getFieldAsString(base + 4);
      creationTime = msg.getFieldAsDate(base + 5);
      modificationTime = msg.getFieldAsDate(base + 6);
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @return owner object id
    */
   public long getObjectId()
   {
      return objectId;
   }

   /**
    * @return the author
    */
   public int getAuthor()
   {
      return author;
   }

   /**
    * @return the last user that edited this entry
    */
   public int getLastEditedBy()
   {
      return lastEditedBy;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @return the description. If the description has multiple lines, returns the first line only. If the first line is longer than
    *         64 characters, returns first 64 characters of description. Returned value ends with "..." if it was contracted.
    */
   public String getDescriptionShort()
   {
      String trimmedDescription = description.replaceAll("(?m)^[ \t]*\r?\n", "");
      trimmedDescription = trimmedDescription.trim();
      int firstLineEnd = trimmedDescription.indexOf('\n');
      int shortLineEnd = firstLineEnd < 0 ? 64 : (firstLineEnd >= 64 ? 64 : firstLineEnd);
      return shortLineEnd >= trimmedDescription.length() ? trimmedDescription : (trimmedDescription.substring(0, shortLineEnd) + "...");
   }

   /**
    * @return the id
    */
   public Date getCreationTime()
   {
      return creationTime;
   }

   /**
    * @return the id
    */
   public Date getModificationTime()
   {
      return modificationTime;
   }
}
