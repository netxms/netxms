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
package org.netxms.ui.eclipse.objectbrowser.api;

import org.netxms.client.objects.AbstractObject;

/**
 * Interface for validators of various actions on NetXMS objects
 */
public interface ObjectActionValidator
{
	public static final int APPROVE = 0;
	public static final int REJECT = 1;
	public static final int IGNORE = 2;
	
	/**
	 * Check if current selection is valid for moving object.
	 * 
	 * @param subtree
	 * @param currentObject
	 * @param parentObject
	 * @return APPROVE, REJECT, or IGNORE
	 */
	public int isValidSelectionForMove(SubtreeType subtree, AbstractObject currentObject, AbstractObject parentObject);
}
