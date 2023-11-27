/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.dashboards;

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
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.DciInfo;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.nxmc.DownloadServiceHandler;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfigFactory;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Export dashboard configuration
 */
public class ExportDashboardAction extends ObjectAction<Dashboard>
{
   private final I18n i18n = LocalizationHelper.getI18n(ExportDashboardAction.class);

   /**
    * Create action for linking asset to object.
    *
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public ExportDashboardAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(Dashboard.class, LocalizationHelper.getI18n(ExportDashboardAction.class).tr("&Export..."), viewPlacement, selectionProvider);
      setImageDescriptor(ResourceManager.getImageDescriptor("icons/export.png"));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(List<Dashboard> selection)
   {
      final Dashboard dashboard = selection.get(0);
      
		final Set<Long> objects = new HashSet<Long>();
		final Map<Long, Long> items = new HashMap<Long, Long>();

      final StringBuilder xml = new StringBuilder("<dashboard>\n\t<name>");
		xml.append(dashboard.getObjectName());
      xml.append("</name>\n\t<columns>");
		xml.append(dashboard.getNumColumns());
      xml.append("</columns>\n\t<flags>");
      xml.append(dashboard.getFlags());
      xml.append("</flags>\n\t<autoBindFlags>");
      xml.append(dashboard.getAutoBindFlags());
      xml.append("</autoBindFlags>\n\t<autoBindFilter>");
      xml.append(dashboard.getAutoBindFilter().replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"));
      xml.append("</autoBindFilter>\n\t<elements>\n");
		for(DashboardElement e : dashboard.getElements())
		{
         xml.append("\t\t<dashboardElement>\n\t\t\t<type>");
			xml.append(e.getType());
         xml.append("</type>\n");
			xml.append(e.getLayout());
			xml.append('\n');
			xml.append(e.getData());
         xml.append("\n\t\t</dashboardElement>\n");

	      DashboardElementConfig config = DashboardElementConfigFactory.create(e);
			if (config != null)
			{
				objects.addAll(config.getObjects());
				items.putAll(config.getDataCollectionItems());
			}
		}
      xml.append("\t</elements>\n");

      final NXCSession session = Registry.getSession();
		new Job(i18n.tr("Export dashboard configuration"), null, getMessageArea()) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				// Add object ID mapping
            xml.append("\t<objectMap>\n");
				for(Long id : objects)
				{
					AbstractObject o = session.findObjectById(id);
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
               xml.append("\t\t<dci id=\"");
					xml.append(dciList.get(i));
               xml.append("\" node=\"");
					xml.append(nodeList.get(i));
               xml.append("\">");
					xml.append(names.get(dciList.get(i)).displayName);
               xml.append("</dci>\n");
				}
            xml.append("\t</dciMap>\n</dashboard>\n");

            final File dashboardFile = File.createTempFile("export_dashboard_" + dashboard.getObjectId(), "_" + System.currentTimeMillis()); 
				OutputStreamWriter out = new OutputStreamWriter(new FileOutputStream(dashboardFile), "UTF-8");
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
				return i18n.tr("Cannot save export file");
			}
		}.start();
	}

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#isValidForSelection(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   public boolean isValidForSelection(IStructuredSelection selection)
   {
      return (selection.size() == 1) && (selection.getFirstElement() instanceof Dashboard);
   }
}
