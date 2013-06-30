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

import java.net.InetAddress;
import org.netxms.base.*;
import org.netxms.client.MacAddress;
import org.netxms.client.NXCSession;

/**
 * Network interface object
 */
public class Interface extends GenericObject
{
	// Interface flags
	public static final int IF_SYNTHETIC_MASK         = 0x00000001;
	public static final int IF_PHYSICAL_PORT          = 0x00000002;
	public static final int IF_EXCLUDE_FROM_TOPOLOGY  = 0x00000004;
	public static final int IF_LOOPBACK               = 0x00000008;
	public static final int IF_CREATED_MANUALLY       = 0x00000010;
	public static final int IF_EXPECTED_STATE_MASK    = 0x30000000;
	
	public static final int ADMIN_STATE_UNKNOWN      = 0;
	public static final int ADMIN_STATE_UP           = 1;
	public static final int ADMIN_STATE_DOWN         = 2;
	public static final int ADMIN_STATE_TESTING      = 3;
	
	public static final int OPER_STATE_UNKNOWN       = 0;
	public static final int OPER_STATE_UP            = 1;
	public static final int OPER_STATE_DOWN          = 2;
	public static final int OPER_STATE_TESTING       = 3;
	
	public static final int PAE_STATE_UNKNOWN        = 0;
	public static final int PAE_STATE_INITIALIZE     = 1;
	public static final int PAE_STATE_DISCONNECTED   = 2;
	public static final int PAE_STATE_CONNECTING     = 3;
	public static final int PAE_STATE_AUTHENTICATING = 4;
	public static final int PAE_STATE_AUTHENTICATED  = 5;
	public static final int PAE_STATE_ABORTING       = 6;
	public static final int PAE_STATE_HELD           = 7;
	public static final int PAE_STATE_FORCE_AUTH     = 8;
	public static final int PAE_STATE_FORCE_UNAUTH   = 9;
	public static final int PAE_STATE_RESTART        = 10;

	public static final int BACKEND_STATE_UNKNOWN    = 0;
	public static final int BACKEND_STATE_REQUEST    = 1;
	public static final int BACKEND_STATE_RESPONSE   = 2;
	public static final int BACKEND_STATE_SUCCESS    = 3;
	public static final int BACKEND_STATE_FAIL       = 4;
	public static final int BACKEND_STATE_TIMEOUT    = 5;
	public static final int BACKEND_STATE_IDLE       = 6;
	public static final int BACKEND_STATE_INITIALIZE = 7;
	public static final int BACKEND_STATE_IGNORE     = 8;
	
	private static final String[] stateText =
		{
			"UNKNOWN",
			"UP",
			"DOWN",
			"TESTING"
		};
	private static final String[] paeStateText =
		{
			"UNKNOWN",
			"INITIALIZE",
			"DISCONNECTED",
			"CONNECTING",
			"AUTHENTICATING",
			"AUTHENTICATED",
			"ABORTING",
			"HELD",
			"FORCE AUTH",
			"FORCE UNAUTH",
			"RESTART"
		};
	private static final String[] backendStateText =
		{
			"UNKNOWN",
			"REQUEST",
			"RESPONSE",
			"SUCCESS",
			"FAIL",
			"TIMEOUT",
			"IDLE",
			"INITIALIZE",
			"IGNORE"
		};
	
	private int flags;
	private InetAddress subnetMask;
	private int ifIndex;
	private int ifType;
	private int slot;
	private int port;
	private MacAddress macAddress;
	private int requiredPollCount;
	private long peerNodeId;
	private long peerInterfaceId;
	private long zoneId;
	private String description;
	private int adminState;
	private int operState;
	private int dot1xPaeState;
	private int dot1xBackendState;
	
