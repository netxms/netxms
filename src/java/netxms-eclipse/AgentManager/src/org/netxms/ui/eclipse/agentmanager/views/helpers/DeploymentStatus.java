/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.agentmanager.views.helpers;

import org.netxms.client.NXCSession;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Internal object used to hold package deployment status for specific node
 */
public class DeploymentStatus
{
	private long nodeId;
	private Node nodeObject;
	private int status;
	private String message;
	
	/**
	 * @param nodeId
	 * @param status
	 * @param message
	 */
	public DeploymentStatus(long nodeId, int status, String message)
	{
		this.nodeId = nodeId;
		this.nodeObject = (Node)((NXCSession)ConsoleSharedData.getSession()).findObjectById(nodeId, Node.class);
		this.status = status;
		this.message = message;
	}
	
	/**
	 * Get node name
	 * 
	 * @return
	 */
	public String getNodeName()
	{
		if (nodeObject == null)
		{
			nodeObject = (Node)((NXCSession)ConsoleSharedData.getSession()).findObjectById(nodeId, Node.class);
		}
		return (nodeObject != null) ? nodeObject.getObjectName() : ("[" + Long.toString(nodeId) + "]");
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
	 * @return the message
	 */
	public String getMessage()
	{
		return (message != null) ? message : "";
	}

	/**
	 * @param message the message to set
	 */
	public void setMessage(String message)
	{
		this.message = message;
	}

	/**
	 * @return the nodeObject
	 */
	public Node getNodeObject()
	{
		return nodeObject;
	}
}
