/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import org.netxms.client.constants.DataCollectionError;
import org.netxms.client.constants.DataCollectionObjectStatus;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.nxmc.localization.LocalizationHelper;

/**
 * Data collection display information
 */
public class DataCollectionDisplayInfo
{
	private static String dciDataTypes[] = new String[9];
	
	/**
	 * Initialize static members. Intended to be called once by library activator.
	 */
	public static void init()
	{
      dciDataTypes[DataType.COUNTER32.getValue()] = LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Counter 32-bit");
      dciDataTypes[DataType.COUNTER64.getValue()] = LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Counter 64-bit");
		dciDataTypes[DataType.INT32.getValue()] = LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Integer");
		dciDataTypes[DataType.UINT32.getValue()] = LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Unsigned Integer");
		dciDataTypes[DataType.INT64.getValue()] = LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Integer 64-bit");
		dciDataTypes[DataType.UINT64.getValue()] = LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Unsigned Integer 64-bit");
		dciDataTypes[DataType.FLOAT.getValue()] = LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Float");
		dciDataTypes[DataType.STRING.getValue()] = LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("String");
		dciDataTypes[DataType.NULL.getValue()] = LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Null");
	}

   /**
    * Get text to be displayed instead of DCI value when data collection failed.
    *
    * @param error reason of the last data collection error
    * @return text to be displayed instead of DCI value
    */
   public static String getErrorText(DataCollectionError error)
   {
      switch(error)
      {
         case ACCESS_DENIED:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("<< ACCESS DENIED >>");
         case COMM_ERROR:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("<< COMM ERROR >>");
         case INVALID_DATA:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("<< INVALID DATA >>");
         case NO_SUCH_INSTANCE:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("<< NO SUCH INSTANCE >>");
         case NOT_SUPPORTED:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("<< NOT SUPPORTED >>");
         default:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("<< ERROR >>");
      }
   }

   /**
    * Get display name for given data origin.
    *
    * @param origin data origin
    * @return display name for data origin
    */
   public static String getOriginName(DataOrigin origin)
   {
      switch(origin)
      {
         case AGENT:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("NetXMS Agent");
         case CLOUD_CONNECTOR:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Cloud Connector");
         case COMPUTED:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Computed");
         case DEVICE_DRIVER:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Network Device Driver");
         case ETHERNET_IP:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("EtherNet/IP");
         case INTERNAL:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Internal");
         case MODBUS:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Modbus");
         case MQTT:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("MQTT");
         case NETCONF:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("NETCONF");
         case OTLP:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("OTLP");
         case PUSH:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Push");
         case SCRIPT:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Script");
         case SMCLP:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("SM-CLP");
         case SNMP:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("SNMP");
         case SSH:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("SSH");
         case TRAFFIC_OBSERVER:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Traffic Observer");
         case WEB_SERVICE:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Web Service");
         case WINPERF:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Windows Performance Counters");
         default:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("<unknown>");
      }
   }

   /**
    * Get display name for given data collection object status.
    *
    * @param status data collection object status
    * @return display name for status
    */
   public static String getStatusName(DataCollectionObjectStatus status)
   {
      switch(status)
      {
         case ACTIVE:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Active");
         case DISABLED:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Disabled");
         case UNSUPPORTED:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("Not supported");
         default:
            return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("<unknown>");
      }
   }

	/**
	 * Get name for given data type
	 * @param dt Data type ID
	 * @return data type name
	 */
	public static String getDataTypeName(DataType dt)
	{
		try
		{
			return dciDataTypes[dt.getValue()];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return LocalizationHelper.getI18n(DataCollectionDisplayInfo.class).tr("<unknown>");
		}
	}
}
