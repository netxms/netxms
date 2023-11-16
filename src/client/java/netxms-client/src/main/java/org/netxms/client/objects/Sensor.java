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
package org.netxms.client.objects;

import java.util.Set;
import org.netxms.base.MacAddress;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.PollState;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.constants.SensorDeviceClass;
import org.netxms.client.objects.interfaces.PollingTarget;

/**
 * Mobile device object
 */
public class Sensor extends DataCollectionTarget implements PollingTarget
{
   private SensorDeviceClass deviceClass;
   private long gatewayId;
   private short modbusUnitId;
   private MacAddress macAddress;
   private String deviceAddress;
	private String vendor;
   private String model;
	private String serialNumber;

	/**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
	 */
	public Sensor(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
      gatewayId = msg.getFieldAsInt32(NXCPCodes.VID_GATEWAY_NODE);
      modbusUnitId = msg.getFieldAsInt16(NXCPCodes.VID_MODBUS_UNIT_ID);
		macAddress = new MacAddress(msg.getFieldAsBinary(NXCPCodes.VID_MAC_ADDR));
      deviceAddress = msg.getFieldAsString(NXCPCodes.VID_DEVICE_ADDRESS);
      deviceClass = SensorDeviceClass.getByValue(msg.getFieldAsInt16(NXCPCodes.VID_DEVICE_CLASS));
	   vendor = msg.getFieldAsString(NXCPCodes.VID_VENDOR);
      model = msg.getFieldAsString(NXCPCodes.VID_MODEL);
	   serialNumber = msg.getFieldAsString(NXCPCodes.VID_SERIAL_NUMBER);
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
    * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
    */
   @Override
   public String getObjectClassName()
   {
      return "Sensor";
   }

   /**
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {      
      Set<String> strings = super.getStrings();
      addString(strings, macAddress.toString());
      addString(strings, vendor);
      addString(strings, model);
      addString(strings, serialNumber);
      addString(strings, deviceAddress);
      return strings;
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
   public String getModel()
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
    * @return the flags
    */
   public int getFlags()
   {
      return flags;
   }

   /**
    * @return the macAddress
    */
   public MacAddress getMacAddress()
   {
      return macAddress;
   }

   /**
    * @return the deviceClass
    */
   public SensorDeviceClass getDeviceClass()
   {
      return deviceClass;
   }

   /**
    * @return the deviceAddress
    */
   public String getDeviceAddress()
   {
      return deviceAddress;
   }

   /**
    * @return the proxyId
    */
   public long getGatewayId()
   {
      return gatewayId;
   }

   /**
    * @return the modbusUnitId
    */
   public short getModbusUnitId()
   {
      return modbusUnitId;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getIfXTablePolicy()
    */
   @Override
   public int getIfXTablePolicy()
   {
      return 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getAgentCacheMode()
    */
   @Override
   public AgentCacheMode getAgentCacheMode()
   {
      return null;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getPollerNodeId()
    */
   @Override
   public long getPollerNodeId()
   {
      return 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHaveAgent()
    */
   @Override
   public boolean canHaveAgent()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHaveInterfaces()
    */
   @Override
   public boolean canHaveInterfaces()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHavePollerNode()
    */
   @Override
   public boolean canHavePollerNode()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canUseEtherNetIP()
    */
   @Override
   public boolean canUseEtherNetIP()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canUseModbus()
    */
   @Override
   public boolean canUseModbus()
   {
      return true;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getPollStates()
    */
   @Override
   public PollState[] getPollStates()
   {
      return pollStates;
   }
}
