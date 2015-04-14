/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.client;

/**
 * Generic progress listener interface
 */
public interface ProgressListener
{
	/**
	 * Called by entity performing operation to indicate total amount of work to be done
	 * 
	 * @param workTotal
	 */
	public abstract void setTotalWorkAmount(long workTotal);
	
	/**
	 * Called by entity performing operation to indicate progress
	 *  
	 * @param workDone raw value of work done (for example, number of bytes transferred) 
	 */
	public abstract void markProgress(long workDone);
}
