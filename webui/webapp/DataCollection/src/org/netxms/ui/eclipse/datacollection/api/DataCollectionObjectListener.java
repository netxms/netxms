/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.api;

/**
 * Listener for data collection object editor
 */
public interface DataCollectionObjectListener
{
	/**
	 * Called by editor after user choose new single value parameter using "Select" button
	 * 
	 * @param origin
	 * @param name
	 * @param description
	 * @param dataType
	 */
	public void onSelectItem(int origin, String name, String description, int dataType);

	/**
	 * Called by editor after user choose new table parameter using "Select" button
	 * 
	 * @param origin
	 * @param name
	 * @param description
	 * @param instanceColumn
	 */
	public void onSelectTable(int origin, String name, String description, String instanceColumn);
}
