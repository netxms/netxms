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
package org.netxms.ui.eclipse.dashboard;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.widgets.DashboardControl;

/**
 * Helper class for building dashboard element creation menu
 */
public class ElementCreationMenuManager extends MenuManager
{
   private ElementCreationHandler handler;

   public ElementCreationMenuManager(ElementCreationHandler handler)
   {
      super();
      this.handler = handler;
      fillMenu();
   }

   public ElementCreationMenuManager(String name, ElementCreationHandler handler)
   {
      super(name);
      this.handler = handler;
      fillMenu();
   }

   /**
    * Build menu.
    */
   private void fillMenu()
   {
      /* charts */
      MenuManager chartsMenu = new MenuManager("&Charts");
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_LineChart, DashboardElement.LINE_CHART);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_BarChart, DashboardElement.BAR_CHART);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_PieChart, DashboardElement.PIE_CHART);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_GaugeChart, DashboardElement.DIAL_CHART);
      chartsMenu.add(new Separator());
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_BarChartForTable, DashboardElement.TABLE_BAR_CHART);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_PieChartForTable, DashboardElement.TABLE_PIE_CHART);
      chartsMenu.add(new Separator());
      addTypeToSelectionMenu(chartsMenu, "Scripted bar chart", DashboardElement.SCRIPTED_BAR_CHART);
      addTypeToSelectionMenu(chartsMenu, "Scripted pie chart", DashboardElement.SCRIPTED_PIE_CHART);
      chartsMenu.add(new Separator());
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_StatusChart, DashboardElement.STATUS_CHART);
      addTypeToSelectionMenu(chartsMenu, Messages.get().AddDashboardElementDlg_AvailabilityChart, DashboardElement.AVAILABLITY_CHART);
      add(chartsMenu);

      /* tables */
      MenuManager tablesMenu = new MenuManager("&Tables");
      addTypeToSelectionMenu(tablesMenu, Messages.get().AddDashboardElementDlg_AlarmViewer, DashboardElement.ALARM_VIEWER);
      addTypeToSelectionMenu(tablesMenu, Messages.get().AddDashboardElementDlg_TableValue, DashboardElement.TABLE_VALUE);
      addTypeToSelectionMenu(tablesMenu, Messages.get().AddDashboardElementDlg_DciSummaryTable, DashboardElement.DCI_SUMMARY_TABLE);
      addTypeToSelectionMenu(tablesMenu, "Object query", DashboardElement.OBJECT_QUERY);
      add(tablesMenu);

      /* maps */
      MenuManager mapsMenu = new MenuManager("&Maps");
      addTypeToSelectionMenu(mapsMenu, Messages.get().AddDashboardElementDlg_NetworkMap, DashboardElement.NETWORK_MAP);
      addTypeToSelectionMenu(mapsMenu, "Service components map", DashboardElement.SERVICE_COMPONENTS);
      addTypeToSelectionMenu(mapsMenu, Messages.get().AddDashboardElementDlg_StatusMap, DashboardElement.STATUS_MAP);
      addTypeToSelectionMenu(mapsMenu, Messages.get().AddDashboardElementDlg_GeoMap, DashboardElement.GEO_MAP);
      add(mapsMenu);

      /* monitors */
      MenuManager monitorsMenu = new MenuManager("M&onitors");
      addTypeToSelectionMenu(monitorsMenu, "&Event monitor", DashboardElement.EVENT_MONITOR);
      addTypeToSelectionMenu(monitorsMenu, "&File monitor", DashboardElement.FILE_MONITOR);
      addTypeToSelectionMenu(monitorsMenu, "SNMP &trap monitor", DashboardElement.SNMP_TRAP_MONITOR);
      addTypeToSelectionMenu(monitorsMenu, "&Syslog monitor", DashboardElement.SYSLOG_MONITOR);
      add(monitorsMenu);

      /* others */
      MenuManager othersMenu = new MenuManager("&Others");
      addTypeToSelectionMenu(othersMenu, Messages.get().AddDashboardElementDlg_StatusIndicator, DashboardElement.STATUS_INDICATOR);
      addTypeToSelectionMenu(othersMenu, "Port view", DashboardElement.PORT_VIEW);
      addTypeToSelectionMenu(othersMenu, "Rack diagram", DashboardElement.RACK_DIAGRAM);
      addTypeToSelectionMenu(othersMenu, "Object Tools", DashboardElement.OBJECT_TOOLS);
      addTypeToSelectionMenu(othersMenu, Messages.get().AddDashboardElementDlg_CustomWidget, DashboardElement.CUSTOM);
      add(othersMenu);

      /* other top level items */
      add(new Separator());
      addTypeToSelectionMenu(this, Messages.get().AddDashboardElementDlg_Dashboard, DashboardElement.DASHBOARD);
      addTypeToSelectionMenu(this, Messages.get().AddDashboardElementDlg_WebPage, DashboardElement.WEB_PAGE);
      add(new Separator());
      addTypeToSelectionMenu(this, Messages.get().AddDashboardElementDlg_Label, DashboardElement.LABEL);
      addTypeToSelectionMenu(this, Messages.get().AddDashboardElementDlg_Separator, DashboardElement.SEPARATOR);
   }

   /**
    * Add element type to selection menu.
    *
    * @param menu menu to add
    * @param name type name
    * @param type type code
    */
   private void addTypeToSelectionMenu(MenuManager manager, String name, int type)
   {
      manager.add(new Action(name) {
         @Override
         public void run()
         {
            handler.elementCreated(new DashboardElement(type, getDefaultElementConfig(type), 0));
         }
      });
   }

   /**
    * Get default configuration for given element type.
    *
    * @param type element type
    * @return default configuration
    */
   private static String getDefaultElementConfig(int type)
   {
      switch(type)
      {
         case DashboardElement.BAR_CHART:
         case DashboardElement.PIE_CHART:
         case DashboardElement.TUBE_CHART:
         case DashboardElement.SCRIPTED_BAR_CHART:
         case DashboardElement.SCRIPTED_PIE_CHART:
            return DashboardControl.DEFAULT_CHART_CONFIG;
         case DashboardElement.LINE_CHART:
            return DashboardControl.DEFAULT_LINE_CHART_CONFIG;
         case DashboardElement.DIAL_CHART:
            return DashboardControl.DEFAULT_DIAL_CHART_CONFIG;
         case DashboardElement.AVAILABLITY_CHART:
            return DashboardControl.DEFAULT_AVAILABILITY_CHART_CONFIG;
         case DashboardElement.TABLE_BAR_CHART:
         case DashboardElement.TABLE_PIE_CHART:
         case DashboardElement.TABLE_TUBE_CHART:
            return DashboardControl.DEFAULT_TABLE_CHART_CONFIG;
         case DashboardElement.LABEL:
            return DashboardControl.DEFAULT_LABEL_CONFIG;
         case DashboardElement.ALARM_VIEWER:
         case DashboardElement.EVENT_MONITOR:
         case DashboardElement.FILE_MONITOR:
         case DashboardElement.SYSLOG_MONITOR:
         case DashboardElement.SNMP_TRAP_MONITOR:
         case DashboardElement.STATUS_INDICATOR:
         case DashboardElement.STATUS_MAP:
         case DashboardElement.DASHBOARD:
         case DashboardElement.RACK_DIAGRAM:
            return DashboardControl.DEFAULT_OBJECT_REFERENCE_CONFIG;
         case DashboardElement.NETWORK_MAP:
         case DashboardElement.SERVICE_COMPONENTS:
            return DashboardControl.DEFAULT_NETWORK_MAP_CONFIG;
         case DashboardElement.GEO_MAP:
            return DashboardControl.DEFAULT_GEO_MAP_CONFIG;
         case DashboardElement.WEB_PAGE:
            return DashboardControl.DEFAULT_WEB_PAGE_CONFIG;
         case DashboardElement.TABLE_VALUE:
            return DashboardControl.DEFAULT_TABLE_VALUE_CONFIG;
         case DashboardElement.DCI_SUMMARY_TABLE:
            return DashboardControl.DEFAULT_SUMMARY_TABLE_CONFIG;
         default:
            return "<element>\n</element>"; //$NON-NLS-1$
      }
   }
}
