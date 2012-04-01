/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.PerfTabDci;
import org.netxms.client.datacollection.Threshold;

/**
 * Test cases for data collection
 *
 */
public class DataCollectionTest extends SessionTest
{
	private static final long nodeId = 101;
	
	public void testGetLastValues() throws Exception
	{
		final NXCSession session = connect();
		
		DciValue[] list = session.getLastValues(nodeId);
		assertEquals(true, list.length > 0);
		
		boolean statusFound = false;
		for(int i = 0; i < list.length; i++)
			if ((list[i].getName().equalsIgnoreCase("Status")) && (list[i].getSource() == DataCollectionObject.INTERNAL))
				statusFound = true;
		assertEquals(true, statusFound);
		
		session.disconnect();
	}

	public void testGetThresholds() throws Exception
	{
		final NXCSession session = connect();
		
		DataCollectionConfiguration dc = session.openDataCollectionConfiguration(nodeId);
		final long dciId = dc.createItem();
		DataCollectionItem dci = (DataCollectionItem)dc.findItem(dciId, DataCollectionItem.class);
		dci.setName("TEST");
		dci.getThresholds().add(new Threshold());
		dc.modifyObject(dciId);
		
		Threshold[] thresholds = session.getThresholds(nodeId, dciId);
		assertNotNull(thresholds);
		assertEquals(1, thresholds.length);

		dc.deleteObject(dciId);
		dc.close();
		session.disconnect();
	}
	
	public void testGetPerfTabItems() throws Exception
	{
		final NXCSession session = connect();
		
		PerfTabDci[] list = session.getPerfTabItems(nodeId);
		assertNotNull(list);
		assertTrue(list.length > 0);
		
		for(PerfTabDci p : list)
		{
			System.out.println("id=" + p.getId() + " descr='" + p.getDescription() + "' settings='" + p.getPerfTabSettings() + "'");
		}
		
		session.disconnect();		
	}
}
