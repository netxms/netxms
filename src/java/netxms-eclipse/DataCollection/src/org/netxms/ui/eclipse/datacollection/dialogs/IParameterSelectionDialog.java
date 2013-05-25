/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.dialogs;

/**
 * Generic interface for parameter selection dialogs
 *
 */
public interface IParameterSelectionDialog
{
	/**
	 * Get name of selected parameter
	 * 
	 * @return Name of selected parameter
	 */
	public String getParameterName();
	
	/**
	 * Get description of selected parameter
	 * 
	 * @return Description of selected parameter
	 */
	public String getParameterDescription();
	
	/**
	 * Get data type for selected parameter
	 * 
	 * @return data type for selected parameter
	 */
	public int getParameterDataType();

	/**
	 * Get instance column name. Relevant only for table selection.
	 * 
	 * @return
	 */
	public String getInstanceColumn();
}
