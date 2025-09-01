/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.widgets;

import java.util.regex.Pattern;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.modules.dashboards.config.TableValueConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.modules.datacollection.widgets.TableValueViewer;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.Gson;

/**
 * "Table value" element for dashboard
 */
public class TableValueElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(TableValueElement.class);
   
	private TableValueConfig config;
	private TableValueViewer viewer;

	/**
	 * @param parent
	 * @param element
	 * @param viewPart
	 */
   public TableValueElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, element, view);

		try
		{
         config = new Gson().fromJson(element.getData(), TableValueConfig.class);
		}
		catch(Exception e)
		{
         logger.error("Cannot parse dashboard element configuration", e);
			config = new TableValueConfig();
		}

      processCommonSettings(config);

      viewer = new TableValueViewer(getContentArea(), SWT.NONE, view, parent.getDashboardObject().getGuid().toString(), false);
      viewer.setSortColumn(config.getSortColumn(), config.getSortDirection());
      if (config.getObjectId() == AbstractObject.CONTEXT)
      {
         configureContext();
      }
      else
      {
         viewer.setObject(config.getObjectId(), config.getDciId());
         viewer.refresh(null);
      }

      final ViewRefreshController refreshController = new ViewRefreshController(view, config.getRefreshRate(), new Runnable() {
			@Override
			public void run()
			{
				if (TableValueElement.this.isDisposed())
					return;

				viewer.refresh(null);
			}
		});

		addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            refreshController.dispose();
         }
      });
	}

   /**
    * Configure context if element is context-aware
    */
   private void configureContext()
   {
      final AbstractObject contextObject = getContext();
      if (contextObject == null)
         return;

      final NXCSession session = Registry.getSession();
      new Job("Configuring context", view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            DciValue[] dciList = session.getLastValues(contextObject.getObjectId());
            Pattern namePattern = Pattern.compile(config.getDciName());
            Pattern descriptionPattern = Pattern.compile(config.getDciDescription());
            for(DciValue dciInfo : dciList)
            {
               if (dciInfo.getDcObjectType() != DataCollectionObject.DCO_TYPE_TABLE)
                  continue;

               if ((!config.getDciName().isEmpty() && namePattern.matcher(dciInfo.getName()).find()) ||
                   (!config.getDciDescription().isEmpty() && descriptionPattern.matcher(dciInfo.getDescription()).find()))
               {
                  runInUIThread(() -> {
                     if (!viewer.isDisposed())
                     {
                        viewer.setObject(dciInfo.getNodeId(), dciInfo.getId());
                        viewer.refresh(null);
                     }
                  });
                  break;
               }
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot read DCI data from server";
         }
      }.start();
   }
}
