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
package org.netxms.ui.eclipse.datacollection.widgets;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.widgets.internal.DciListComparator;
import org.netxms.ui.eclipse.datacollection.widgets.internal.DciListLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Show DCI on given node
 */
public class DciList extends Composite
{
	private static final long serialVersionUID = 1L;

	// Columns
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_PARAMETER = 1;
	public static final int COLUMN_DESCRIPTION = 2;
	
	private final ViewPart viewPart;
	private Node node;
	private NXCSession session;
	private SortableTableViewer viewer;
	
	/**
	 * Create "last values" widget
	 * 
	 * @param viewPart owning view part
	 * @param parent parent widget
	 * @param style style
	 * @param _node node to display data for
	 * @param configPrefix configuration prefix for saving/restoring viewer settings
	 */
	public DciList(ViewPart viewPart, Composite parent, int style, Node _node, final String configPrefix)
	{
		super(parent, style);
		session = (NXCSession)ConsoleSharedData.getSession();
		this.viewPart = viewPart;		
		this.node = _node;
		
		final IDialogSettings ds = Activator.getDefault().getDialogSettings();
		
		// Setup table columns
		final String[] names = { "ID", "Parameter", "Description" };
		final int[] widths = { 70, 150, 250 };
		viewer = new SortableTableViewer(this, names, widths, 2, SWT.DOWN, SWT.SINGLE | SWT.FULL_SELECTION);
	
		viewer.setLabelProvider(new DciListLabelProvider());
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setComparator(new DciListComparator());
		WidgetHelper.restoreTableViewerSettings(viewer, ds, configPrefix);
		
		addListener(SWT.Resize, new Listener() {
			private static final long serialVersionUID = 1L;

			public void handleEvent(Event e)
			{
				viewer.getControl().setBounds(DciList.this.getClientArea());
			}
		});
		
		viewer.getTable().addDisposeListener(new DisposeListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, ds, configPrefix);
			}
		});

		getDataFromServer();
	}
	
	/**
	 * Get data from server
	 */
	private void getDataFromServer()
	{
		if (node == null)
		{
			viewer.setInput(new DciValue[0]);
			return;
		}

		ConsoleJob job = new ConsoleJob("Get DCI values for node " + node.getObjectName(), viewPart, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get DCI list for node " + node.getObjectName();
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final DciValue[] data = session.getLastValues(node.getObjectId());
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(data);
					}
				});
			}
		};
		job.setUser(false);
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
	
	/**
	 * Refresh view
	 */
	public void refresh()
	{
		getDataFromServer();
	}
	
	/**
	 * Get selected DCI
	 * 
	 * @return selected DCI
	 */
	public DciValue getSelection()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		return (DciValue)selection.getFirstElement();
	}

	/**
	 * @return the viewer
	 */
	public SortableTableViewer getViewer()
	{
		return viewer;
	}
}
