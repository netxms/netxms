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
package org.netxms.websvc.handlers;

import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableColumnDefinition;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.websvc.json.ResponseContainer;

/**
 * Objects request handler
 */
public class HistoricalData extends AbstractObjectHandler
{
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String)
    */
   @Override
   protected Object get(String id, Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      AbstractObject object = getObject();
      long dciId = 0;
      try 
      {
         dciId = Long.parseLong(id);
      }
      catch(NumberFormatException e)
      {
         dciId = session.dciNameToId(object.getObjectId(), id);
      }

      if ((object == null) || (dciId == 0) || !(object instanceof DataCollectionTarget))
         throw new NXCException(RCC.INVALID_OBJECT_ID);
      
      String timeFrom = query.get("from");
      String timeTo = query.get("to");
      String timeInterval = query.get("timeInterval");
      String itemCount = query.get("itemCount");

      DciData data = null;
      DataCollectionConfiguration dataCollectionConfiguration = session.openDataCollectionConfiguration(getObjectId());
      DataCollectionObject dataCollectionObject = dataCollectionConfiguration.findItem(dciId);
      HistoricalDataType valueType = HistoricalDataType.PROCESSED;

      if (dataCollectionObject instanceof DataCollectionTable) {
         valueType = HistoricalDataType.FULL_TABLE;
      }

      if (timeFrom != null || timeTo != null) {
         data = session.getCollectedData(object.getObjectId(), dciId, new Date(parseLong(timeFrom, 0) * 1000),
               new Date(parseLong(timeTo, System.currentTimeMillis() / 1000) * 1000), parseInt(itemCount, 0),
               valueType);
      } else if (timeInterval != null) {
         Date now = new Date();
         long from = now.getTime() - parseLong(timeInterval, 0) * 1000;
         data = session.getCollectedData(object.getObjectId(), dciId, new Date(from), new Date(),
               parseInt(itemCount, 0), valueType);
      } else if (itemCount != null) {
         data = session.getCollectedData(object.getObjectId(), dciId, null, null, parseInt(itemCount, 0), valueType);
      } else {
         Date now = new Date();
         long from = now.getTime() - 3600000; // one hour
         data = session.getCollectedData(object.getObjectId(), dciId, new Date(from), now, parseInt(itemCount, 0),
               valueType);
      }

      if (dataCollectionObject instanceof DataCollectionTable) {
         return new ResponseContainer("values", transformTableDataOutput(data));
      }

      return new ResponseContainer("values", data);
   }

   /**
    * Transforms Full historical table data into consumable JSON format
    */
   private Map<Object, Object> transformTableDataOutput(DciData data) throws Exception {
      HashMap<Object, Object> response = new HashMap<Object, Object>();
      DciDataRow[] tableValues = data.getValues();

      // All the columns will be the same regardless of the table,
      // getting the first one to figure out the structure
      Table firstTable = (Table) tableValues[0].getValue();

      response.put("nodeId", data.getNodeId());
      response.put("dciId", data.getDciId());
      response.put("dataType", data.getDataType());
      response.put("source", firstTable.getSource());
      response.put("title", firstTable.getTitle());
      response.put("columns", firstTable.getColumns());

      // Get the instance columns
      // Assumes that table has atleast 1 instance column
      ArrayList<TableColumnDefinition> instanceColumns = new ArrayList<TableColumnDefinition>();
      for (TableColumnDefinition column : firstTable.getColumns()) {
         if (column.isInstanceColumn()) {
            instanceColumns.add(column);
         }
      }
      if (instanceColumns.isEmpty()) {
         throw new Exception("No instance column found in table");
      }

      Map<String, HashMap<String, ArrayList<HashMap<String, Object>>>> transformedData = new HashMap<>();
      for (int i = 0; i < tableValues.length; i++) {
         long timestamp = tableValues[i].getTimestamp().getTime();
         Table table = (Table) tableValues[i].getValue();
         for (int j = 0; j < table.getRowCount(); j++) {

            String instanceKey = null;
            for (TableColumnDefinition instanceColumn : instanceColumns) {
               if (instanceKey != null) {
                  instanceKey += "-";
               }
               instanceKey += table.getCellValue(j, table.getColumnIndex(instanceColumn.getName()));
            }

            if (transformedData.get(instanceKey) == null) {
               transformedData.put(instanceKey, new HashMap<String, ArrayList<HashMap<String, Object>>>());
            }

            for (TableColumnDefinition column : table.getColumns()) {
               HashMap<String, Object> cell = new HashMap<String, Object>();
               cell.put("value", table.getCellValue(j, table.getColumnIndex(column.getName())));
               cell.put("timestamp", timestamp);
               if (transformedData.get(instanceKey).get(column.getName()) == null) {
                  transformedData.get(instanceKey).put(column.getName(), new ArrayList<HashMap<String, Object>>());
               }
               transformedData.get(instanceKey).get(column.getName()).add(cell);
            }
         }
      }

      ArrayList<Object> dataList = new ArrayList<Object>();
      for (Object _dataItem : transformedData.values()) {
         dataList.add(_dataItem);
      }
      response.put("data", dataList);
      return response;
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();

      String dciQuery = query.get("dciList");
      String[] requestPairs = dciQuery.split(";");
      if (requestPairs == null)
         throw new NXCException(RCC.INVALID_DCI_ID);

      HashMap<Long, DciData> dciData = new HashMap<Long, DciData>();

      for (int i = 0; i < requestPairs.length; i++)
      {
         String[] dciPairs = requestPairs[i].split(",");
         if (dciPairs == null)
            throw new NXCException(RCC.INVALID_DCI_ID);

         String dciId = dciPairs[0];
         String nodeId = dciPairs[1];
         String timeFrom = dciPairs[2];
         String timeTo = dciPairs[3];
         String timeInterval = dciPairs[4];
         String timeUnit = dciPairs[5];

         if (dciId == null || nodeId == null || !(session.findObjectById(parseLong(nodeId, 0)) instanceof DataCollectionTarget))
            throw new NXCException(RCC.INVALID_OBJECT_ID);

         DciData collectedData = null;

         if (!timeFrom.equals("0") || !timeTo.equals("0"))
         {
            collectedData = session.getCollectedData(parseLong(nodeId, 0), parseLong(dciId, 0),
                  new Date(parseLong(timeFrom, 0) * 1000), new Date(parseLong(timeTo, System.currentTimeMillis() / 1000) * 1000),
                  0, HistoricalDataType.PROCESSED);
         }
         else if (!timeInterval.equals("0"))
         {
            Date now = new Date();
            long from;
            if (parseInt(timeUnit, 0 ) == GraphSettings.TIME_UNIT_HOUR)
               from = now.getTime() - parseLong(timeInterval, 0) * 3600000;
            else if (parseInt(timeUnit, 0 ) == GraphSettings.TIME_UNIT_DAY)
               from = now.getTime() - parseLong(timeInterval, 0) * 3600000 * 24;
            else
               from = now.getTime() - parseLong(timeInterval, 0) * 60000;

            collectedData = session.getCollectedData(parseInt(nodeId, 0), parseInt(dciId, 0), new Date(from), new Date(), 0, HistoricalDataType.PROCESSED);
         }
         else
         {
            Date now = new Date();
            long from = now.getTime() - 3600000; // one hour
            collectedData = session.getCollectedData(parseInt(nodeId, 0), parseInt(dciId, 0), new Date(from), now, 0, HistoricalDataType.PROCESSED);
         }
         
         dciData.put((long)parseInt(dciId, 0), collectedData);
      }

      return new ResponseContainer("values", dciData);
   }
}
