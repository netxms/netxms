/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

import java.util.UUID;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.EventReferenceType;

/**
 * Information about an object that is using the event
 */
public class EventReference
{
   /** Object type */
   private final EventReferenceType type;
   /** Object id within it's type */
   private final long id;
   /** Object guid */
   private final UUID guid;
   /** Object name */
   private final String name;
   /** Object owner id within it's type, if one exists (for example node id for DCI) */
   private final long ownerId;
   /** Object owner guid, if one exists (for example node guid for DCI) */
   private final UUID ownerGuid;
   /** Object owner's name */
   private final String ownerName;
   /** Additional description */
   private final String description;

   /**
    * Create new event reference from NXCPMessage.
    * @param msg message
    * @param fieldId base field ID
    */
   public EventReference(NXCPMessage msg, long fieldId)
   {
      type = EventReferenceType.getByValue(msg.getFieldAsInt32(fieldId));
      id = msg.getFieldAsInt64(fieldId + 1);
      guid = msg.getFieldAsUUID(fieldId + 2);
      name = msg.getFieldAsString(fieldId + 3);
      ownerId = msg.getFieldAsInt64(fieldId + 4);
      ownerGuid = msg.getFieldAsUUID(fieldId + 5);
      ownerName = msg.getFieldAsString(fieldId + 6);
      description = msg.getFieldAsString(fieldId + 7);
   }

   /**
    * @return the type
    */
   public EventReferenceType getType()
   {
      return type;
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @return the guid
    */
   public UUID getGuid()
   {
      return guid;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return the ownerId
    */
   public long getOwnerId()
   {
      return ownerId;
   }

   /**
    * @return the ownerGuid
    */
   public UUID getOwnerGuid()
   {
      return ownerGuid;
   }

   /**
    * @return the ownerName
    */
   public String getOwnerName()
   {
      return ownerName;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }
}
