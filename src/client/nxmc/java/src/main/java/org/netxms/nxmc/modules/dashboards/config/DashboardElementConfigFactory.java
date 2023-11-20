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
package org.netxms.nxmc.modules.dashboards.config;

import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.xml.XMLTools;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Factory for dashboard element configurations.
 */
public class DashboardElementConfigFactory
{
   private static final Logger logger = LoggerFactory.getLogger(DashboardElementConfigFactory.class);

   /**
    * Create configuration object from dashboard element object.
    *
    * @param element dashboard element object
    * @return dashboard element configuration object or null on error
    */
   public static DashboardElementConfig create(DashboardElement element)
   {
      try
      {
         switch(element.getType())
         {
            case DashboardElement.ALARM_VIEWER:
               return XMLTools.createFromXml(AlarmViewerConfig.class, element.getData());
            case DashboardElement.AVAILABLITY_CHART:
               return XMLTools.createFromXml(AvailabilityChartConfig.class, element.getData());
            case DashboardElement.BAR_CHART:
            case DashboardElement.TUBE_CHART:
               return XMLTools.createFromXml(BarChartConfig.class, element.getData());
            case DashboardElement.CUSTOM:
               return XMLTools.createFromXml(CustomWidgetConfig.class, element.getData());
            case DashboardElement.DASHBOARD:
               return EmbeddedDashboardConfig.createFromXml(element.getData());
            case DashboardElement.DCI_SUMMARY_TABLE:
               return XMLTools.createFromXml(DciSummaryTableConfig.class, element.getData());
            case DashboardElement.DIAL_CHART:
               return XMLTools.createFromXml(GaugeConfig.class, element.getData());
            case DashboardElement.EVENT_MONITOR:
               return XMLTools.createFromXml(EventMonitorConfig.class, element.getData());
            case DashboardElement.FILE_MONITOR:
               return XMLTools.createFromXml(FileMonitorConfig.class, element.getData());
            case DashboardElement.GEO_MAP:
               return XMLTools.createFromXml(GeoMapConfig.class, element.getData());
            case DashboardElement.LABEL:
               return LabelConfig.createFromXml(element.getData());
            case DashboardElement.LINE_CHART:
               return XMLTools.createFromXml(LineChartConfig.class, element.getData());
            case DashboardElement.NETWORK_MAP:
               return XMLTools.createFromXml(NetworkMapConfig.class, element.getData());
            case DashboardElement.OBJECT_QUERY:
               return XMLTools.createFromXml(ObjectDetailsConfig.class, element.getData());
            case DashboardElement.OBJECT_TOOLS:
               return XMLTools.createFromXml(ObjectToolsConfig.class, element.getData());
            case DashboardElement.PIE_CHART:
               return XMLTools.createFromXml(PieChartConfig.class, element.getData());
            case DashboardElement.PORT_VIEW:
               return XMLTools.createFromXml(PortViewConfig.class, element.getData());
            case DashboardElement.RACK_DIAGRAM:
               return XMLTools.createFromXml(RackDiagramConfig.class, element.getData());
            case DashboardElement.SCRIPTED_BAR_CHART:
               return XMLTools.createFromXml(ScriptedBarChartConfig.class, element.getData());
            case DashboardElement.SCRIPTED_PIE_CHART:
               return XMLTools.createFromXml(ScriptedPieChartConfig.class, element.getData());
            case DashboardElement.SEPARATOR:
               return XMLTools.createFromXml(SeparatorConfig.class, element.getData());
            case DashboardElement.SERVICE_COMPONENTS:
               return XMLTools.createFromXml(ServiceComponentsConfig.class, element.getData());
            case DashboardElement.SNMP_TRAP_MONITOR:
               return XMLTools.createFromXml(SnmpTrapMonitorConfig.class, element.getData());
            case DashboardElement.STATUS_CHART:
               return XMLTools.createFromXml(ObjectStatusChartConfig.class, element.getData());
            case DashboardElement.STATUS_INDICATOR:
               return XMLTools.createFromXml(StatusIndicatorConfig.class, element.getData());
            case DashboardElement.STATUS_MAP:
               return XMLTools.createFromXml(StatusMapConfig.class, element.getData());
            case DashboardElement.SYSLOG_MONITOR:
               return XMLTools.createFromXml(SyslogMonitorConfig.class, element.getData());
            case DashboardElement.TABLE_BAR_CHART:
            case DashboardElement.TABLE_TUBE_CHART:
               return XMLTools.createFromXml(TableBarChartConfig.class, element.getData());
            case DashboardElement.TABLE_PIE_CHART:
               return XMLTools.createFromXml(TablePieChartConfig.class, element.getData());
            case DashboardElement.TABLE_VALUE:
               return XMLTools.createFromXml(TableValueConfig.class, element.getData());
            case DashboardElement.WEB_PAGE:
               return XMLTools.createFromXml(WebPageConfig.class, element.getData());
            default:
               return null;
         }
      }
      catch(Exception e)
      {
         logger.error("Cannot create dashboard element configuration from element object " + element, e);
         return null;
      }
   }
}
