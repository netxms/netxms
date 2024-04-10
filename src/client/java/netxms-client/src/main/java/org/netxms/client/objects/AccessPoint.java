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
package org.netxms.client.objects;

import java.util.Set;
import org.netxms.base.InetAddressEx;
import org.netxms.base.MacAddress;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.PollState;
import org.netxms.client.constants.AccessPointState;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.constants.LinkLayerDiscoveryProtocol;
import org.netxms.client.objects.interfaces.PollingTarget;
import org.netxms.client.topology.RadioInterface;

/**
 * Access point class
 */
public class AccessPoint extends DataCollectionTarget implements PollingTarget
{
   private long wirelessDomainId;
   private long controllerId;
	private int index;
	private MacAddress macAddress;
	private InetAddressEx ipAddress;
   private AccessPointState state;
	private String vendor;
	private String model;
	private String serialNumber;
	private RadioInterface[] radios;
   private long peerNodeId;
   private long peerInterfaceId;
   private LinkLayerDiscoveryProtocol peerDiscoveryProtocol;

	/**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
	 */
	public AccessPoint(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
      wirelessDomainId = msg.getFieldAsInt64(NXCPCodes.VID_DOMAIN_ID);
      controllerId = msg.getFieldAsInt64(NXCPCodes.VID_CONTROLLER_ID);
      index = msg.getFieldAsInt32(NXCPCodes.VID_AP_INDEX);
		macAddress = new MacAddress(msg.getFieldAsBinary(NXCPCodes.VID_MAC_ADDR));
		ipAddress = msg.getFieldAsInetAddressEx(NXCPCodes.VID_IP_ADDRESS);
		state = AccessPointState.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_STATE));
		vendor = msg.getFieldAsString(NXCPCodes.VID_VENDOR);
		model = msg.getFieldAsString(NXCPCodes.VID_MODEL);
		serialNumber = msg.getFieldAsString(NXCPCodes.VID_SERIAL_NUMBER);
      peerNodeId = msg.getFieldAsInt64(NXCPCodes.VID_PEER_NODE_ID);
      peerInterfaceId = msg.getFieldAsInt64(NXCPCodes.VID_PEER_INTERFACE_ID);
      peerDiscoveryProtocol = LinkLayerDiscoveryProtocol.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_PEER_PROTOCOL));

		int count = msg.getFieldAsInt32(NXCPCodes.VID_RADIO_COUNT);
		radios = new RadioInterface[count];
		long fieldId = NXCPCodes.VID_RADIO_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			radios[i] = new RadioInterface(this, msg, fieldId);
			fieldId += 10;
		}
	}

	/**
	 * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "AccessPoint";
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
    * Get wireless domain ID for this access point.
    *
    * @return wireless domain ID for this access point
    */
   public long getWirelessDomainId()
	{
      return wirelessDomainId;
	}

   /**
    * Get wireless controller ID for this access point.
    *
    * @return wireless controller ID for this access point
    */
   public long getControllerId()
   {
      return controllerId;
   }

	/**
    * @return the index
    */
   public int getIndex()
   {
      return index;
   }

   /**
	 * @return the macAddress
	 */
	public MacAddress getMacAddress()
	{
		return macAddress;
	}

	/**
    * @return the ipAddress
    */
   public InetAddressEx getIpAddress()
   {
      return ipAddress;
   }

   /**
	 * @return the vendor
	 */
	public String getVendor()
	{
		return vendor;
	}

	/**
	 * @return the serialNumber
	 */
	public String getSerialNumber()
	{
		return serialNumber;
	}

	/**
	 * @return the model
	 */
	public String getModel()
	{
		return model;
	}

	/**
	 * @return the radios
	 */
	public RadioInterface[] getRadios()
	{
		return radios;
	}

   /**
    * @return the state
    */
   public AccessPointState getState()
   {
      return state;
   }

   /**
    * @return the peerNodeId
    */
   public long getPeerNodeId()
   {
      return peerNodeId;
   }

   /**
    * @return the peerInterfaceId
    */
   public long getPeerInterfaceId()
   {
      return peerInterfaceId;
   }

   /**
    * @return the peerDiscoveryProtocol
    */
   public LinkLayerDiscoveryProtocol getPeerDiscoveryProtocol()
   {
      return peerDiscoveryProtocol;
   }

   /**
    * Get parent node object.
    * 
    * @return parent node object or null if it is not exist or inaccessible
    */
   public AbstractNode getParentNode()
   {
      AbstractNode node = null;
      synchronized(parents)
      {
         for(Long id : parents)
         {
            AbstractObject object = session.findObjectById(id);
            if (object instanceof AbstractNode)
            {
               node = (AbstractNode)object;
               break;
            }
         }
      }
      return node;
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
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, model);
      addString(strings, serialNumber);
      addString(strings, vendor);
      addString(strings, macAddress.toString());
      return strings;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getFlags()
    */
   @Override
   public int getFlags()
   {
      return flags;
   }/**
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
      return false;
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
