/**
 * NetXMS - open source network management system
 * Copyright (C) 2017-2021 Raden Solutions
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

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import org.json.JSONArray;
import org.json.JSONObject;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.constants.AggregationFunction;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DciSummaryTableColumn;
import org.netxms.client.objects.AbstractObject;
import org.netxms.websvc.json.JsonTools;
import org.netxms.websvc.json.ResponseContainer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SummaryTableAdHoc  extends AbstractHandler
{
   private Logger log = LoggerFactory.getLogger(AbstractHandler.class);

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#create(org.json.JSONObject)
    */
   @Override
   protected Object create(JSONObject data) throws Exception
   {
      NXCSession session = getSession();
      if (!session.areObjectsSynchronized())
         session.syncObjects();

      String objectFilter = JsonTools.getStringFromJson(data, "baseObject", null);
      log.debug("POST adhoc summaryTable: baseObject = " + objectFilter);
      JSONArray columnDefinitions = JsonTools.getJsonArrayFromJson(data, "columns", null);
      if ((objectFilter == null) || objectFilter.isEmpty() || (columnDefinitions == null))
      {
         log.warn("POST adhoc summaryTable: no DciSummaryTableColumn table or no value for BaseObject");
         return createErrorResponse(RCC.INVALID_ARGUMENT);
      }
      long baseObjectId;
      try
      {
         baseObjectId = Long.parseLong(objectFilter);
      }
      catch(NumberFormatException ex)
      {
         AbstractObject object = session.findObjectByName(objectFilter); 
         if (object != null)
            baseObjectId = object.getObjectId();
         else
            baseObjectId = 0;
      }

      List<DciSummaryTableColumn> columns = new ArrayList<DciSummaryTableColumn>();
      for(int i = 0; i < columnDefinitions.length(); i++)
      {
         JSONObject columnDefinition = columnDefinitions.getJSONObject(i);
         int flags = 0;
         if (JsonTools.getBooleanFromJson(columnDefinition, "isRegexp", false) || JsonTools.getBooleanFromJson(columnDefinition, "useRegexp", false))
            flags |= DciSummaryTableColumn.REGEXP_MATCH;
         if (JsonTools.getBooleanFromJson(columnDefinition, "matchByDescription", false))
            flags |= DciSummaryTableColumn.DESCRIPTION_MATCH;
         columns.add(new DciSummaryTableColumn(JsonTools.getStringFromJson(columnDefinition, "columnName", ""), JsonTools.getStringFromJson(columnDefinition, "dciName", ""), flags));
      }

      AggregationFunction agrFunc = JsonTools.getEnumFromJson(data, AggregationFunction.class, "aggregationFunction", null);   

      Date startDate;
      Date endDate;
      long date = JsonTools.getLongFromJson(data, "startDate", -1);
      startDate = date > 0 ? new Date(date * 1000) : null;
      date = JsonTools.getLongFromJson(data, "endDate", -1);
      endDate = date > 0 ? new Date(date * 1000) : null;

      boolean multiInstance = JsonTools.getBooleanFromJson(data, "multiInstance", true);
      
      Table table = session.queryAdHocDciSummaryTable(baseObjectId, columns, agrFunc, startDate, endDate, multiInstance);
      
      JSONObject root = new JSONObject();
      JSONArray resultColumns = new JSONArray();
      JSONArray resultRows = new JSONArray();
      String names[] = table.getColumnDisplayNames();
      for(int i = 0; i < names.length; i++)
         resultColumns.put(names[i]);
      root.put("columns", resultColumns);

      TableRow rows[] = table.getAllRows();
      for(int i = 0; i < rows.length; i++)
      {
         JSONArray row = new JSONArray();
         for(int j = 0; j < rows[i].size(); j++)
            row.put(rows[i].get(j).getValue());
         resultRows.put(row);
      }
      root.put("rows", resultRows);
      return new ResponseContainer("table", root);
   }
}
