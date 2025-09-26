/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
import java.util.List;
import java.util.Map;

import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableColumnDefinition;
import org.netxms.client.TableRow;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.constants.RCC;
import org.netxms.client.constants.TimeUnit;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.websvc.json.ResponseContainer;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;

/**
 * Objects request handler
 */
public class HistoricalData extends AbstractObjectHandler
{
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String, java.util.Map)
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

      DataSeries data = null;
      DataCollectionConfiguration dataCollectionConfiguration = session.openDataCollectionConfiguration(getObjectId());
      DataCollectionObject dataCollectionObject = dataCollectionConfiguration.findItem(dciId);
      HistoricalDataType valueType = HistoricalDataType.PROCESSED;

      if (dataCollectionObject instanceof DataCollectionTable)
      {
         valueType = HistoricalDataType.FULL_TABLE;
      }

      if (timeFrom != null || timeTo != null)
      {
         data = session.getCollectedData(object.getObjectId(), dciId, new Date(parseLong(timeFrom, 0) * 1000), new Date(parseLong(timeTo, System.currentTimeMillis() / 1000) * 1000),
               parseInt(itemCount, 0), valueType);
      }
      else if (timeInterval != null)
      {
         Date now = new Date();
         long from = now.getTime() - parseLong(timeInterval, 0) * 1000;
         data = session.getCollectedData(object.getObjectId(), dciId, new Date(from), new Date(), parseInt(itemCount, 0), valueType);
      }
      else if (itemCount != null)
      {
         data = session.getCollectedData(object.getObjectId(), dciId, null, null, parseInt(itemCount, 0), valueType);
      }
      else
      {
         Date now = new Date();
         long from = now.getTime() - 3600000; // one hour
         data = session.getCollectedData(object.getObjectId(), dciId, new Date(from), now, parseInt(itemCount, 0), valueType);
      }

      return (dataCollectionObject instanceof DataCollectionTable) ? transformTableDataOutput(data, query.get("outputFormat")) : data;
   }

   /**
    * Transforms Full historical table data into consumable JSON format
    */
   private Object transformTableDataOutput(DataSeries data, String outputFormat) throws Exception
   {
      HashMap<Object, Object> response = new HashMap<Object, Object>();
      DciDataRow[] tableValues = data.getValues();

      // All the columns will be the same regardless of the table,
      // getting the first one to figure out the structure
      Table firstTable = (Table)tableValues[0].getValue();

      response.put("nodeId", data.getNodeId());
      response.put("dciId", data.getDciId());
      response.put("source", firstTable.getSource());
      response.put("title", firstTable.getTitle());
      response.put("columns", firstTable.getColumns());

      // Get the instance columns
      List<Integer> instanceColumns = new ArrayList<Integer>();
      for(int i = 0; i < firstTable.getColumnCount(); i++)
      {
         if (firstTable.getColumnDefinition(i).isInstanceColumn())
            instanceColumns.add(Integer.valueOf(i));
      }

      Object outputData;
      if ((outputFormat == null) || outputFormat.isEmpty() || outputFormat.equalsIgnoreCase("RowColumn"))
         outputData = transformTableDataOutputByRowColumn(tableValues, instanceColumns);
      else if (outputFormat.equalsIgnoreCase("ColumnRow"))
         outputData = transformTableDataOutputByColumnRow(tableValues);
      else if (outputFormat.equalsIgnoreCase("InstanceColumnRow"))
         outputData = transformTableDataOutputByInstanceColumnRow(tableValues, instanceColumns);
      else
         throw new Exception("Invalid table output format");

      response.put("values", outputData);
      return response;
   }

   /**
    * Transforms Full historical table data into consumable JSON format, grouped by row and column
    */
   private Object transformTableDataOutputByRowColumn(DciDataRow[] tableValues, List<Integer> instanceColumns) throws Exception
   {
      JsonArray output = new JsonArray(tableValues.length);
      for(int i = 0; i < tableValues.length; i++)
      {
         Table table = (Table)tableValues[i].getValue();

         JsonObject entry = new JsonObject();
         entry.addProperty("timestamp", tableValues[i].getTimestamp().getTime() / 1000);

         JsonArray rows = new JsonArray(table.getRowCount());
         for(int j = 0; j < table.getRowCount(); j++)
         {
            TableRow r = table.getRow(j);
            JsonObject jr = new JsonObject();
            jr.addProperty("__instance", instanceKeyForRow(table, j, instanceColumns));
            for(int k = 0; k < table.getColumnCount(); k++)
            {
               jr.addProperty(table.getColumnName(k), r.getValue(k));
            }
            rows.add(jr);
         }
         entry.add("rows", rows);

         output.add(entry);
      }
      return output;
   }

   /**
    * Transforms Full historical table data into consumable JSON format, grouped by column and row
    */
   private Object transformTableDataOutputByColumnRow(DciDataRow[] tableValues) throws Exception
   {
      JsonArray output = new JsonArray(tableValues.length);
      for(int i = 0; i < tableValues.length; i++)
      {
         Table table = (Table)tableValues[i].getValue();

         JsonObject entry = new JsonObject();
         entry.addProperty("timestamp", tableValues[i].getTimestamp().getTime() / 1000);

         JsonObject columns = new JsonObject();
         for(int j = 0; j < table.getColumnCount(); j++)
         {
            TableColumnDefinition c = table.getColumnDefinition(j);
            JsonArray jc = new JsonArray(table.getRowCount());
            for(int k = 0; k < table.getRowCount(); k++)
            {
               jc.add(table.getCellValue(k, j));
            }
            columns.add(c.getName(), jc);
         }
         entry.add("columns", columns);

         output.add(entry);
      }
      return output;
   }

   /**
    * Transforms Full historical table data into consumable JSON format, grouped by instance, column, and row
    */
   @SuppressWarnings("unchecked")
   private Object transformTableDataOutputByInstanceColumnRow(DciDataRow[] tableValues, List<Integer> instanceColumns) throws Exception
   {
      if (instanceColumns.isEmpty())
         throw new Exception("No instance columns in requested table");

      Map<String, Map<String, Object>> transformedData = new HashMap<>();
      for(int i = 0; i < tableValues.length; i++)
      {
         long timestamp = tableValues[i].getTimestamp().getTime() / 1000;
         Table table = (Table)tableValues[i].getValue();
         for(int j = 0; j < table.getRowCount(); j++)
         {
            String instanceKey = instanceKeyForRow(table, j, instanceColumns);
            if (transformedData.get(instanceKey) == null)
            {
               Map<String, Object> data = new HashMap<>();
               data.put("__instance", instanceKey);
               transformedData.put(instanceKey, data);
            }

            for(TableColumnDefinition column : table.getColumns())
            {
               HashMap<String, Object> cell = new HashMap<String, Object>();
               cell.put("value", table.getCellValue(j, table.getColumnIndex(column.getName())));
               cell.put("timestamp", timestamp);
               List<Map<String, Object>> cells = (List<Map<String, Object>>)transformedData.get(instanceKey).get(column.getName());
               if (cells == null)
               {
                  cells = new ArrayList<Map<String, Object>>();
                  transformedData.get(instanceKey).put(column.getName(), cells);
               }
               cells.add(cell);
            }
         }
      }

      ArrayList<Object> dataList = new ArrayList<Object>();
      for(Object _dataItem : transformedData.values())
      {
         dataList.add(_dataItem);
      }
      return dataList;
   }

   /**
    * Build instance key for given table row.
    *
    * @param table table object
    * @param row row number
    * @param instanceColumns list of instance column indexes
    * @return instance key
    */
   private static String instanceKeyForRow(Table table, int row, List<Integer> instanceColumns)
   {
      if (instanceColumns.isEmpty())
         return null;

      StringBuilder instanceKey = new StringBuilder();
      for(Integer column : instanceColumns)
      {
         if (instanceKey.length() > 0)
            instanceKey.append('-');
         instanceKey.append(table.getCellValue(row, column));
      }
      return instanceKey.toString();
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();

      String dciQuery = query.get("dciList");
      String[] requestPairs = dciQuery.split(";");
      if (requestPairs == null)
         throw new NXCException(RCC.INVALID_DCI_ID);

      HashMap<Long, DataSeries> dciData = new HashMap<Long, DataSeries>();

      for(int i = 0; i < requestPairs.length; i++)
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

         DataSeries collectedData = null;

         if (!timeFrom.equals("0") || !timeTo.equals("0"))
         {
            collectedData = session.getCollectedData(parseLong(nodeId, 0), parseLong(dciId, 0), new Date(parseLong(timeFrom, 0) * 1000),
                  new Date(parseLong(timeTo, System.currentTimeMillis() / 1000) * 1000), 0, HistoricalDataType.PROCESSED);
         }
         else if (!timeInterval.equals("0"))
         {
            Date now = new Date();
            long from;
            if (parseInt(timeUnit, 0) == TimeUnit.HOUR.getValue())
               from = now.getTime() - parseLong(timeInterval, 0) * 3600000;
            else if (parseInt(timeUnit, 0) == TimeUnit.DAY.getValue())
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
