/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.propertypages.AbstractChart;
import org.netxms.nxmc.modules.dashboards.propertypages.AlarmViewer;
import org.netxms.nxmc.modules.dashboards.propertypages.AvailabilityChart;
import org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage;
import org.netxms.nxmc.modules.dashboards.propertypages.DataSources;
import org.netxms.nxmc.modules.dashboards.propertypages.DciSummaryTable;
import org.netxms.nxmc.modules.dashboards.propertypages.EmbeddedDashboard;
import org.netxms.nxmc.modules.dashboards.propertypages.EventMonitor;
import org.netxms.nxmc.modules.dashboards.propertypages.FileMonitor;
import org.netxms.nxmc.modules.dashboards.propertypages.Gauge;
import org.netxms.nxmc.modules.dashboards.propertypages.GeoMap;
import org.netxms.nxmc.modules.dashboards.propertypages.LabelProperties;
import org.netxms.nxmc.modules.dashboards.propertypages.Layout;
import org.netxms.nxmc.modules.dashboards.propertypages.NetworkMap;
import org.netxms.nxmc.modules.dashboards.propertypages.ObjectDetailsPropertyList;
import org.netxms.nxmc.modules.dashboards.propertypages.ObjectDetailsQuery;
import org.netxms.nxmc.modules.dashboards.propertypages.ObjectStatusChart;
import org.netxms.nxmc.modules.dashboards.propertypages.ObjectTools;
import org.netxms.nxmc.modules.dashboards.propertypages.PortView;
import org.netxms.nxmc.modules.dashboards.propertypages.RackDiagram;
import org.netxms.nxmc.modules.dashboards.propertypages.ScriptedChart;
import org.netxms.nxmc.modules.dashboards.propertypages.SeparatorProperties;
import org.netxms.nxmc.modules.dashboards.propertypages.ServiceComponents;
import org.netxms.nxmc.modules.dashboards.propertypages.SnmpTrapMonitor;
import org.netxms.nxmc.modules.dashboards.propertypages.StatusIndicator;
import org.netxms.nxmc.modules.dashboards.propertypages.StatusIndicatorElements;
import org.netxms.nxmc.modules.dashboards.propertypages.StatusIndicatorScript;
import org.netxms.nxmc.modules.dashboards.propertypages.StatusMap;
import org.netxms.nxmc.modules.dashboards.propertypages.SyslogMonitor;
import org.netxms.nxmc.modules.dashboards.propertypages.TableComparisonChart;
import org.netxms.nxmc.modules.dashboards.propertypages.TableDataSource;
import org.netxms.nxmc.modules.dashboards.propertypages.TableValue;
import org.netxms.nxmc.modules.dashboards.propertypages.WebPage;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Manager for dashboard element properties
 */
public class DashboardElementPropertiesManager
{
   private static final Logger logger = LoggerFactory.getLogger(DashboardElementPropertiesManager.class);

   private static Set<Class<? extends DashboardElementPropertyPage>> pageClasses = new HashSet<Class<? extends DashboardElementPropertyPage>>();
   static
   {
      pageClasses.add(AbstractChart.class);
      pageClasses.add(AlarmViewer.class);
      pageClasses.add(AvailabilityChart.class);
      pageClasses.add(DataSources.class);
      pageClasses.add(DciSummaryTable.class);
      pageClasses.add(EmbeddedDashboard.class);
      pageClasses.add(EventMonitor.class);
      pageClasses.add(FileMonitor.class);
      pageClasses.add(Gauge.class);
      pageClasses.add(GeoMap.class);
      pageClasses.add(LabelProperties.class);
      pageClasses.add(Layout.class);
      pageClasses.add(NetworkMap.class);
      pageClasses.add(ObjectDetailsPropertyList.class);
      pageClasses.add(ObjectDetailsQuery.class);
      pageClasses.add(ObjectStatusChart.class);
      pageClasses.add(ObjectTools.class);
      pageClasses.add(PortView.class);
      pageClasses.add(RackDiagram.class);
      pageClasses.add(ScriptedChart.class);
      pageClasses.add(SeparatorProperties.class);
      pageClasses.add(ServiceComponents.class);
      pageClasses.add(SnmpTrapMonitor.class);
      pageClasses.add(StatusIndicator.class);
      pageClasses.add(StatusIndicatorElements.class);
      pageClasses.add(StatusIndicatorScript.class);
      pageClasses.add(StatusMap.class);
      pageClasses.add(SyslogMonitor.class);
      pageClasses.add(TableComparisonChart.class);
      pageClasses.add(TableDataSource.class);
      pageClasses.add(TableValue.class);
      pageClasses.add(WebPage.class);
   }

   /**
    * Open properties dialog for dashboard element.
    * 
    * @param elementConfig element's configuration
    * @param shell parent shell
    * @return true if OK was pressed
    */
   public static boolean openElementPropertiesDialog(DashboardElementConfig elementConfig, Shell shell)
   {
      List<DashboardElementPropertyPage> pages = new ArrayList<DashboardElementPropertyPage>(pageClasses.size());
      for(Class<? extends DashboardElementPropertyPage> c : pageClasses)
      {
         try
         {
            DashboardElementPropertyPage p = c.getConstructor(DashboardElementConfig.class).newInstance(elementConfig);
            if (p.isVisible())
               pages.add(p);
         }
         catch(Exception e)
         {
            logger.error("Error instantiating dashboard element property page", e);
         }
      }
      pages.sort(new Comparator<DashboardElementPropertyPage>() {
         @Override
         public int compare(DashboardElementPropertyPage p1, DashboardElementPropertyPage p2)
         {
            int rc = p1.getPriority() - p2.getPriority();
            return (rc != 0) ? rc : p1.getTitle().compareToIgnoreCase(p2.getTitle());
         }
      });

      PreferenceManager pm = new PreferenceManager();
      for(DashboardElementPropertyPage p : pages)
         pm.addToRoot(new PreferenceNode(p.getId(), p));

      PreferenceDialog dlg = new PreferenceDialog(shell, pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText(LocalizationHelper.getI18n(DashboardElementPropertiesManager.class).tr("Dashboard Element Properties"));
         }
      };
      dlg.setBlockOnOpen(true);
      return dlg.open() == Window.OK;
   }
}
