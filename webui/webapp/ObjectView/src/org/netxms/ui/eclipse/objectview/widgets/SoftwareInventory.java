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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SoftwarePackage;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.objectview.widgets.helpers.SoftwareInventoryContentProvider;
import org.netxms.ui.eclipse.objectview.widgets.helpers.SoftwareInventoryNode;
import org.netxms.ui.eclipse.objectview.widgets.helpers.SoftwarePackageComparator;
import org.netxms.ui.eclipse.objectview.widgets.helpers.SoftwarePackageLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Software inventory information widget
 */
public class SoftwareInventory extends Composite
{
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_VERSION = 1;
	public static final int COLUMN_VENDOR = 2;
	public static final int COLUMN_DATE = 3;
	public static final int COLUMN_DESCRIPTION = 4;
	public static final int COLUMN_URL = 5;
	
	private final String[] names = { Messages.get().SoftwareInventory_Name, Messages.get().SoftwareInventory_Version, Messages.get().SoftwareInventory_Vendor, Messages.get().SoftwareInventory_InstallDate, Messages.get().SoftwareInventory_Description, Messages.get().SoftwareInventory_URL };
	private static final int[] widths = { 200, 100, 200, 100, 300, 200 };
	
	private IViewPart viewPart;
	private long rootObjectId;
	private ColumnViewer viewer;
	private String configPrefix;
	private MenuManager menuManager = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public SoftwareInventory(Composite parent, int style, IViewPart viewPart, String configPrefix)
	{
		super(parent, style);
		this.viewPart = viewPart;
		this.configPrefix = configPrefix;
		
		setLayout(new FillLayout());
		createTableViewer();
	}
	
	/**
	 * Create viewer for table mode
	 */
	private void createTableViewer()
	{
		viewer = new SortableTableViewer(this, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
		WidgetHelper.restoreColumnViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configPrefix);
		viewer.getControl().addDisposeListener(new DisposeListener() {			
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveColumnViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configPrefix);
			}
		});
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SoftwarePackageLabelProvider(false));
		viewer.setComparator(new SoftwarePackageComparator());
		
		if (menuManager != null)
		{
			Menu menu = menuManager.createContextMenu(viewer.getControl());
			viewer.getControl().setMenu(menu);
			viewPart.getSite().registerContextMenu(menuManager, viewer);
		}
	}
	
	/**
	 * Create viewer for tree mode
	 */
	private void createTreeViewer()
	{
		viewer = new SortableTreeViewer(this, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
		WidgetHelper.restoreColumnViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configPrefix);
		viewer.getControl().addDisposeListener(new DisposeListener() {			
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveColumnViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configPrefix);
			}
		});
		viewer.setContentProvider(new SoftwareInventoryContentProvider());
		viewer.setLabelProvider(new SoftwarePackageLabelProvider(true));
		viewer.setComparator(new SoftwarePackageComparator());
		
		if (menuManager != null)
		{
			Menu menu = menuManager.createContextMenu(viewer.getControl());
			viewer.getControl().setMenu(menu);
			viewPart.getSite().registerContextMenu(menuManager, viewer);
		}
	}
	
	/**
	 * Refresh list
	 */
	public void refresh()
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.get().SoftwareInventory_JobName, viewPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				AbstractObject object = session.findObjectById(rootObjectId);
				if (object instanceof AbstractNode)
				{
					final List<SoftwarePackage> packages = session.getNodeSoftwarePackages(rootObjectId);
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							viewer.setInput(packages.toArray());
						}
					});
				}
				else
				{
					final List<SoftwareInventoryNode> nodes = new ArrayList<SoftwareInventoryNode>();
					for(final AbstractObject o : object.getAllChilds(AbstractObject.OBJECT_NODE))
					{
						try
						{
							List<SoftwarePackage> packages = session.getNodeSoftwarePackages(o.getObjectId());
							nodes.add(new SoftwareInventoryNode((Node)o, packages));
						}
						catch(NXCException e)
						{
							if (e.getErrorCode() != RCC.NO_SOFTWARE_PACKAGE_DATA)
								throw e;
						}
					}
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							viewer.setInput(nodes);
						}
					});
				}
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().SoftwareInventory_JobError;
			}
		}.start();
	}

	/**
	 * @return the rootObjectId
	 */
	public long getRootObjectId()
	{
		return rootObjectId;
	}

	/**
	 * @param rootObjectId the rootObjectId to set
	 */
	public void setRootObjectId(long rootObjectId)
	{
		this.rootObjectId = rootObjectId;
		AbstractObject object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(rootObjectId);
		if (object instanceof Node)
		{
			if (!(viewer instanceof SortableTableViewer))
			{
				viewer.getControl().dispose();
				createTableViewer();
			}
		}
		else
		{
			if (!(viewer instanceof SortableTreeViewer))
			{
				viewer.getControl().dispose();
				createTreeViewer();
			}
		}
	}

	/**
	 * @return the viewer
	 */
	public ColumnViewer getViewer()
	{
		return viewer;
	}
	
	/**
	 * @param manager
	 */
	public void setViewerMenu(MenuManager manager)
	{
		menuManager = manager;
		if (viewer != null)
		{
			Menu menu = menuManager.createContextMenu(viewer.getControl());
			viewer.getControl().setMenu(menu);
			viewPart.getSite().registerContextMenu(menuManager, viewer);
		}
	}
}
