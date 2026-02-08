/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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

import java.util.UUID;
import org.netxms.base.NXCPMessage;

/**
 * Represents a conflict detected during EPP save with optimistic concurrency.
 */
public class EPPConflict
{
   /**
    * Conflict types
    */
   public enum Type
   {
      MODIFY(1),   // Same rule modified by both client and server
      DELETE(2);   // Rule deleted by one party, modified by the other

      private final int value;

      Type(int value)
      {
         this.value = value;
      }

      public int getValue()
      {
         return value;
      }

      public static Type fromValue(int value)
      {
         for(Type t : values())
            if (t.value == value)
               return t;
         return MODIFY;
      }
   }

   private Type type;
   private UUID ruleGuid;
   private boolean hasClientRule;
   private boolean hasServerRule;
   private UUID serverModifiedByGuid;
   private String serverModifiedByName;
   private long serverModificationTime;

   /**
    * Create conflict from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID in the message
    */
   public EPPConflict(NXCPMessage msg, long baseId)
   {
      type = Type.fromValue(msg.getFieldAsInt32(baseId));
      ruleGuid = msg.getFieldAsUUID(baseId + 1);
      hasClientRule = msg.getFieldAsBoolean(baseId + 2);
      hasServerRule = msg.getFieldAsBoolean(baseId + 3);

      if (hasServerRule)
      {
         serverModifiedByGuid = msg.getFieldAsUUID(baseId + 4);
         serverModifiedByName = msg.getFieldAsString(baseId + 5);
         serverModificationTime = msg.getFieldAsInt64(baseId + 6);
      }
   }

   /**
    * Get conflict type.
    *
    * @return conflict type
    */
   public Type getType()
   {
      return type;
   }

   /**
    * Get GUID of the conflicting rule.
    *
    * @return rule GUID
    */
   public UUID getRuleGuid()
   {
      return ruleGuid;
   }

   /**
    * Check if client had a version of this rule.
    *
    * @return true if client had this rule
    */
   public boolean hasClientRule()
   {
      return hasClientRule;
   }

   /**
    * Check if server has a version of this rule.
    *
    * @return true if server has this rule
    */
   public boolean hasServerRule()
   {
      return hasServerRule;
   }

   /**
    * Get GUID of user who last modified server version.
    *
    * @return user GUID or null
    */
   public UUID getServerModifiedByGuid()
   {
      return serverModifiedByGuid;
   }

   /**
    * Get name of user who last modified server version.
    *
    * @return user name or null
    */
   public String getServerModifiedByName()
   {
      return serverModifiedByName;
   }

   /**
    * Get timestamp of last server modification.
    *
    * @return modification timestamp (Unix epoch) or 0
    */
   public long getServerModificationTime()
   {
      return serverModificationTime;
   }
}
