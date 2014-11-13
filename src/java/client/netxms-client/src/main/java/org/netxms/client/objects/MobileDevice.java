/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.client.objects;

import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Mobile device object
 */
public class MobileDevice extends GenericObject
{
	private String deviceId;
	private String vendor;
	private String model;
	private String serialNumber;
	private String osName;
	private String osVersion;
	private String userId;
	private int batteryLevel;
	private Date lastReportTime;
	
	/**
	 * @param msg
	 * @param session
	 */
	public MobileDevice(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		deviceId = msg.getVariableAsString(NXCPCodes.VID_DEVICE_ID);
		vendor = msg.getVariableAsString(NXCPCodes.VID_VENDOR);
		model = msg.getVariableAsString(NXCPCodes.VID_MODEL);
		serialNumber = msg.getVariableAsString(NXCPCodes.VID_SERIAL_NUMBER);
		osName = msg.getVariableAsString(NXCPCodes.VID_OS_NAME);
		osVersion = msg.getVariableAsString(NXCPCodes.VID_OS_VERSION);
		userId = msg.getVariableAsString(NXCPCodes.VID_USER_ID);
		batteryLevel = msg.getVariableAsInteger(NXCPCodes.VID_BATTERY_LEVEL);
		lastReportTime = msg.getVariableAsDate(NXCPCodes.VID_LAST_CHANGE_TIME);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
	 */
	@Override
	public boolean isAllowedOnMap()
	{
		return true;
	}

   /* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#isAlarmsVisible()
    */
   @Override
   public boolean isAlarmsVisible()
   {
      return true;
   }

	/**
	 * @return the deviceId
	 */
	public final String getDeviceId()
	{
		return deviceId;
	}

	/**
	 * @return the vendor
	 */
	public final String getVendor()
	{
		return vendor;
	}

	/**
	 * @return the model
	 */
	public final String getModel()
	{
		return model;
	}

	/**
	 * @return the serialNumber
	 */
	public final String getSerialNumber()
	{
		return serialNumber;
	}

	/**
	 * @return the osName
	 */
	public final String getOsName()
	{
		return osName;
	}

	/**
	 * @return the osVersion
	 */
	public final String getOsVersion()
	{
		return osVersion;
	}

	/**
	 * @return the userId
	 */
	public final String getUserId()
	{
		return userId;
	}

	/**
	 * @return the batteryLevel
	 */
	public final int getBatteryLevel()
	{
		return batteryLevel;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "MobileDevice";
	}

	/**
	 * @return the lastReportTime
	 */
	public final Date getLastReportTime()
	{
		return lastReportTime;
	}
}
