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
package org.netxms.ui.eclipse.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.library.Activator;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Abstract trace view (like event trace)
 */
public abstract class AbstractTraceView extends ViewPart
{
	private static final int MAX_ELEMENTS = 1000;
	
	private TableViewer viewer;
	private List<Object> data = new ArrayList<Object>(MAX_ELEMENTS);
	private boolean paused = false;
	private Action actionClear;
	private Action actionPause;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		viewer = new TableViewer(parent, SWT.FULL_SELECTION | SWT.SINGLE);
		viewer.getTable().setHeaderVisible(true);
		viewer.setContentProvider(new ArrayContentProvider());
		setupViewer(viewer);
		WidgetHelper.restoreColumnSettings(viewer.getTable(), getDialogSettings(), getConfigPrefix());
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveColumnSettings(viewer.getTable(), getDialogSettings(), getConfigPrefix());
			}
		});
	
		createActions();
		contributeToActionBars();
	}

	/**
	 * Create actions
	 */
	protected void createActions()
	{
		actionClear = new Action("&Clear", SharedIcons.CLEAR_LOG) {
			@Override
			public void run()
			{
				clear();
			}
		};
		
		actionPause = new Action("&Pause", Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				setPaused(actionPause.isChecked());
			}
		};
		actionPause.setImageDescriptor(Activator.getImageDescriptor("icons/pause.png"));
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
	protected void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionPause);
		manager.add(actionClear);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	protected void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionPause);
		manager.add(actionClear);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

	/**
	 * Setup table vewer
	 * 
	 * @param viewer
	 */
	protected abstract void setupViewer(TableViewer viewer);
	
	/**
	 * Add column to viewer
	 * 
	 * @param name
	 * @param width
	 */
	protected void addColumn(String name, int width)
	{
		final TableColumn tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText(name);
		tc.setWidth(width);
	}
	
	/**
	 * Add new element to the viewer
	 * 
	 * @param element
	 */
	protected void addElement(Object element)
	{
		if (data.size() == MAX_ELEMENTS)
			data.remove(data.size() - 1);
		
		data.add(0, element);
		if (!paused)
			viewer.setInput(data.toArray());
	}
	
	/**
	 * Clear viewer
	 */
	protected void clear()
	{
		data.clear();
		viewer.setInput(new Object[0]);
	}
	
	/**
	 * Refresh viewer
	 */
	protected void refresh()
	{
		viewer.refresh();
	}

	/**
	 * @return the paused
	 */
	protected boolean isPaused()
	{
		return paused;
	}

	/**
	 * @param paused the paused to set
	 */
	protected void setPaused(boolean paused)
	{
		this.paused = paused;
		if (!paused)
			viewer.setInput(data.toArray());
	}
	
	/**
	 * Get dialog settings for saving viewer config
	 * 
	 * @return
	 */
	protected abstract IDialogSettings getDialogSettings();
	
	/**
	 * Get configuration prefix for saving viewer config
	 * 
	 * @return
	 */
	protected abstract String getConfigPrefix();
}
