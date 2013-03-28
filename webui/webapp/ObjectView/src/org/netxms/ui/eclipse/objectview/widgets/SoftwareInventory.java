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
package org.netxms.ui.eclipse.objectview.widgets;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SoftwarePackage;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.widgets.helpers.SoftwarePackageComparator;
import org.netxms.ui.eclipse.objectview.widgets.helpers.SoftwarePackageLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Software inventory information widget
 */
public class SoftwareInventory extends Composite
{
	private static final long serialVersionUID = 1L;

	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_VERSION = 1;
	public static final int COLUMN_VENDOR = 2;
	public static final int COLUMN_DATE = 3;
	public static final int COLUMN_DESCRIPTION = 4;
	public static final int COLUMN_URL = 5;
	
	private IViewPart viewPart;
	private long nodeId;
	private SortableTableViewer viewer;
	
	/**
	 * @param parent
	 * @param style
	 */
	public SoftwareInventory(Composite parent, int style, IViewPart viewPart, final String configPrefix)
	{
		super(parent, style);
		this.viewPart = viewPart;
		
		setLayout(new FillLayout());
		
		final String[] names = { "Name", "Version", "Vendor", "Install Date", "Description", "URL" };
		final int[] widths = { 200, 100, 200, 100, 300, 200 };
		viewer = new SortableTableViewer(this, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configPrefix);
		viewer.getTable().addDisposeListener(new DisposeListener() {			
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configPrefix);
			}
		});
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SoftwarePackageLabelProvider());
		viewer.setComparator(new SoftwarePackageComparator());
	}
	
	/**
	 * Refresh list
	 */
	public void refresh()
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Read software package information", viewPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<SoftwarePackage> packages = session.getNodeSoftwarePackages(nodeId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(packages.toArray());
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get software package information for node";
			}
		}.start();
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @param nodeId the nodeId to set
	 */
	public void setNodeId(long nodeId)
	{
		this.nodeId = nodeId;
	}
}