	/**
	 * @param msg
	 */
	public Interface(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		flags = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		subnetMask = msg.getVariableAsInetAddress(NXCPCodes.VID_IP_NETMASK);
		ifIndex = msg.getVariableAsInteger(NXCPCodes.VID_IF_INDEX);
		ifType = msg.getVariableAsInteger(NXCPCodes.VID_IF_TYPE);
		slot = msg.getVariableAsInteger(NXCPCodes.VID_IF_SLOT);
		port = msg.getVariableAsInteger(NXCPCodes.VID_IF_PORT);
		macAddress = new MacAddress(msg.getVariableAsBinary(NXCPCodes.VID_MAC_ADDR));
		requiredPollCount = msg.getVariableAsInteger(NXCPCodes.VID_REQUIRED_POLLS);
		peerNodeId = msg.getVariableAsInt64(NXCPCodes.VID_PEER_NODE_ID);
		peerInterfaceId = msg.getVariableAsInt64(NXCPCodes.VID_PEER_INTERFACE_ID);
		zoneId = msg.getVariableAsInt64(NXCPCodes.VID_ZONE_ID);
		description = msg.getVariableAsString(NXCPCodes.VID_DESCRIPTION);
		adminState = msg.getVariableAsInteger(NXCPCodes.VID_ADMIN_STATE);
		operState = msg.getVariableAsInteger(NXCPCodes.VID_OPER_STATE);
		dot1xPaeState = msg.getVariableAsInteger(NXCPCodes.VID_DOT1X_PAE_STATE);
		dot1xBackendState = msg.getVariableAsInteger(NXCPCodes.VID_DOT1X_BACKEND_STATE);
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
	 * @return Interface subnet mask
	 */
	public InetAddress getSubnetMask()
	{
		return subnetMask;
	}

	/**
	 * @return Interface index
	 */
	public int getIfIndex()
	{
		return ifIndex;
	}

	/**
	 * @return Interface type
	 */
	public int getIfType()
	{
		return ifType;
	}

	/**
	 * @return Interface MAC address
	 */
	public MacAddress getMacAddress()
	{
		return macAddress;
	}

	/**
	 * @return Number of polls required to change interface status
	 */
	public int getRequiredPollCount()
	{
		return requiredPollCount;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Interface";
	}

	/**
	 * @return the slot
	 */
	public int getSlot()
	{
		return slot;
	}

	/**
	 * @return the port
	 */
	public int getPort()
	{
		return port;
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
	 * @return the zoneId
	 */
	public long getZoneId()
	{
		return zoneId;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

	/**
	 * @return the dot1xPaeState
	 */
	public int getDot1xPaeState()
	{
		return dot1xPaeState;
	}
	
	/**
	 * Get 802.1x PAE state as text
	 * 
	 * @return
	 */
	public String getDot1xPaeStateAsText()
	{
		try
		{
			return paeStateText[dot1xPaeState];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return paeStateText[PAE_STATE_UNKNOWN];
		}
	}

	/**
	 * @return the dot1xBackendState
	 */
	public int getDot1xBackendState()
	{
		return dot1xBackendState;
	}

	/**
	 * Get 802.1x backend state as text
	 * 
	 * @return
	 */
	public String getDot1xBackendStateAsText()
	{
		try
		{
			return backendStateText[dot1xBackendState];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return backendStateText[BACKEND_STATE_UNKNOWN];
		}
	}

	/**
	 * @return the adminState
	 */
	public int getAdminState()
	{
		return adminState;
	}

	/**
	 * @return the adminState
	 */
	public String getAdminStateAsText()
	{
		try
		{
			return stateText[adminState];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return stateText[ADMIN_STATE_UNKNOWN];
		}
	}

	/**
	 * @return the operState
	 */
	public int getOperState()
	{
		return operState;
	}
	
	/**
	 * @return the operState
	 */
	public String getOperStateAsText()
	{
		try
		{
			return stateText[operState];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return stateText[OPER_STATE_UNKNOWN];
		}
	}

	/**
	 * Get interface expected state
	 * 
	 * @return
	 */
	public int getExpectedState()
	{
		return (flags & IF_EXPECTED_STATE_MASK) >> 28;
	}
	
	/**
	 * @return
	 */
	public boolean isPhysicalPort()
	{
		return (flags & IF_PHYSICAL_PORT) != 0;
	}
	
	/**
	 * @return
	 */
	public boolean isLoopback()
	{
		return (flags & IF_LOOPBACK) != 0;
	}
	
	/**
	 * @return
	 */
	public boolean isExcludedFromTopology()
	{
		return (flags & IF_EXCLUDE_FROM_TOPOLOGY) != 0;
	}
}
