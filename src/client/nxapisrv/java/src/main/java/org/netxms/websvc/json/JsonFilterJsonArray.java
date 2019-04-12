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
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

/**
 * Filter for JSON arrays
 */
public class JsonFilterJsonArray extends JsonFilter<JsonArray>
{
   /**
    * @param object
    * @param fields
    */
   protected JsonFilterJsonArray(JsonArray object, Set<String> fields)
   {
      super(object, fields);
   }

   /* (non-Javadoc)
    * @see org.netxms.websvc.json.JsonFilter#filter()
    */
   @Override
   public JsonArray filter()
   {
      for(int i = 0; i < object.size(); i++)
      {
         JsonElement e = object.get(i);
         if (e instanceof JsonObject)
         {
            object.set(i, new JsonFilterJsonObject((JsonObject)e, fields).filter());
         }
      }
      return object;
   }
}
