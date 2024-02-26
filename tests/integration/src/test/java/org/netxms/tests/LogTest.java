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
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertNotNull;
import java.util.ArrayList;
import java.util.Collection;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.client.log.LogFilter;
import org.netxms.client.log.OrderingColumn;

/**
 * @author Victor
 *
 */
public class LogTest extends AbstractSessionTest
{
   @Test
	public void testOpenLog() throws Exception
	{
		final NXCSession session = connect();
		
		final Log log = session.openServerLog("EventLog");
		assertNotNull(log);
		log.close();
	}

   @Test
	public void testLogInfo() throws Exception
	{
		final NXCSession session = connect();
		
		final Log log = session.openServerLog("EventLog");
		
		Collection<LogColumn> columns = log.getColumns();
		for(LogColumn c : columns)
		{
			System.out.println(c.getName() + ":" + c.getType() + ":" + c.getDescription());
		}
		
		log.close();
	}
   
   @Test
	public void testAuditLog() throws Exception
	{
		final NXCSession session = connect();
		
		final Log log = session.openServerLog("AuditLog");
		
		final LogFilter filter = new LogFilter();
		ArrayList<OrderingColumn> orderingColumns = new ArrayList<OrderingColumn>(1);
		orderingColumns.add(new OrderingColumn(log.getColumn("record_id"), true));
		filter.setOrderingColumns(orderingColumns);
		
		log.query(filter);
		System.out.println("Number of records in result set: " + log.getNumRecords());
		
		final Table data = log.retrieveData(0, 15);
		for(int i = 0; i < data.getRowCount(); i++)
		{
			TableRow row = data.getRow(i);
			System.out.println("ROW " + i + ": (" + row.get(0).getValue() + ") " + row.get(5).getValue());
		}
		
		log.close();
	}
}
