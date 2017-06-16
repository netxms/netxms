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
import org.json.JSONObject;

/**
 * Filter for JSONObject class
 */
public class JsonFilterOrgJsonObject extends JsonFilter<JSONObject>
{
   /**
    * @param object
    * @param fields
    */
   protected JsonFilterOrgJsonObject(JSONObject object, Set<String> fields)
   {
      super(object, fields);
   }

   /* (non-Javadoc)
    * @see org.netxms.websvc.json.JsonFilter#filter()
    */
   @Override
   public JSONObject filter()
   {
      JSONObject result = new JSONObject();
      for(String s : object.keySet())
      {
         if (fields.contains(s))
         {
            result.put(s, object.get(s));
         }
      }
      return result;
   }
}
