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
package org.netxms.ui.eclipse.dashboard.widgets;

import java.util.regex.Pattern;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.TableValueConfig;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.widgets.TableValueViewer;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ViewRefreshController;
import com.google.gson.Gson;

/**
 * "Table value" element for dashboard
 */
public class TableValueElement extends ElementWidget
{
	private TableValueConfig config;
	private TableValueViewer viewer;

	/**
	 * @param parent
	 * @param element
	 * @param viewPart
	 */
	public TableValueElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);

		try
		{
         config = new Gson().fromJson(element.getData(), TableValueConfig.class);
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new TableValueConfig();
		}

      processCommonSettings(config);

      viewer = new TableValueViewer(getContentArea(), SWT.NONE, viewPart, parent.getDashboardObject().getGuid().toString(), true);
      if (config.getObjectId() == AbstractObject.CONTEXT)
      {
         configureContext();
      }
      else
      {
         viewer.setObject(config.getObjectId(), config.getDciId());
         viewer.refresh(null);
      }

		final ViewRefreshController refreshController = new ViewRefreshController(viewPart, config.getRefreshRate(), new Runnable() {
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

      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Configuring context", viewPart, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
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
                  runInUIThread(new Runnable() {
                     @Override
                     public void run()
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
