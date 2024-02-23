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
package org.netxms.nxmc.modules.datacollection;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.Table;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.views.SummaryTable;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Cache for DCI summary tables
 */
public class SummaryTablesCache
{
   private static final Logger logger = LoggerFactory.getLogger(SummaryTablesCache.class);

   private Map<Integer, DciSummaryTableDescriptor> tables = new HashMap<Integer, DciSummaryTableDescriptor>();
   private NXCSession session;

   /**
    * Attach session to cache
    * 
    * @param session session object
    */
   public static void attachSession(Display display, NXCSession session)
   {
      SummaryTablesCache instance = new SummaryTablesCache(session);
      Registry.setSingleton(display, SummaryTablesCache.class, instance);
   }

   /**
    * Get cache instance
    * 
    * @return
    */
   public static SummaryTablesCache getInstance()
   {
      return (SummaryTablesCache)Registry.getSingleton(SummaryTablesCache.class);
   }

   /**
    * Initialize object tools cache. Should be called when connection with the server already established.
    */
   private SummaryTablesCache(NXCSession session)
   {
      this.session = session;

		reload();

		session.addListener(new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				switch(n.getCode())
				{
					case SessionNotification.DCI_SUMMARY_TABLE_UPDATED:
						onTableChange((int)n.getSubCode());
						break;
					case SessionNotification.DCI_SUMMARY_TABLE_DELETED:
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
		}
		catch(Exception e)
		{
         logger.error("Exception in SummaryTablesCache.reload()", e);
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
   public boolean isEmpty(boolean menuOnly)
	{
      synchronized(tables)
      {
         if (menuOnly)
         {
            for(DciSummaryTableDescriptor d : tables.values())
               if (!d.getMenuPath().isEmpty())
                  return false;
            return true;
         }
         return tables.isEmpty();
      }
	}

   /**
    * Query table using selected base object
    * 
    * @param baseObjectId
    * @param tableId
    */
   public static void queryTable(final long baseObjectId, long contextId, final int tableId, ViewPlacement viewPlacement)
   {
      final NXCSession session = Registry.getSession();
      I18n i18n = LocalizationHelper.getI18n(SummaryTablesCache.class);
      new Job(i18n.tr("Query DCI summary table"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Table results = session.queryDciSummaryTable(tableId, baseObjectId);
            runInUIThread(() -> {
               SummaryTable view = new SummaryTable(tableId, baseObjectId, contextId);
               viewPlacement.openView(view);
               view.setTable(results);
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read data for DCI summary table");
         }
      }.start();
   }
}
