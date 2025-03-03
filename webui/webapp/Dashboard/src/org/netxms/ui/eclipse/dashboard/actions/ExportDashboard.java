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
package org.netxms.ui.eclipse.dashboard.actions;

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.DciInfo;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.xml.XMLTools;
import org.netxms.ui.eclipse.console.DownloadServiceHandler;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementLayout;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import com.google.gson.Gson;

/**
 * Export dashboard configuration
 */
public class ExportDashboard implements IObjectActionDelegate
{
	private Dashboard dashboard = null;
	private IWorkbenchPart wbPart = null;
	
   /**
    * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
    */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		wbPart = targetPart;
	}

   /**
    * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
    */
	@Override
	public void run(IAction action)
	{
		if (dashboard == null)
			return;
		
		final Set<Long> objects = new HashSet<Long>();
		final Map<Long, Long> items = new HashMap<Long, Long>();

		final StringBuilder xml = new StringBuilder("<dashboard>\n\t<name>"); //$NON-NLS-1$
		xml.append(dashboard.getObjectName());
		xml.append("</name>\n\t<columns>"); //$NON-NLS-1$
		xml.append(dashboard.getNumColumns());
      xml.append("</columns>\n\t<flags>"); //$NON-NLS-1$
      xml.append(dashboard.getFlags());
      xml.append("</flags>\n\t<autoBindFlags>"); //$NON-NLS-1$
      xml.append(dashboard.getAutoBindFlags());
      xml.append("</autoBindFlags>\n\t<autoBindFilter>"); //$NON-NLS-1$
      xml.append(dashboard.getAutoBindFilter().replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"));
      xml.append("</autoBindFilter>\n\t<elements>\n"); //$NON-NLS-1$
		for(DashboardElement e : dashboard.getElements())
		{
         Gson gson = new Gson();
         DashboardElementLayout layout = gson.fromJson(e.getLayout(), DashboardElementLayout.class);
			DashboardElementConfig config = (DashboardElementConfig)Platform.getAdapterManager().getAdapter(e, DashboardElementConfig.class);
			if (config != null)
			{
            try
            {
               final StringBuilder element = new StringBuilder();
               element.append("\t\t<dashboardElement>\n\t\t\t<type>");
               element.append(e.getType());
               element.append("</type>\n");
               element.append(XMLTools.serialize(layout));
               element.append('\n');
               element.append(config.createXml()); 
               element.append("\n\t\t</dashboardElement>\n");
               
               objects.addAll(config.getObjects());
               items.putAll(config.getDataCollectionItems());
               
               xml.append(element);
            }
            catch(Exception e1)
            {
               // TODO add logging of missing element due to error 
               e1.printStackTrace();
            }   
			}
		}
		xml.append("\t</elements>\n"); //$NON-NLS-1$

      final NXCSession session = ConsoleSharedData.getSession();
		new ConsoleJob(Messages.get().ExportDashboard_JobTitle, wbPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				// Add object ID mapping
				xml.append("\t<objectMap>\n"); //$NON-NLS-1$
				for(Long id : objects)
				{
					AbstractObject o = session.findObjectById(id);
					if (o != null)
					{
						xml.append("\t\t<object id=\""); //$NON-NLS-1$
						xml.append(id);
						xml.append("\" class=\""); //$NON-NLS-1$
						xml.append(o.getObjectClass());
						xml.append("\">"); //$NON-NLS-1$
						xml.append(o.getObjectName());
						xml.append("</object>\n"); //$NON-NLS-1$
					}
				}
				xml.append("\t</objectMap>\n\t<dciMap>\n"); //$NON-NLS-1$

				// Add DCI ID mapping
		      List<Long> nodeList = new ArrayList<Long>();
		      List<Long> dciList = new ArrayList<Long>();
				for(Entry<Long, Long> dci : items.entrySet())
				{
               if ((dci.getKey() != 0) && (dci.getValue() != 0)) // ignore template entries
               {
                  dciList.add(dci.getKey());
                  nodeList.add(dci.getValue());
               }
				}
				Map<Long, DciInfo> names = session.dciIdsToNames(nodeList, dciList);
				for(int i = 0; i < names.size(); i++)
				{
					xml.append("\t\t<dci id=\""); //$NON-NLS-1$
					xml.append(dciList.get(i));
					xml.append("\" node=\""); //$NON-NLS-1$
					xml.append(nodeList.get(i));
					xml.append("\">"); //$NON-NLS-1$
					xml.append(names.get(dciList.get(i)).displayName);
					xml.append("</dci>\n"); //$NON-NLS-1$
				}
				xml.append("\t</dciMap>\n</dashboard>\n"); //$NON-NLS-1$

            final File dashboardFile = File.createTempFile("export_dashboard_" + dashboard.getObjectId(), "_" + System.currentTimeMillis()); 
				OutputStreamWriter out = new OutputStreamWriter(new FileOutputStream(dashboardFile), "UTF-8"); //$NON-NLS-1$
				try
				{
					out.write(xml.toString());
				}
				finally
				{
					out.close();
					if (dashboardFile.length() > 0)
					{
   	            DownloadServiceHandler.addDownload(dashboardFile.getName(), dashboard.getObjectName() + ".xml", dashboardFile, "application/octet-stream");
   	            runInUIThread(new Runnable() {
   	               @Override
   	               public void run()
   	               {
   	                  DownloadServiceHandler.startDownload(dashboardFile.getName());
   	               }
   	            });
					}
				}				
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().ExportDashboard_ErrorText;
			}
		}.start();
	}

   /**
    * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
    */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		Object obj;
		if ((selection instanceof IStructuredSelection) &&
		    (((IStructuredSelection)selection).size() == 1) &&
			 ((obj = ((IStructuredSelection)selection).getFirstElement()) instanceof Dashboard))
		{
			dashboard = (Dashboard)obj;
		}
		else
		{
			dashboard = null;
		}
		action.setEnabled(dashboard != null);
	}
}
