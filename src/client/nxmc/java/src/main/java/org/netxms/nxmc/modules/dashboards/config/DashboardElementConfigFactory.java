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
package org.netxms.nxmc.modules.dashboards.config;

import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.xml.XMLTools;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.Gson;

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
      if (element.getData().trim().startsWith("<"))
      {
         return createFromXML(element);
      }
      else
      {
         return createFromJson(element);
      }      
   }

   /**
    * Create configuration object from dashboard element object.
    *
    * @param element dashboard element object
    * @return dashboard element configuration object or null on error
    */
   public static DashboardElementConfig createFromXML(DashboardElement element)
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
            case DashboardElement.DASHBOARD:
               return EmbeddedDashboardConfig.createFromXmlOrJson(element.getData());
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
               return LabelConfig.createFromXmlOrJson(element.getData());
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

   /**
    * Create configuration object from dashboard element object.
    *
    * @param element dashboard element object
    * @return dashboard element configuration object or null on error
    */
   public static DashboardElementConfig createFromJson(DashboardElement element)
   {
      try
      {
         switch(element.getType())
         {
            case DashboardElement.ALARM_VIEWER:
               return new Gson().fromJson(element.getData(), AlarmViewerConfig.class);
            case DashboardElement.AVAILABLITY_CHART:
               return new Gson().fromJson(element.getData(), AvailabilityChartConfig.class);
            case DashboardElement.BAR_CHART:
            case DashboardElement.TUBE_CHART:
               return new Gson().fromJson(element.getData(), BarChartConfig.class);
            case DashboardElement.DASHBOARD:
               return EmbeddedDashboardConfig.createFromXmlOrJson(element.getData());
            case DashboardElement.DCI_SUMMARY_TABLE:
               return new Gson().fromJson(element.getData(), DciSummaryTableConfig.class);
            case DashboardElement.DIAL_CHART:
               return new Gson().fromJson(element.getData(), GaugeConfig.class);
            case DashboardElement.EVENT_MONITOR:
               return new Gson().fromJson(element.getData(), EventMonitorConfig.class);
            case DashboardElement.FILE_MONITOR:
               return new Gson().fromJson(element.getData(), FileMonitorConfig.class);
            case DashboardElement.GEO_MAP:
               return new Gson().fromJson(element.getData(), GeoMapConfig.class);
            case DashboardElement.LABEL:
               return LabelConfig.createFromXmlOrJson(element.getData());
            case DashboardElement.LINE_CHART:
               return new Gson().fromJson(element.getData(), LineChartConfig.class);
            case DashboardElement.NETWORK_MAP:
               return new Gson().fromJson(element.getData(), NetworkMapConfig.class);
            case DashboardElement.OBJECT_QUERY:
               return new Gson().fromJson(element.getData(), ObjectDetailsConfig.class);
            case DashboardElement.OBJECT_TOOLS:
               return new Gson().fromJson(element.getData(), ObjectToolsConfig.class);
            case DashboardElement.PIE_CHART:
               return new Gson().fromJson(element.getData(), PieChartConfig.class);
            case DashboardElement.PORT_VIEW:
               return new Gson().fromJson(element.getData(), PortViewConfig.class);
            case DashboardElement.RACK_DIAGRAM:
               return new Gson().fromJson(element.getData(), RackDiagramConfig.class);
            case DashboardElement.SCRIPTED_BAR_CHART:
               return new Gson().fromJson(element.getData(), ScriptedBarChartConfig.class);
            case DashboardElement.SCRIPTED_PIE_CHART:
               return new Gson().fromJson(element.getData(), ScriptedPieChartConfig.class);
            case DashboardElement.SEPARATOR:
               return new Gson().fromJson(element.getData(), SeparatorConfig.class);
            case DashboardElement.SERVICE_COMPONENTS:
               return new Gson().fromJson(element.getData(), ServiceComponentsConfig.class);
            case DashboardElement.SNMP_TRAP_MONITOR:
               return new Gson().fromJson(element.getData(), SnmpTrapMonitorConfig.class);
            case DashboardElement.STATUS_CHART:
               return new Gson().fromJson(element.getData(), ObjectStatusChartConfig.class);
            case DashboardElement.STATUS_INDICATOR:
               return new Gson().fromJson(element.getData(), StatusIndicatorConfig.class);
            case DashboardElement.STATUS_MAP:
               return new Gson().fromJson(element.getData(), StatusMapConfig.class);
            case DashboardElement.SYSLOG_MONITOR:
               return new Gson().fromJson(element.getData(), SyslogMonitorConfig.class);
            case DashboardElement.TABLE_BAR_CHART:
            case DashboardElement.TABLE_TUBE_CHART:
               return new Gson().fromJson(element.getData(), TableBarChartConfig.class);
            case DashboardElement.TABLE_PIE_CHART:
               return new Gson().fromJson(element.getData(), TablePieChartConfig.class);
            case DashboardElement.TABLE_VALUE:
               return new Gson().fromJson(element.getData(), TableValueConfig.class);
            case DashboardElement.WEB_PAGE:
               return new Gson().fromJson(element.getData(), WebPageConfig.class);
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
