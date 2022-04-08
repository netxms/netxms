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
package org.netxms.nxmc.modules.objects.widgets;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
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
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SoftwarePackage;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.helpers.SoftwareInventoryContentProvider;
import org.netxms.nxmc.modules.objects.widgets.helpers.SoftwareInventoryNode;
import org.netxms.nxmc.modules.objects.widgets.helpers.SoftwarePackageComparator;
import org.netxms.nxmc.modules.objects.widgets.helpers.SoftwarePackageLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Software inventory information widget
 */
public class SoftwareInventory extends Composite
{
   private static final I18n i18n = LocalizationHelper.getI18n(SoftwareInventory.class);
   
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_VERSION = 1;
	public static final int COLUMN_VENDOR = 2;
	public static final int COLUMN_DATE = 3;
	public static final int COLUMN_DESCRIPTION = 4;
	public static final int COLUMN_URL = 5;
	
	private static final String[] names = { i18n.tr("Name"), i18n.tr("Version"), i18n.tr("Vendor"), i18n.tr("Install Date"), i18n.tr("Description"), i18n.tr("URL") };
	private static final int[] widths = { 200, 100, 200, 100, 300, 200 };
	
	private View viewPart;
	private long rootObjectId;
	private ColumnViewer viewer;
	private String configPrefix;
	private MenuManager menuManager = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public SoftwareInventory(Composite parent, int style, View viewPart, String configPrefix)
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
		WidgetHelper.restoreColumnViewerSettings(viewer, configPrefix);
		viewer.getControl().addDisposeListener(new DisposeListener() {			
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveColumnViewerSettings(viewer, configPrefix);
			}
		});
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SoftwarePackageLabelProvider(false));
		viewer.setComparator(new SoftwarePackageComparator());
		
		if (menuManager != null)
		{
			Menu menu = menuManager.createContextMenu(viewer.getControl());
			viewer.getControl().setMenu(menu);
		}
	}
	
	/**
	 * Create viewer for tree mode
	 */
	private void createTreeViewer()
	{
		viewer = new SortableTreeViewer(this, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
		WidgetHelper.restoreColumnViewerSettings(viewer, configPrefix);
		viewer.getControl().addDisposeListener(new DisposeListener() {			
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveColumnViewerSettings(viewer, configPrefix);
			}
		});
		viewer.setContentProvider(new SoftwareInventoryContentProvider());
		viewer.setLabelProvider(new SoftwarePackageLabelProvider(true));
		viewer.setComparator(new SoftwarePackageComparator());
		
		if (menuManager != null)
		{
			Menu menu = menuManager.createContextMenu(viewer.getControl());
			viewer.getControl().setMenu(menu);
		}
	}
	
	/**
	 * Refresh list
	 */
	public void refresh()
	{
		final NXCSession session = Registry.getSession();
		new Job(i18n.tr("Read software package information"), viewPart) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
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
					for(final AbstractObject o : object.getAllChildren(AbstractObject.OBJECT_NODE))
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
				return i18n.tr("Cannot get software package information for node");
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
		AbstractObject object = Registry.getSession().findObjectById(rootObjectId);
      System.out.println(viewer);
		if (object instanceof Node)
		{
			if (!(viewer instanceof SortableTableViewer))
			{
				viewer.getControl().dispose();
				createTableViewer();
		      layout(true, true);
			}
		}
		else
		{
			if (!(viewer instanceof SortableTreeViewer))
			{
				viewer.getControl().dispose();
				createTreeViewer();
		      layout(true, true);
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
		}
	}

	/**
	 * Clear invenotry list
	 */
   public void clear()
   {
      viewer.setInput(new ArrayList<Objects>());
   }
}
