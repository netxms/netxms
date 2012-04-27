/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Export dashboard configuration
 */
public class ExportDashboard implements IObjectActionDelegate
{
	private Dashboard dashboard = null;
	private IWorkbenchPart wbPart = null;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		wbPart = targetPart;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
		if (dashboard == null)
			return;
		
		FileDialog dlg = new FileDialog(wbPart.getSite().getShell(), SWT.SAVE);
		dlg.setText("Select File");
		dlg.setFilterExtensions(new String[] { "*.xml", "*.*" });
		dlg.setFilterNames(new String[] { "XML files", "All files" });
		final String fileName = dlg.open();
		if (fileName == null)
			return;
		
		final Set<Long> objects = new HashSet<Long>();
		final Map<Long, Long> items = new HashMap<Long, Long>();

		final StringBuilder xml = new StringBuilder("<dashboard>\n\t<name>");
		xml.append(dashboard.getObjectName());
		xml.append("</name>\n\t<columns>");
		xml.append(dashboard.getNumColumns());
		xml.append("</columns>\n\t<elements>\n");
		for(DashboardElement e : dashboard.getElements())
		{
			xml.append("\t\t<dashboardElement>\n\t\t\t<type>");
			xml.append(e.getType());
			xml.append("</type>\n");
			xml.append(e.getLayout());
			xml.append('\n');
			xml.append(e.getData());
			xml.append("\n\t\t</dashboardElement>\n");

			DashboardElementConfig config = (DashboardElementConfig)Platform.getAdapterManager().getAdapter(e, DashboardElementConfig.class);
			if (config != null)
			{
				objects.addAll(config.getObjects());
				items.putAll(config.getDataCollectionItems());
			}
		}
		xml.append("\t</elements>\n");
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Export dashbnoard configuration", wbPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				// Add object ID mapping
				xml.append("\t<objectMap>\n");
				for(Long id : objects)
				{
					GenericObject o = session.findObjectById(id);
					if (o != null)
					{
						xml.append("\t\t<object id=\"");
						xml.append(id);
						xml.append("\" class=\"");
						xml.append(o.getObjectClass());
						xml.append("\">");
						xml.append(o.getObjectName());
						xml.append("</object>\n");
					}
				}
				xml.append("\t</objectMap>\n\t<dciMap>\n");
		
				// Add DCI ID mapping
				long[] nodeList = new long[items.size()];
				long[] dciList = new long[items.size()];
				int pos = 0;
				for(Entry<Long, Long> dci : items.entrySet())
				{
					dciList[pos] = dci.getKey();
					nodeList[pos] = dci.getValue();
					pos++;
				}
				String[] names = session.resolveDciNames(nodeList, dciList);
				for(int i = 0; i < names.length; i++)
				{
					xml.append("\t\t<dci id=\"");
					xml.append(dciList[i]);
					xml.append("\" node=\"");
					xml.append(nodeList[i]);
					xml.append("\">");
					xml.append(names[i]);
					xml.append("</object>\n");
				}
				xml.append("\t</dciMap>\n</dashboard>\n");
				
				OutputStreamWriter out = new OutputStreamWriter(new FileOutputStream(fileName), "UTF-8");
				try
				{
					out.write(xml.toString());
				}
				finally
				{
					out.close();
				}
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Cannot save export file";
			}
		}.start();
	}

	/* (non-Javadoc)
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
