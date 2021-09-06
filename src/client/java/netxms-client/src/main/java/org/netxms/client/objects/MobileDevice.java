/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
import java.util.Set;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Mobile device object
 */
public class MobileDevice extends DataCollectionTarget
{
	private String deviceId;
   private String commProtocol;
	private String vendor;
	private String model;
	private String serialNumber;
	private String osName;
	private String osVersion;
	private String userId;
	private short batteryLevel;
	private float speed;
	private short direction;
	private int altitude;
	private Date lastReportTime;
	
	/**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
	 */
	public MobileDevice(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		deviceId = msg.getFieldAsString(NXCPCodes.VID_DEVICE_ID);
      commProtocol = msg.getFieldAsString(NXCPCodes.VID_COMM_PROTOCOL);
		vendor = msg.getFieldAsString(NXCPCodes.VID_VENDOR);
		model = msg.getFieldAsString(NXCPCodes.VID_MODEL);
		serialNumber = msg.getFieldAsString(NXCPCodes.VID_SERIAL_NUMBER);
		osName = msg.getFieldAsString(NXCPCodes.VID_OS_NAME);
		osVersion = msg.getFieldAsString(NXCPCodes.VID_OS_VERSION);
		userId = msg.getFieldAsString(NXCPCodes.VID_USER_ID);
		batteryLevel = msg.getFieldAsInt16(NXCPCodes.VID_BATTERY_LEVEL);
		speed = msg.getFieldAsDouble(NXCPCodes.VID_SPEED).floatValue();
      direction = msg.getFieldAsInt16(NXCPCodes.VID_DIRECTION);
      altitude = msg.getFieldAsInt32(NXCPCodes.VID_ALTITUDE);
		lastReportTime = msg.getFieldAsDate(NXCPCodes.VID_LAST_CHANGE_TIME);
	}

   /**
    * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
    */
	@Override
	public boolean isAllowedOnMap()
	{
		return true;
	}

   /**
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
    * @return the commProtocol
    */
   public String getCommProtocol()
   {
      return commProtocol;
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
	public final short getBatteryLevel()
	{
		return batteryLevel;
	}

	/**
	 * Get last reported speed of the device. Will return -1 if speed is not known.
	 *
	 * @return last reported speed of the device or -1
	 */
	public float getSpeed()
   {
      return speed;
   }

   /**
    * Get last reported direction of the device (in range 0..360). Will return -1 if direction is not known.
    *
    * @return last reported direction of the device or -1
    */
   public short getDirection()
   {
      return direction;
   }

   /**
    * Get last reported altitude of the device
    *
    * @return last reported altitude of the device
    */
   public int getAltitude()
   {
      return altitude;
   }

   /**
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

   /**
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, deviceId);
      addString(strings, model);
      addString(strings, osName);
      addString(strings, osVersion);
      addString(strings, serialNumber);
      addString(strings, userId);
      addString(strings, vendor);
      return strings;
   }
}
