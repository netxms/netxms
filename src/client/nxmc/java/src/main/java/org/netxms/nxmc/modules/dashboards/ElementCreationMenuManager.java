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
package org.netxms.nxmc.modules.dashboards;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.widgets.DashboardControl;
import org.xnap.commons.i18n.I18n;

/**
 * Helper class for building dashboard element creation menu
 */
public class ElementCreationMenuManager extends MenuManager
{
   private final I18n i18n = LocalizationHelper.getI18n(ElementCreationMenuManager.class);

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
      MenuManager chartsMenu = new MenuManager(i18n.tr("&Charts"));
      addTypeToSelectionMenu(chartsMenu, i18n.tr("Line chart"), DashboardElement.LINE_CHART);
      addTypeToSelectionMenu(chartsMenu, i18n.tr("Bar chart"), DashboardElement.BAR_CHART);
      addTypeToSelectionMenu(chartsMenu, i18n.tr("Pie chart"), DashboardElement.PIE_CHART);
      addTypeToSelectionMenu(chartsMenu, i18n.tr("Gauge chart"), DashboardElement.DIAL_CHART);
      chartsMenu.add(new Separator());
      addTypeToSelectionMenu(chartsMenu, i18n.tr("Bar chart for table DCI"), DashboardElement.TABLE_BAR_CHART);
      addTypeToSelectionMenu(chartsMenu, i18n.tr("Pie chart for table DCI"), DashboardElement.TABLE_PIE_CHART);
      chartsMenu.add(new Separator());
      addTypeToSelectionMenu(chartsMenu, i18n.tr("Scripted bar chart"), DashboardElement.SCRIPTED_BAR_CHART);
      addTypeToSelectionMenu(chartsMenu, i18n.tr("Scripted pie chart"), DashboardElement.SCRIPTED_PIE_CHART);
      chartsMenu.add(new Separator());
      addTypeToSelectionMenu(chartsMenu, i18n.tr("Status chart"), DashboardElement.STATUS_CHART);
      addTypeToSelectionMenu(chartsMenu, i18n.tr("Availability chart"), DashboardElement.AVAILABLITY_CHART);
      add(chartsMenu);

      /* tables */
      MenuManager tablesMenu = new MenuManager(i18n.tr("&Tables"));
      addTypeToSelectionMenu(tablesMenu, i18n.tr("Alarm viewer"), DashboardElement.ALARM_VIEWER);
      addTypeToSelectionMenu(tablesMenu, i18n.tr("Table value"), DashboardElement.TABLE_VALUE);
      addTypeToSelectionMenu(tablesMenu, i18n.tr("DCI summary table"), DashboardElement.DCI_SUMMARY_TABLE);
      addTypeToSelectionMenu(tablesMenu, i18n.tr("Object query"), DashboardElement.OBJECT_QUERY);
      add(tablesMenu);

      /* maps */
      MenuManager mapsMenu = new MenuManager(i18n.tr("&Maps"));
      addTypeToSelectionMenu(mapsMenu, i18n.tr("Network map"), DashboardElement.NETWORK_MAP);
      addTypeToSelectionMenu(mapsMenu, i18n.tr("Service components map"), DashboardElement.SERVICE_COMPONENTS);
      addTypeToSelectionMenu(mapsMenu, i18n.tr("Status map"), DashboardElement.STATUS_MAP);
      addTypeToSelectionMenu(mapsMenu, i18n.tr("Geo map"), DashboardElement.GEO_MAP);
      add(mapsMenu);

      /* monitors */
      MenuManager monitorsMenu = new MenuManager(i18n.tr("M&onitors"));
      addTypeToSelectionMenu(monitorsMenu, i18n.tr("&Event monitor"), DashboardElement.EVENT_MONITOR);
      addTypeToSelectionMenu(monitorsMenu, i18n.tr("&File monitor"), DashboardElement.FILE_MONITOR);
      addTypeToSelectionMenu(monitorsMenu, i18n.tr("SNMP &trap monitor"), DashboardElement.SNMP_TRAP_MONITOR);
      addTypeToSelectionMenu(monitorsMenu, i18n.tr("&Syslog monitor"), DashboardElement.SYSLOG_MONITOR);
      add(monitorsMenu);

      /* others */
      MenuManager othersMenu = new MenuManager(i18n.tr("&Others"));
      addTypeToSelectionMenu(othersMenu, i18n.tr("Status indicator"), DashboardElement.STATUS_INDICATOR);
      addTypeToSelectionMenu(othersMenu, i18n.tr("Port view"), DashboardElement.PORT_VIEW);
      addTypeToSelectionMenu(othersMenu, i18n.tr("Rack diagram"), DashboardElement.RACK_DIAGRAM);
      addTypeToSelectionMenu(othersMenu, i18n.tr("Object Tools"), DashboardElement.OBJECT_TOOLS);
      addTypeToSelectionMenu(othersMenu, i18n.tr("Custom widget"), DashboardElement.CUSTOM);
      add(othersMenu);

      /* other top level items */
      add(new Separator());
      addTypeToSelectionMenu(this, i18n.tr("&Dashboard"), DashboardElement.DASHBOARD);
      addTypeToSelectionMenu(this, i18n.tr("&Web page"), DashboardElement.WEB_PAGE);
      add(new Separator());
      addTypeToSelectionMenu(this, i18n.tr("&Label"), DashboardElement.LABEL);
      addTypeToSelectionMenu(this, i18n.tr("&Separator"), DashboardElement.SEPARATOR);
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
            handler.elementCreated(new DashboardElement(type, getDefaultElementConfig(type)));
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
            return "<element>\n</element>";
      }
   }
}
