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
package org.netxms.nxmc.modules.dashboards.config;

import org.netxms.client.dashboards.DashboardElement;
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
               return AlarmViewerConfig.createFromXml(element.getData());
            case DashboardElement.BAR_CHART:
            case DashboardElement.TUBE_CHART:
               return BarChartConfig.createFromXml(element.getData());
            case DashboardElement.CUSTOM:
               return CustomWidgetConfig.createFromXml(element.getData());
            case DashboardElement.DASHBOARD:
               return EmbeddedDashboardConfig.createFromXml(element.getData());
            case DashboardElement.DCI_SUMMARY_TABLE:
               return DciSummaryTableConfig.createFromXml(element.getData());
            case DashboardElement.DIAL_CHART:
               return GaugeConfig.createFromXml(element.getData());
            case DashboardElement.EVENT_MONITOR:
               return EventMonitorConfig.createFromXml(element.getData());
            case DashboardElement.GEO_MAP:
               return GeoMapConfig.createFromXml(element.getData());
            case DashboardElement.LABEL:
               return LabelConfig.createFromXml(element.getData());
            case DashboardElement.LINE_CHART:
               return LineChartConfig.createFromXml(element.getData());
            case DashboardElement.NETWORK_MAP:
               return NetworkMapConfig.createFromXml(element.getData());
            case DashboardElement.OBJECT_QUERY:
               return ObjectDetailsConfig.createFromXml(element.getData());
            case DashboardElement.OBJECT_TOOLS:
               return ObjectToolsConfig.createFromXml(element.getData());
            case DashboardElement.PIE_CHART:
               return PieChartConfig.createFromXml(element.getData());
            case DashboardElement.PORT_VIEW:
               return PortViewConfig.createFromXml(element.getData());
            case DashboardElement.RACK_DIAGRAM:
               return RackDiagramConfig.createFromXml(element.getData());
            case DashboardElement.SCRIPTED_BAR_CHART:
               return ScriptedBarChartConfig.createFromXml(element.getData());
            case DashboardElement.SCRIPTED_PIE_CHART:
               return ScriptedPieChartConfig.createFromXml(element.getData());
            case DashboardElement.SEPARATOR:
               return SeparatorConfig.createFromXml(element.getData());
            case DashboardElement.SERVICE_COMPONENTS:
               return ServiceComponentsConfig.createFromXml(element.getData());
            case DashboardElement.SNMP_TRAP_MONITOR:
               return SnmpTrapMonitorConfig.createFromXml(element.getData());
            case DashboardElement.STATUS_CHART:
               return ObjectStatusChartConfig.createFromXml(element.getData());
            case DashboardElement.STATUS_INDICATOR:
               return StatusIndicatorConfig.createFromXml(element.getData());
            case DashboardElement.STATUS_MAP:
               return StatusMapConfig.createFromXml(element.getData());
            case DashboardElement.SYSLOG_MONITOR:
               return SyslogMonitorConfig.createFromXml(element.getData());
            case DashboardElement.TABLE_BAR_CHART:
            case DashboardElement.TABLE_TUBE_CHART:
               return TableBarChartConfig.createFromXml(element.getData());
            case DashboardElement.TABLE_PIE_CHART:
               return TablePieChartConfig.createFromXml(element.getData());
            case DashboardElement.TABLE_VALUE:
               return TableValueConfig.createFromXml(element.getData());
            case DashboardElement.WEB_PAGE:
               return WebPageConfig.createFromXml(element.getData());
            default:
               return null;
         }
      }
      catch(Exception e)
      {
         logger.error("Cannot create dashboard element configuration from element object " + element, e);
         e.printStackTrace();
         return null;
      }
   }
}
