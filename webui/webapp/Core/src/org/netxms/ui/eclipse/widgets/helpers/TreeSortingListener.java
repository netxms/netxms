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
package org.netxms.ui.eclipse.widgets.helpers;

import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeColumn;

/**
 * Table control column selection listener used to change sorting column
 */
public class TreeSortingListener extends SelectionAdapter
{
	private static final long serialVersionUID = 1L;

	private TreeViewer viewer;
	
	/**
	 * Constructor
	 */
	public TreeSortingListener(TreeViewer viewer)
	{
		this.viewer = viewer;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.SelectionAdapter#widgetSelected(org.eclipse.swt.events.SelectionEvent)
	 */
	@Override
	public void widgetSelected(SelectionEvent e)
	{
		Tree tree = viewer.getTree();
		TreeColumn sortColumn = tree.getSortColumn();
		if (sortColumn == (TreeColumn)e.getSource())
		{
			tree.setSortDirection(tree.getSortDirection() == SWT.UP ? SWT.DOWN : SWT.UP);
		}
		else
		{
			tree.setSortColumn((TreeColumn)e.getSource());
		}
		viewer.refresh(false);
	}
}
