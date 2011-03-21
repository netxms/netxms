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
package org.netxms.ui.eclipse.epp.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.epp.Activator;
import org.netxms.ui.eclipse.epp.SituationCache;
import org.netxms.ui.eclipse.epp.views.helpers.SituationTreeContentProvider;
import org.netxms.ui.eclipse.epp.views.helpers.SituationTreeLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Situation manager
 */
public class SituationsManager extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.epp.views.SituationsManager";
	
	private TreeViewer situationTree;
	private SortableTableViewer details;
	
	private Action actionRefresh;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		SashForm splitter = new SashForm(parent, SWT.HORIZONTAL);
		
		situationTree = new TreeViewer(splitter, SWT.BORDER | SWT.MULTI);
		situationTree.setContentProvider(new SituationTreeContentProvider());
		situationTree.setLabelProvider(new SituationTreeLabelProvider());
		situationTree.setInput(SituationCache.getAllSituations());
		
		final String[] names = { "Attribute", "Value" };
		final int[] widths = { 150, 150 };
		details = new SortableTableViewer(splitter, names, widths, 0, SWT.UP, SWT.BORDER | SWT.FULL_SELECTION);
		details.getTable().setHeaderVisible(true);
		details.getTable().setLinesVisible(true);
		WidgetHelper.restoreTableViewerSettings(details, Activator.getDefault().getDialogSettings(), "SituationInstanceDetails");

		createActions();
		contributeToActionBars();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
				situationTree.setInput(SituationCache.getAllSituations());
			}
		};
	}
	
	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionRefresh);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		situationTree.getTree().setFocus();
	}
}
