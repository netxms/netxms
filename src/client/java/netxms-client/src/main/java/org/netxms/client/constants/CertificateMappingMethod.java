/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.client.constants;

import java.util.HashMap;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Certificate mapping method
 */
public enum CertificateMappingMethod
{
   SUBJECT(0),
   PUBLIC_KEY(1),
   COMMON_NAME(2),
   TEMPLATE_ID(3);

   private static Logger logger = LoggerFactory.getLogger(CertificateMappingMethod.class);
   private static Map<Integer, CertificateMappingMethod> lookupTable = new HashMap<Integer, CertificateMappingMethod>();
   static
   {
      for(CertificateMappingMethod element : CertificateMappingMethod.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   private int value;

   /**
    * Internal constructor
    *  
    * @param value integer value
    */
   private CertificateMappingMethod(int value)
   {
      this.value = value;
   }

   /**
    * Get integer value
    * 
    * @return integer value
    */
   public int getValue()
   {
      return value;
   }

   /**
    * Get enum element by integer value
    * 
    * @param value integer value
    * @return enum element corresponding to given integer value or fall-back element for invalid value
    */
   public static CertificateMappingMethod getByValue(int value)
   {
      final CertificateMappingMethod element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return SUBJECT; // fall-back
      }
      return element;
   }
}
