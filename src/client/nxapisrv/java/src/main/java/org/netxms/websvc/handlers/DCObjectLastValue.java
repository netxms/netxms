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
package org.netxms.websvc.handlers;

import java.util.Map;
import org.netxms.client.TableColumnDefinition;
import org.netxms.client.TableRow;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciLastValue;
import org.netxms.websvc.json.JsonTools;
import com.google.gson.Gson;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;

/**
 * Handler for data collection object last value request
 */
public class DCObjectLastValue extends AbstractObjectHandler
{
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String, java.util.Map)
    */
   @Override
   protected Object get(String id, Map<String, String> query) throws Exception
   {
      DciLastValue value = getSession().getDciLastValue(getObjectId(), Long.parseLong(id));
      if (value.getDciType() != DataCollectionObject.DCO_TYPE_TABLE)
         return value;

      JsonObject result = new JsonObject();
      result.addProperty("dciType", value.getDciType());
      result.addProperty("dataOrigin", value.getDataOrigin().toString());
      result.addProperty("timestamp", value.getTimestamp().getTime());

      JsonObject tableValue = new JsonObject();
      result.add("tableValue", tableValue);

      Gson gson = JsonTools.createGsonInstance();
      TableColumnDefinition[] columns = value.getTableValue().getColumns();
      tableValue.add("columns", gson.toJsonTree(columns));

      JsonArray rows = new JsonArray();
      tableValue.add("data", rows);

      if (Boolean.parseBoolean(query.getOrDefault("rowsAsObjects", "false")))
      {
         for(TableRow r : value.getTableValue().getAllRows())
         {
            JsonObject row = new JsonObject();
            for(int i = 0; i < columns.length; i++)
               row.addProperty(columns[i].getName(), r.getValue(i));
            rows.add(row);
         }
      }
      else
      {
         for(TableRow r : value.getTableValue().getAllRows())
         {
            JsonArray row = new JsonArray();
            for(int i = 0; i < columns.length; i++)
               row.add(r.getValue(i));
            rows.add(row);
         }
      }

      return result;
   }
}
