/**
 * NetXMS - open source network management system
 * Copyright (C) 2017 Raden Solutions
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

   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#create(org.json.JSONObject)
    */
   @Override
   protected Object create(JSONObject data) throws Exception
   {
      NXCSession session = getSession();
      if (!session.isObjectsSynchronized())
         session.syncObjects();
      
      String objectFilter = JsonTools.getStringFromJson(data, "baseObject", null);
      log.debug("POST adhoc summaryTable: baseObject = " + objectFilter);
      JSONArray columnFilter = JsonTools.getJsonArrayFromJson(data, "columns", null);
      if(objectFilter == null || objectFilter.isEmpty() || columnFilter == null)
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
         AbstractObject obj = session.findObjectByName(objectFilter); 
         if(obj != null)
            baseObjectId = obj.getObjectId();
         else
            baseObjectId = 0;
      }
      
      List<DciSummaryTableColumn> columns = new ArrayList<DciSummaryTableColumn>();
      for(int i = 0; i < columnFilter.length(); i++)
      {
         JSONObject obj = columnFilter.getJSONObject(i);
         columns.add(new DciSummaryTableColumn(JsonTools.getStringFromJson(obj, "columnName", ""), 
                                               JsonTools.getStringFromJson(obj, "dciName", ""), 
                                               JsonTools.getBooleanFromJson(obj, "isRegexp", false) ? DciSummaryTableColumn.REGEXP_MATCH : 0));
      }
      
      AggregationFunction agrFunc = JsonTools.getEnumFromJson(data, AggregationFunction.class, "aggregationFunction", null);   
      
      Date startDate;
      Date endDate;
      long date = JsonTools.getLongFromJson(data, "startDate", -1);
      startDate = date > 0 ? new Date(date * 1000) : null;
      date = JsonTools.getLongFromJson(data, "endDate", -1);
      endDate = date > 0 ? new Date(date * 1000) : null;
      //end date
      
      boolean multiInstance = JsonTools.getBooleanFromJson(data, "multiInstance", true);
      
      Table table = session.queryAdHocDciSummaryTable(baseObjectId, columns, agrFunc, startDate, endDate, multiInstance);
      
      //create json
      JSONObject root = new JSONObject();
      JSONArray columnList = new JSONArray();
      JSONArray rowList = new JSONArray();
      String names[] = table.getColumnDisplayNames();
      for(int i = 0; i < names.length; i++)
         columnList.put(names[i]);
      root.put("columns", columnList);
      
      TableRow rows[] = table.getAllRows();
      for(int i = 0; i < rows.length; i++)
      {
         JSONArray row = new JSONArray();
         for(int j = 0; j < rows[i].size(); j++)
            row.put(rows[i].get(j).getValue());
         rowList.put(row);
      }
      root.put("rows", rowList);
      
      
      return new ResponseContainer("table", root);
   }
}
