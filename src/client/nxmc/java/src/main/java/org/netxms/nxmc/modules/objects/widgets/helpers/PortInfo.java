/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.nxmc.modules.objects.widgets.helpers;

import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.Interface;

/**
 * Port  information for PortView widget
 */
public class PortInfo
{
	private long interfaceObjectId;
   private int chassis;
	private int module;
	private int pic;
   private int port;
	private ObjectStatus status;
	private int adminState;
	private int operState;
	private boolean highlighted;
	
	/**
	 * Create port information record
	 * 
	 * @param iface Interface object
	 */
	public PortInfo(Interface iface)
	{
		interfaceObjectId = iface.getObjectId();
		chassis = iface.getChassis();
		module = iface.getModule();
		pic = iface.getPIC();
		port = iface.getPort();
		status = iface.getStatus();
		adminState = iface.getAdminState();
		operState = iface.getOperState();
	}

	/**
	 * @return the status
	 */
	public ObjectStatus getStatus()
	{
		return status;
	}

	/**
	 * @param status the status to set
	 */
	public void setStatus(ObjectStatus status)
	{
		this.status = status;
	}

	/**
	 * @return the interfaceObjectId
	 */
	public long getInterfaceObjectId()
	{
		return interfaceObjectId;
	}

	/**
    * @return the chassis
    */
   public int getChassis()
   {
      return chassis;
   }

   /**
	 * @return the slot
	 */
	public int getModule()
	{
		return module;
	}

	/**
    * @return the pic
    */
   public int getPIC()
   {
      return pic;
   }

   /**
	 * @return the port
	 */
	public int getPort()
	{
		return port;
	}

	/**
	 * @return the highlighted
	 */
	public boolean isHighlighted()
	{
		return highlighted;
	}
	/**
	 * @param highlighted the highlighted to set
	 */
	public void setHighlighted(boolean highlighted)
	{
		this.highlighted = highlighted;
	}

	/**
	 * @return the adminState
	 */
	public int getAdminState()
	{
		return adminState;
	}

	/**
	 * @param adminState the adminState to set
	 */
	public void setAdminState(int adminState)
	{
		this.adminState = adminState;
	}

	/**
	 * @return the operState
	 */
	public int getOperState()
	{
		return operState;
	}

	/**
	 * @param operState the operState to set
	 */
	public void setOperState(int operState)
	{
		this.operState = operState;
	}
}
