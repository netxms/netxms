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
package org.netxms.ui.eclipse.dashboard.api;

import org.eclipse.swt.widgets.Composite;

/**
 * Creation data for custom dashboard element
 */
public class DashboardElementCreationData
{
	public Composite parent;
	public String data;

	/**
	 * @param parent
	 * @param data
	 */
	public DashboardElementCreationData(Composite parent, String data)
	{
		super();
		this.parent = parent;
		this.data = data;
	}
}
