/**
 * 
 */
package org.netxms.ui.eclipse.topology.widgets.helpers;

import org.netxms.client.objects.Interface;

/**
 * Port  information for PortView widget
 */
public class PortInfo
{
	private long interfaceObjectId;
	private int slot;
	private int port;
	private int status;
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
		slot = iface.getSlot();
		port = iface.getPort();
		status = iface.getStatus();
		adminState = iface.getAdminState();
		operState = iface.getOperState();
	}

	/**
	 * @return the status
	 */
	public int getStatus()
	{
		return status;
	}

	/**
	 * @param status the status to set
	 */
	public void setStatus(int status)
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
