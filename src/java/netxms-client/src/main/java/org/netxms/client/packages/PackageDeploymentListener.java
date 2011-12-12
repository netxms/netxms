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
package org.netxms.client.packages;

/**
 * Listener interface for package deployment progress
 */
public interface PackageDeploymentListener
{
	public static final int PENDING = 0;
	public static final int TRANSFER = 1;
	public static final int INSTALLATION = 2;
	public static final int COMPLETED = 3;
	public static final int FAILED = 4;
	public static final int INITIALIZE = 5;
	public static final int FINISHED = 255;
	
	/**
	 * Called by deployment thread when deployment task started.
	 */
	public abstract void deploymentStarted();
	
	/**
	 * Status update for node.
	 * 
	 * @param nodeId node object ID
	 * @param status status code (defined in PackageDeploymentListener interface)
	 * @param message error message, if applicable (otherwise null)
	 */
	public abstract void statusUpdate(long nodeId, int status, String message);
	
	/**
	 * Called by deployment thread when entire deployment task is completed.
	 */
	public abstract void deploymentComplete();
}
