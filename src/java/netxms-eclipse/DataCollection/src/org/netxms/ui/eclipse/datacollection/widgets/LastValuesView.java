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
package org.netxms.ui.eclipse.datacollection.widgets;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.widgets.internal.LastValuesComparator;
import org.netxms.ui.eclipse.datacollection.widgets.internal.LastValuesLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * @author victor
 *
 */
public class LastValuesView extends Composite
{
	public static final String JOB_FAMILY = "LastValuesViewJob";
	
	// Columns
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_DESCRIPTION = 1;
	public static final int COLUMN_VALUE = 2;
	public static final int COLUMN_TIMESTAMP = 3;
	
	private final ViewPart viewPart;
	private Node node;
	private NXCSession session;
	private TableViewer dataViewer;
	
	public LastValuesView(ViewPart viewPart, Composite parent, int style, Node _node)
	{
		super(parent, style);
		session = ConsoleSharedData.getInstance().getSession();
		this.viewPart = viewPart;		
		this.node = _node;
		
		// Setup table columns
		final String[] names = { "ID", "Description", "Value", "Timestamp" };
		final int[] widths = { 70, 250, 150, 100 };
		dataViewer = new SortableTableViewer(this, names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
	
		dataViewer.setLabelProvider(new LastValuesLabelProvider());
		dataViewer.setContentProvider(new ArrayContentProvider());
		dataViewer.setComparator(new LastValuesComparator());
		
		createPopupMenu();

		addListener(SWT.Resize, new Listener() {
			public void handleEvent(Event e)
			{
				dataViewer.getControl().setBounds(LastValuesView.this.getClientArea());
			}
		});

		getDataFromServer();
	}
	
	/**
	 * Create pop-up menu for alarm list
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(dataViewer.getControl());
		dataViewer.getControl().setMenu(menu);

		// Register menu for extension.
		if (viewPart != null)
			viewPart.getSite().registerContextMenu(menuMgr, dataViewer);
	}
	
	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}
	
	/**
	 * Get data from server
	 */
	private void getDataFromServer()
	{
		if (node == null)
		{
			dataViewer.setInput(new DciValue[0]);
			return;
		}

		ConsoleJob job = new ConsoleJob("Get DCI values for node " + node.getObjectName(), viewPart, Activator.PLUGIN_ID, LastValuesView.JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get DCI values for node " + node.getObjectName();
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final DciValue[] data = session.getLastValues(node.getObjectId());
				new UIJob("Initialize last values view") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						dataViewer.setInput(data);
						return Status.OK_STATUS;
					}
				}.schedule();
			}
		};
		job.start();
	}
	
	/**
	 * Change node object
	 * 
	 * @param _node new node object
	 */
	public void setNode(Node _node)
	{
		this.node = _node;
		getDataFromServer();
	}
}
