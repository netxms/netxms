/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Cache for DCI summary tables
 */
public class SummaryTablesCache
{
	private Map<Integer, DciSummaryTableDescriptor> tables = new HashMap<Integer, DciSummaryTableDescriptor>();
	private NXCSession session;
	private Display display;
	
   /**
    * Attach session to cache
    * 
    * @param session
    */
   public static void attachSession(Display display, NXCSession session)
   {
      SummaryTablesCache instance = new SummaryTablesCache(display, session);
      ConsoleSharedData.setProperty(display, "SummaryTablesCache", instance);
   }

   /**
    * Get cache instance
    * 
    * @return
    */
   public static SummaryTablesCache getInstance()
   {
      return (SummaryTablesCache)ConsoleSharedData.getProperty("SummaryTablesCache");
   }

   /**
	 * Initialize object tools cache. Should be called when connection with
	 * the server already established.
	 */
	private SummaryTablesCache(Display display, NXCSession session)
	{
		this.session = session;
      this.display = display;
		
		reload();
		
		session.addListener(new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				switch(n.getCode())
				{
					case NXCNotification.DCI_SUMMARY_TABLE_UPDATED:
						onTableChange((int)n.getSubCode());
						break;
					case NXCNotification.DCI_SUMMARY_TABLE_DELETED:
						onTableDelete((int)n.getSubCode());
						break;
				}
			}
		});
	}
	
	/**
	 * Reload tables from server
	 */
	private void reload()
	{
		try
		{
			List<DciSummaryTableDescriptor> list = session.listDciSummaryTables();
			synchronized(tables)
			{
				tables.clear();
				for(DciSummaryTableDescriptor d : list)
				{
					tables.put(d.getId(), d);
				}
			}
			SourceProvider p = SourceProvider.getInstance(display);
			if (p != null)
			{
			   p.update();
			}
		}
		catch(Exception e)
		{
		   Activator.logError("Exception in SummaryTablesCache.reload()", e);
		}
	}
	
	/**
	 * Handler for table change
	 * 
	 * @param tableId ID of changed table
	 */
	private void onTableChange(final int tableId)
	{
		new Thread() {
			@Override
			public void run()
			{
				reload();
			}
		}.start();
	}

	/**
	 * Handler for table deletion
	 * 
	 * @param tableId ID of deleted table
	 */
	private void onTableDelete(final int tableId)
	{
		synchronized(tables)
		{
			tables.remove(tableId);
		}
		SourceProvider.getInstance(display).update();
	}
	
	/**
	 * Get current set of DCI summary tables. Returned array is a copy of
	 * cache content. 
	 * 
	 * @return current set of DCI summary tables
	 */
	public DciSummaryTableDescriptor[] getTables()
	{
		synchronized(tables)
		{
			return tables.values().toArray(new DciSummaryTableDescriptor[tables.values().size()]);
		}
	}
	
	/**
	 * @return
	 */
	public boolean isEmpty()
	{
      synchronized(tables)
      {
         return tables.isEmpty();
      }
	}
}
