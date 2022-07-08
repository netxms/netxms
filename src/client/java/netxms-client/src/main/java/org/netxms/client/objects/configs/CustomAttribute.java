/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.client.objects.configs;

import org.netxms.base.NXCPMessage;

/**
 * Custom attribute class
 */
public class CustomAttribute
{
   public static final long INHERITABLE = 1;
   public static final long REDEFINED = 2;
   public static final long CONFLICT = 4;
   
   protected String value;
   protected long flags;
   protected long sourceObject; 
   
   /**
    * Constructor form NXCPMessage
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public CustomAttribute(final NXCPMessage msg, long baseId)
   {
      value = msg.getFieldAsString(baseId);
      flags = msg.getFieldAsInt32(baseId + 1);
      sourceObject = msg.getFieldAsInt32(baseId + 2);
   }
   
   /**
    * Copy constructor
    * 
    * @param src source object
    */
   public CustomAttribute(CustomAttribute src)
   {
      value = src.value;
      flags = src.flags;
      sourceObject = src.sourceObject;
   }

   /**
    * Create custom attribute from scratch. 
    * 
    * @param value value
    * @param flags flags
    * @param inheritedFrom ID of NetXMS object this attribute was inherited from (0 for non-inherited attributes)
    */
   public CustomAttribute(String value, long flags, long inheritedFrom)
   {
      this.value = value;
      this.flags = flags;
      this.sourceObject = inheritedFrom;
   }

   /**
    * @return the value
    */
   public String getValue()
   {
      return value;
   }

   /**
    * @return the flags
    */
   public long getFlags()
   {
      return flags;
   }

   /**
    * @return the inheritedFrom
    */
   public long getSourceObject()
   {
      return sourceObject;
   }

   /**
    * If attribute is redefined
    * @return true if redefined
    */
   public boolean isRedefined()
   {
      return (flags & REDEFINED) > 0;
   }   

   /**
    * If attribute is inherited
    * @return true if has source
    */
   public boolean isInherited()
   {
      return getSourceObject() != 0;
   } 

   /**
    * If attribute is inheritable
    * @return true if inheritable flag
    */
   public boolean isInheritable()
   {
      return (flags & INHERITABLE) > 0;
   } 

   /**
    * If attribute is inheritable
    * @return true if inheritable flag
    */
   public boolean isConflict()
   {
      return (flags & CONFLICT) > 0;
   } 
}
