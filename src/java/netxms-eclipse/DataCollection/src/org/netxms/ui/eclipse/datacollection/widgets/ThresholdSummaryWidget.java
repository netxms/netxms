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
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.TreeViewerColumn;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.ThresholdViolationSummary;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.widgets.internal.ThresholdTreeContentProvider;
import org.netxms.ui.eclipse.datacollection.widgets.internal.ThresholdTreeLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Widget to show threshold violation summary
 */
public class ThresholdSummaryWidget extends Composite
{
	private GenericObject object;
	private IViewPart viewPart;
	private TreeViewer viewer;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ThresholdSummaryWidget(Composite parent, int style, IViewPart viewPart)
	{
		super(parent, style);
		this.viewPart = viewPart;
		setLayout(new FillLayout());

		viewer = new TreeViewer(this, SWT.FULL_SELECTION);
		setupViewer();
		
	}
	
	private void setupViewer()
	{
		viewer.getTree().setHeaderVisible(true);
		
		TreeViewerColumn column = new TreeViewerColumn(viewer, SWT.LEFT);
		column.getColumn().setText("Node");
		column.getColumn().setWidth(200);

		column = new TreeViewerColumn(viewer, SWT.LEFT);
		column.getColumn().setText("Status");
		column.getColumn().setWidth(100);

		column = new TreeViewerColumn(viewer, SWT.LEFT);
		column.getColumn().setText("Parameter");
		column.getColumn().setWidth(250);

		column = new TreeViewerColumn(viewer, SWT.LEFT);
		column.getColumn().setText("Value");
		column.getColumn().setWidth(100);

		column = new TreeViewerColumn(viewer, SWT.LEFT);
		column.getColumn().setText("Condition");
		column.getColumn().setWidth(100);

		column = new TreeViewerColumn(viewer, SWT.LEFT);
		column.getColumn().setText("Since");
		column.getColumn().setWidth(140);
		
		viewer.setContentProvider(new ThresholdTreeContentProvider());
		viewer.setLabelProvider(new ThresholdTreeLabelProvider());
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
