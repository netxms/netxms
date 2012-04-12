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
package org.netxms.ui.eclipse.datacollection.widgets;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.ThresholdViolationSummary;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.widgets.internal.ThresholdTreeComparator;
import org.netxms.ui.eclipse.datacollection.widgets.internal.ThresholdTreeContentProvider;
import org.netxms.ui.eclipse.datacollection.widgets.internal.ThresholdTreeLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Widget to show threshold violation summary
 */
public class ThresholdSummaryWidget extends Composite
{
	public static final int COLUMN_NODE = 0;
	public static final int COLUMN_STATUS = 1;
	public static final int COLUMN_PARAMETER = 2;
	public static final int COLUMN_VALUE = 3;
	public static final int COLUMN_CONDITION = 4;
	public static final int COLUMN_TIMESTAMP = 5;
	
	private GenericObject object;
	private IViewPart viewPart;
	private SortableTreeViewer viewer;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ThresholdSummaryWidget(Composite parent, int style, IViewPart viewPart)
	{
		super(parent, style);
		this.viewPart = viewPart;
		setLayout(new FillLayout());

		final String[] names = { "Node",  "Status", "Parameter", "Value", "Condition", "Since" };
		final int[] widths = { 200, 100, 250, 100, 100, 140 };
		viewer = new SortableTreeViewer(this, names, widths, COLUMN_NODE, SWT.UP, SWT.FULL_SELECTION);
		viewer.setContentProvider(new ThresholdTreeContentProvider());
		viewer.setLabelProvider(new ThresholdTreeLabelProvider());		
		viewer.setComparator(new ThresholdTreeComparator());

		createPopupMenu();
	}

	/**
	 * Create pop-up menu for alarm list
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
		if (viewPart != null)
			viewPart.getSite().registerContextMenu(menuMgr, viewer);
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
	 * Refresh widget
	 */
	public void refresh()
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final long rootId = object.getObjectId();
		ConsoleJob job = new ConsoleJob("Get threshold summary", viewPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<ThresholdViolationSummary> data = session.getThresholdSummary(rootId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if (isDisposed() || (object == null) || (rootId != object.getObjectId()))
							return;
						viewer.setInput(data);
						viewer.expandAll();
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get threshold summary";
			}
		};
		job.setUser(false);
		job.start();
	}

	/**
	 * @param object the object to set
	 */
	public void setObject(GenericObject object)
	{
		this.object = object;
		refresh();
	}
}
