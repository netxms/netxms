/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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

import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DciValue;

/**
 * @author Victor
 *
 */
public class DataCollectionTest extends SessionTest
{
	public void testGetLastValues() throws Exception
	{
		final NXCSession session = connect();
		
		DciValue[] list = session.getLastValues(12);
		assertEquals(true, list.length > 0);
		
		boolean statusFound = false;
		for(int i = 0; i < list.length; i++)
			if ((list[i].getName().equalsIgnoreCase("Status")) && (list[i].getSource() == DataCollectionItem.INTERNAL))
				statusFound = true;
		assertEquals(true, statusFound);
		
		session.disconnect();
	}
}
