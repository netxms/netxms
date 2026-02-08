/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.constants.AggregationFunction;
import org.netxms.client.constants.DataCollectionObjectStatus;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.client.datacollection.DciSummaryTableColumn;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.PerfTabDci;
import org.netxms.client.datacollection.RemoteChangeListener;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.datacollection.ThresholdViolationSummary;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.Node;
import org.netxms.utilities.TestHelper;

/**
 * Test cases for data collection
 */
public class DataCollectionTest extends AbstractSessionTest
{
   @Test
   public void testGetLastValues() throws Exception
   {
      final NXCSession session = connectAndLogin();

      Node node = TestHelper.findManagementServer(session);
      assertNotNull(node);

      DataCollectionConfiguration dcc;
      dcc = session.openDataCollectionConfiguration(node.getObjectId());

      final DataCollectionItem dci = new DataCollectionItem(dcc, 0);

      dci.setOrigin(DataOrigin.INTERNAL);
      dci.setDescription("testCreateDCI");
      dci.setName("Status");

      long id = dcc.modifyObject(dci);

      DciValue[] list = session.getLastValues(node.getObjectId());
      assertEquals(true, list.length > 0);

      boolean statusFound = false;
      for(int i = 0; i < list.length; i++)
      {
         System.out.println(list[i].getDescription() + " = " + list[i].getValue());
         if ((list[i].getName().equalsIgnoreCase("Status")) && (list[i].getSource() == DataOrigin.INTERNAL))
            statusFound = true;
      }
      assertEquals(true, statusFound);

      dcc.deleteObject(id);
      dcc.close();
      session.disconnect();
   }

   @Test
   public void testGetLastValuesForMap() throws Exception
   {
      final NXCSession session = connectAndLogin();

      Node node = TestHelper.findManagementServer(session);
      assertNotNull(node);

      DataCollectionConfiguration dcc;
      dcc = session.openDataCollectionConfiguration(node.getObjectId());

      final DataCollectionItem dci = new DataCollectionItem(dcc, 0);

      dci.setOrigin(DataOrigin.INTERNAL);
      dci.setDescription("testCreateDCI");
      dci.setName("Status");
      dci.setShowOnObjectTooltip(true);

      long id = dcc.modifyObject(dci);
      DciValue[] list = session.getDataCollectionSummary(node.getObjectId(), true, false, false);

      boolean statusFound = false;
      for(int i = 0; i < list.length; i++)
      {
         System.out.println(list[i].getDescription() + " = " + list[i].getValue());
         if ((list[i].getName().equalsIgnoreCase("Status")) && (list[i].getSource() == DataOrigin.INTERNAL))
            statusFound = true;
      }
      assertEquals(true, statusFound);

      dcc.deleteObject(id);
      dcc.close();
      session.disconnect();
   }

   @Test
   public void testGetThresholds() throws Exception
   {
      final NXCSession session = connectAndLogin();
      Node node = TestHelper.findManagementServer(session);
      assertNotNull(node);

      final Object condition = new Object();
      DataCollectionConfiguration dc = session.openDataCollectionConfiguration(node.getObjectId(), new RemoteChangeListener() {
         @Override
         public void onUpdate(DataCollectionObject object)
         {
            if (object.getName().equals("ThresholdTest-NewDCI"))
            {
               synchronized(condition)
               {
                  condition.notifyAll();
               }
            }
         }

         @Override
         public void onDelete(long id)
         {
         }

         @Override
         public void onStatusChange(long id, DataCollectionObjectStatus status)
         {
         }
      });
      DataCollectionItem newDci = new DataCollectionItem(dc, 0);
      newDci.setName("ThresholdTest-NewDCI");

      long dciId;
      synchronized(condition)
      {
         dciId = dc.modifyObject(newDci);
         condition.wait();
      }

      DataCollectionItem dci = (DataCollectionItem)dc.findItem(dciId, DataCollectionItem.class);
      assertNotNull(dci);

      dci.setName("TEST");
      dci.getThresholds().add(new Threshold());
      dc.modifyObject(dciId);

      Threshold[] thresholds = session.getThresholds(node.getObjectId(), dciId);
      assertNotNull(thresholds);
      assertEquals(1, thresholds.length);

      dc.deleteObject(dciId);
      dc.close();
      session.disconnect();
   }

