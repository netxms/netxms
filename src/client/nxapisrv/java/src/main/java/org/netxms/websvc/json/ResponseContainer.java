/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.websvc.json;

import java.util.Set;

/**
 * Generic container for named response.
 */
public class ResponseContainer
{
   private String name;
   private Object value;

   /**
    * Create container.
    * 
    * @param name
    * @param value
    */
   public ResponseContainer(String name, Object value)
   {
      this.name = name;
      this.value = value;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return the value
    */
   public Object getValue()
   {
      return value;
   }
   
   /**
    * Create JSON code from container.
    * 
    * @return JSON code
    */
   public String toJson(Set<String> fields)
   {
      StringBuilder sb = new StringBuilder("{ \"");
      sb.append(name);
      sb.append("\":");
      sb.append(JsonTools.jsonFromObject(value, fields));
      sb.append(" }");
      return sb.toString();
   }
}
