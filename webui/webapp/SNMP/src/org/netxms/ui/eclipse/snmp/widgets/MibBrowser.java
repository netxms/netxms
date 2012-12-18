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
package org.netxms.ui.eclipse.snmp.widgets;

import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Tree;
import org.netxms.client.snmp.MibObject;
import org.netxms.ui.eclipse.snmp.shared.MibCache;
import org.netxms.ui.eclipse.snmp.widgets.helpers.MibObjectComparator;
import org.netxms.ui.eclipse.snmp.widgets.helpers.MibTreeContentProvider;
import org.netxms.ui.eclipse.snmp.widgets.helpers.MibTreeLabelProvider;

/**
 * MIB browser widget
 *
 */
public class MibBrowser extends Composite
{
	private static final long serialVersionUID = 1L;

	private TreeViewer mibTree;
	
	/**
	 * Create new MIB tree browser
	 * 
	 * @param parent parent composite
	 * @param style SWT style
	 */
	public MibBrowser(Composite parent, int style)
	{
		super(parent, style);
		
		setLayout(new FillLayout());
		
		mibTree = new TreeViewer(this, SWT.NONE);
		mibTree.setContentProvider(new MibTreeContentProvider());
		mibTree.setLabelProvider(new MibTreeLabelProvider());
		mibTree.setComparator(new MibObjectComparator());
		mibTree.setInput(MibCache.getMibTree());
	}
	
	/**
	 * Get currently selected MIB object.
	 * 
	 * @return currently selected MIB object or null if there are no selection
	 */
	public MibObject getSelection()
	{
		IStructuredSelection selection = (IStructuredSelection)mibTree.getSelection();
		return (MibObject)selection.getFirstElement();
	}
	
	/**
	 * Add tree selection listener.
	 * 
	 * @param listener Listener
	 */
	public void addSelectionChangedListener(ISelectionChangedListener listener)
	{
		mibTree.addSelectionChangedListener(listener);
	}

	/**
	 * Set current selection
	 * @param object object to be selected
	 */
	public void setSelection(MibObject object)
	{
		mibTree.setSelection(new StructuredSelection(object), true);
	}

	/**
	 * Refresh MIB tree
	 */
	public void refreshTree()
	{
		mibTree.setInput(MibCache.getMibTree());
	}
	
	/**
	 * Get underlying tree control
	 * 
	 * @return tree control
	 */
	public Tree getTreeControl()
	{
		return mibTree.getTree();
	}

	/**
	 * Get underlying tree viewer
	 * 
	 * @return tree viewer
	 */
	public TreeViewer getTreeViewer()
	{
		return mibTree;
	}
}
