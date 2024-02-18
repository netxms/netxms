/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.client.topology;

import org.netxms.base.MacAddress;
import org.netxms.base.NXCPMessage;
import org.netxms.base.annotations.Internal;
import org.netxms.client.constants.RadioBand;
import org.netxms.client.objects.AbstractObject;

/**
 * Radio interface information
 */
public class RadioInterface
{
	@Internal
   private AbstractObject owner;
	
	private int index;
	private String name;
   private MacAddress bssid;
   private String ssid;
   private int frequency;
   private RadioBand band;
	private int channel;
	private int powerDBm;
	private int powerMW;
	
	/**
    * Create radio interface object from NXCP message
    *
    * @param owner Owner (access point or node)
    * @param msg NXCP message
    * @param baseId object base id
    */
   public RadioInterface(AbstractObject owner, NXCPMessage msg, long baseId)
	{
      this.owner = owner;
		index = msg.getFieldAsInt32(baseId);
		name = msg.getFieldAsString(baseId + 1);
      bssid = new MacAddress(msg.getFieldAsBinary(baseId + 2));
      frequency = msg.getFieldAsInt32(baseId + 3);
      band = RadioBand.getByValue(msg.getFieldAsInt32(baseId + 4));
      channel = msg.getFieldAsInt32(baseId + 5);
      powerDBm = msg.getFieldAsInt32(baseId + 6);
      powerMW = msg.getFieldAsInt32(baseId + 7);
      ssid = msg.getFieldAsString(baseId + 8);
	}

	/**
	 * @return the index
	 */
	public int getIndex()
	{
		return index;
	}

	/**
    * Get name of this interface.
    *
    * @return name of this interface
    */
	public String getName()
	{
		return name;
	}

	/**
    * Get BSSID of this interface.
    *
    * @return BSSID of this interface
    */
   public MacAddress getBSSID()
	{
      return bssid;
	}

   /**
    * Get SSID of this interface.
    *
    * @return SSID of this interface
    */
   public String getSSID()
   {
      return ssid;
   }

	/**
    * Get radio frequency of this interface.
    *
    * @return radio frequency of this interface in MHz
    */
   public int getFrequency()
   {
      return frequency;
   }

   /**
    * Get radio band of this interface.
    *
    * @return radio band of this interface.
    */
   public RadioBand getBand()
   {
      return band;
   }

   /**
    * @return the channel
    */
	public int getChannel()
	{
		return channel;
	}

	/**
	 * Get transmitting power in dBm
	 * 
	 * @return the powerDBm
	 */
	public int getPowerDBm()
	{
		return powerDBm;
	}

	/**
	 * Get transmitting power in milliwatts
	 * 
	 * @return the powerMW
	 */
	public int getPowerMW()
	{
		return powerMW;
	}

	/**
    * @return owner object
    */
   public AbstractObject getOwner()
	{
      return owner;
	}
}