   @Test
   public void testGetThresholdSummary() throws Exception
   {
      final NXCSession session = connectAndLogin();

      session.syncObjects();

      final List<ThresholdViolationSummary> list = session.getThresholdSummary(1);
      for(ThresholdViolationSummary s : list)
      {
         DataCollectionTarget target = (DataCollectionTarget)session.findObjectById(s.getNodeId(), DataCollectionTarget.class);
         System.out.println("* " + target.getObjectName());
         if (s.getDciList().size() > 0)
         {
            for(DciValue v : s.getDciList())
            {
               System.out.println("   + " + v.getDescription());
            }
         }
         else
         {
            System.out.println("   --- no threshold violations");
         }
      }

      session.disconnect();
   }

   @Test
   public void testGetPerfTabItems() throws Exception
   {
      final NXCSession session = connectAndLogin();
      Node node = TestHelper.findManagementServer(session);
      assertNotNull(node);

      List<PerfTabDci> list = session.getPerfTabItems(node.getObjectId());
      assertNotNull(list);
      assertFalse(list.isEmpty());

      for(PerfTabDci p : list)
      {
         System.out.println("id=" + p.getId() + " descr='" + p.getDescription() + "' settings='" + p.getPerfTabSettings() + "'");
      }

      session.disconnect();
   }

   @Test
   public void testDciSummaryTables() throws Exception
   {
      final NXCSession session = connectAndLogin();

      DciSummaryTable t = new DciSummaryTable("test", "Test Table");
      t.getColumns().add(new DciSummaryTableColumn("Idle", "System.CPU.Idle"));
      t.getColumns().add(new DciSummaryTableColumn("I/O Wait", "System.CPU.IOWait"));

      int id = session.modifyDciSummaryTable(t);
      System.out.println("Assigned ID: " + id);
      t.setId(id);

      t.getColumns().add(new DciSummaryTableColumn("System", "^System\\.CPU\\.Sys.*", DciSummaryTableColumn.REGEXP_MATCH));
      session.modifyDciSummaryTable(t);

      List<DciSummaryTableDescriptor> list = session.listDciSummaryTables();
      for(DciSummaryTableDescriptor d : list)
         System.out.println(d.getId() + ": " + d.getMenuPath() + " " + d.getTitle());

      session.getDciSummaryTable(id);
      session.deleteDciSummaryTable(id);

      try
      {
         session.getDciSummaryTable(id);
         assertTrue(false);
      }
      catch(NXCException e)
      {
         if (e.getErrorCode() != RCC.INVALID_SUMMARY_TABLE_ID)
            throw e;
      }

      session.disconnect();
   }

   private void printTable(Table t)
   {
      System.out.println(t.getRowCount() + " rows in result set");
      for(int i = 0; i < t.getColumnCount(); i++)
         System.out.print(String.format(" | %-20s", t.getColumnDisplayName(i)));
      System.out.println(" |");
      for(TableRow r : t.getAllRows())
      {
         for(int i = 0; i < t.getColumnCount(); i++)
            System.out.print(String.format(" | %-20s", r.get(i).getValue()));
         System.out.println(" |");
      }
   }

   @Test
   public void testAdHocDciSummaryTables() throws Exception
   {
      final NXCSession session = connectAndLogin();

      // single instance
      List<DciSummaryTableColumn> columns = new ArrayList<DciSummaryTableColumn>();
      columns.add(new DciSummaryTableColumn("Usage", "System.CPU.Usage", 0, ";"));
      columns.add(new DciSummaryTableColumn("I/O Wait", "System.CPU.IOWait", 0, ";"));

      Table result = session.queryAdHocDciSummaryTable(2, columns, AggregationFunction.AVERAGE, new Date(System.currentTimeMillis() - 86400000), new Date(), false);
      printTable(result);

      // multi instance
      columns.clear();
      columns.add(new DciSummaryTableColumn("Free %", "FileSystem\\.FreePerc\\(.*\\)", DciSummaryTableColumn.REGEXP_MATCH, ";"));
      columns.add(new DciSummaryTableColumn("Free bytes", "FileSystem\\.Free\\(.*\\)", DciSummaryTableColumn.REGEXP_MATCH, ";"));

      result = session.queryAdHocDciSummaryTable(2, columns, AggregationFunction.LAST, null, null, true);
      printTable(result);

      session.disconnect();
   }
}
