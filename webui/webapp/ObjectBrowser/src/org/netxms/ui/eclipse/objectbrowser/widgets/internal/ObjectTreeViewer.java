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
package org.netxms.ui.eclipse.objectbrowser.widgets.internal;

import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.TreeItem;

/**
 * Custom extension of tree viewer for object tree
 */
public class ObjectTreeViewer extends TreeViewer
{
	private static final long serialVersionUID = 1L;

	/**
	 * @param parent
	 * @param style
	 */
	public ObjectTreeViewer(Composite parent, int style)
	{
		super(parent, style);
	}

	/**
	 * @param item
	 * @return
	 */
	public ViewerRow getTreeViewerRow(TreeItem item)
	{
		return getViewerRowFromItem(item);
	}
	
	/**
	 * Toggle item's expanded/collapsed state
	 * 
	 * @param item
	 */
	public void toggleItemExpandState(TreeItem item)
	{
		if (item.getExpanded())
		{
			item.setExpanded(false);
		}
		else
		{
			createChildren(item);
			item.setExpanded(true);
		}
	}
}
