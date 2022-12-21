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

import org.eclipse.core.runtime.IAdapterFactory;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.widgets.internal.AlarmViewerConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.BarChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.CustomWidgetConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DciSummaryTableConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.EmbeddedDashboardConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.EventMonitorConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.FileMonitorConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.GaugeConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.GeoMapConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.LabelConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.LineChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.NetworkMapConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectDetailsConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectStatusChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectToolsConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.PieChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.PortViewConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.RackDiagramConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ScriptedBarChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ScriptedPieChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.SeparatorConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ServiceComponentsConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.SnmpTrapMonitorConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.StatusIndicatorConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.StatusMapConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.SyslogMonitorConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.TableBarChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.TablePieChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.TableValueConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.WebPageConfig;

/**
 * Adapter factory for dashboard elements
 */
public class DashboardElementAdapterFactory implements IAdapterFactory
{
	private static final Class<?>[] supportedClasses = 
	{
		DashboardElementConfig.class
	};

   /**
    * @see org.eclipse.core.runtime.IAdapterFactory#getAdapterList()
    */
	@Override
	public Class<?>[] getAdapterList()
	{
		return supportedClasses;
	}
	
   /**
    * @see org.eclipse.core.runtime.IAdapterFactory#getAdapter(java.lang.Object, java.lang.Class)
    */
	@SuppressWarnings({ "rawtypes", "unchecked" })
	@Override
	public Object getAdapter(Object adaptableObject, Class adapterType)
	{
		if (!(adaptableObject instanceof DashboardElement))
			return null;
		
		if (adapterType == DashboardElementConfig.class)
		{
			DashboardElement element = (DashboardElement)adaptableObject;
			try
			{
				switch(element.getType())
				{
				   case DashboardElement.SYSLOG_MONITOR:
				      return SyslogMonitorConfig.createFromXml(element.getData());
				   case DashboardElement.SNMP_TRAP_MONITOR:
				      return SnmpTrapMonitorConfig.createFromXml(element.getData());
				   case DashboardElement.EVENT_MONITOR:
				      return EventMonitorConfig.createFromXml(element.getData());
               case DashboardElement.FILE_MONITOR:
                  return FileMonitorConfig.createFromXml(element.getData());
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
					case DashboardElement.STATUS_CHART:
						return ObjectStatusChartConfig.createFromXml(element.getData());
					case DashboardElement.STATUS_INDICATOR:
						return StatusIndicatorConfig.createFromXml(element.getData());
					case DashboardElement.STATUS_MAP:
						return StatusMapConfig.createFromXml(element.getData());
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
				e.printStackTrace();
				return null;
			}
		}
		return null;
	}
}
