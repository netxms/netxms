/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.client;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import org.netxms.client.constants.AggregationFunction;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DataCollectionConfigurationChangeListener;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.client.datacollection.DciSummaryTableColumn;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.PerfTabDci;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.datacollection.ThresholdViolationSummary;
import org.netxms.client.objects.DataCollectionTarget;

/**
 * Test cases for data collection
 */
public class DataCollectionTest extends AbstractSessionTest
{
	public void testGetLastValues() throws Exception
	{
		final NXCSession session = connect();
		
		DciValue[] list = session.getLastValues(TestConstants.NODE_ID);
		assertEquals(true, list.length > 0);
		
		boolean statusFound = false;
		for(int i = 0; i < list.length; i++)
		{
			System.out.println(list[i].getDescription() + " = " + list[i].getValue());
			if ((list[i].getName().equalsIgnoreCase("Status")) && (list[i].getSource() == DataCollectionObject.INTERNAL))
				statusFound = true;
		}
		assertEquals(true, statusFound);
		
		session.disconnect();
	}

	public void testGetLastValuesForMap() throws Exception
	{
		final NXCSession session = connect();
		
		DciValue[] list = session.getLastValues(TestConstants.LOCAL_NODE_ID, true, false, false);
		assertEquals(true, list.length > 0);
		
		boolean statusFound = false;
		for(int i = 0; i < list.length; i++)
		{
			System.out.println(list[i].getDescription() + " = " + list[i].getValue());
			if ((list[i].getName().equalsIgnoreCase("Status")) && (list[i].getSource() == DataCollectionObject.INTERNAL))
				statusFound = true;
		}
		assertEquals(true, statusFound);
		
		session.disconnect();
	}

	public void testGetThresholds() throws Exception
	{
		final NXCSession session = connect();

		final Object condition = new Object();
		DataCollectionConfiguration dc = session.openDataCollectionConfiguration(TestConstants.NODE_ID, 
		      new DataCollectionConfigurationChangeListener() {
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
               public void onStatusChange(long id, int status)
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
		
		Threshold[] thresholds = session.getThresholds(TestConstants.NODE_ID, dciId);
		assertNotNull(thresholds);
		assertEquals(1, thresholds.length);

		dc.deleteObject(dciId);
		dc.close();
		session.disconnect();
	}
	
	public void testGetThresholdSummary() throws Exception
	{
		final NXCSession session = connect();
		
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

	public void testGetPerfTabItems() throws Exception
	{
		final NXCSession session = connect();
		
		PerfTabDci[] list = session.getPerfTabItems(TestConstants.NODE_ID);
		assertNotNull(list);
		assertTrue(list.length > 0);
		
		for(PerfTabDci p : list)
		{
			System.out.println("id=" + p.getId() + " descr='" + p.getDescription() + "' settings='" + p.getPerfTabSettings() + "'");
		}
		
		session.disconnect();		
	}
	
	public void testDciSummaryTables() throws Exception
	{
		final NXCSession session = connect();
		
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

   public void testAdHocDciSummaryTables() throws Exception
   {
      final NXCSession session = connect();

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
