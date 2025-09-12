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
import org.netxms.client.objects.DashboardBase;
import org.netxms.client.objects.DashboardTemplate;
import org.netxms.client.xml.XMLTools;
import org.netxms.nxmc.DownloadServiceHandler;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfigFactory;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementLayout;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;
import com.google.gson.Gson;

/**
 * Export dashboard configuration
 */
public class ExportDashboardAction extends ObjectAction<DashboardBase>
{
   private final I18n i18n = LocalizationHelper.getI18n(ExportDashboardAction.class);
   private static final Logger logger = LoggerFactory.getLogger(ExportDashboardAction.class);

   /**
    * Create action for linking asset to object.
    *
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public ExportDashboardAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(DashboardBase.class, LocalizationHelper.getI18n(ExportDashboardAction.class).tr("&Export..."), viewPlacement, selectionProvider);
      setImageDescriptor(ResourceManager.getImageDescriptor("icons/export.png"));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(List<DashboardBase> selection)
   {
      final DashboardBase dashboard = selection.get(0);

      if ((dashboard instanceof Dashboard) && ((Dashboard)dashboard).isTemplateInstance())
         if (!MessageDialogHelper.openQuestion(getShell(), i18n.tr("Warning"), 
               i18n.tr("This instance of a Template Dashboard will be exported as a regular Dashboard. Continue?")))
            return;
      
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
      xml.append("</autoBindFilter>\n");
      
      if (dashboard instanceof Dashboard)
      {
         xml.append("\t<isTemplate>");
         xml.append(false);
         xml.append("</isTemplate>\n\n");
      }
      else if (dashboard instanceof DashboardTemplate)
      {
         xml.append("\t<isTemplate>");
         xml.append(true);
         xml.append("</isTemplate>\n\n");
         
         String nameTemplate = ((DashboardTemplate)dashboard).getNameTemplate();
         xml.append("\t<nameTemplate>");
         xml.append(nameTemplate.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"));
         xml.append("</nameTemplate>\n\n");
      }

      xml.append("\t<elements>\n");      
		for(DashboardElement e : dashboard.getElements())
		{
         Gson gson = new Gson();
         DashboardElementLayout layout = gson.fromJson(e.getLayout(), DashboardElementLayout.class);
         DashboardElementConfig config = DashboardElementConfigFactory.create(e);
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
               logger.error("Failed to convert dashboard element", e1);
            }           
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
      return (selection.size() == 1) && (selection.getFirstElement() instanceof DashboardBase); 
   }
}
