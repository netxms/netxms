/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.epp.widgets.helpers;

import org.netxms.ui.eclipse.epp.widgets.Cell;
import org.netxms.ui.eclipse.epp.widgets.Rule;

/**
 * Abstract cell factory for policy editor
 *
 */
public abstract class CellFactory
{
	/**
	 * Create cell. Usually called by rule's constructor.
	 * 
	 * @param column Column number
	 * @param rule Rule object
	 * @param data Rule's data
	 * @return New cell widget
	 */
	public abstract Cell createCell(int column, Rule rule, Object data);
	
	/**
	 * Get number of columns in policy
	 * 
	 * @return Number of columns in policy
	 */
	public abstract int getColumnCount();

	/**
	 * Get default width for given column
	 * 
	 * @param index Column index
	 * @return Default width for given column
	 */
	public int getDefaultColumnWidth(int index)
	{
		return 200;
	}
}
