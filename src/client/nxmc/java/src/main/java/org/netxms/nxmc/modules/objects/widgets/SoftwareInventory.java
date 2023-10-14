/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.objects.widgets.helpers.SoftwareInventoryContentProvider;
import org.netxms.nxmc.modules.objects.widgets.helpers.SoftwareInventoryNode;
import org.netxms.nxmc.modules.objects.widgets.helpers.SoftwarePackageComparator;
import org.netxms.nxmc.modules.objects.widgets.helpers.SoftwarePackageFilter;
import org.netxms.nxmc.modules.objects.widgets.helpers.SoftwarePackageLabelProvider;
import org.xnap.commons.i18n.I18n;

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

   private final I18n i18n = LocalizationHelper.getI18n(SoftwareInventory.class);
   private final String[] names = { i18n.tr("Name"), i18n.tr("Version"), i18n.tr("Vendor"), i18n.tr("Install Date"), i18n.tr("Description"), i18n.tr("URL") };
   private final int[] widths = { 200, 100, 200, 100, 300, 200 };

	private ObjectView view;
	private ColumnViewer viewer;
   private SoftwarePackageFilter filter = new SoftwarePackageFilter();
	private MenuManager menuManager = null;
	
	/**
	 * @param parent
	 * @param style
	 */
   public SoftwareInventory(Composite parent, int style, ObjectView view)
	{
		super(parent, style);
		this.view = view;
		
		setLayout(new FillLayout());
		createTableViewer();
	}

	/**
	 * Create viewer for table mode
	 */
	private void createTableViewer()
	{
		viewer = new SortableTableViewer(this, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SoftwarePackageLabelProvider(false));
		viewer.setComparator(new SoftwarePackageComparator());
      viewer.addFilter(filter);
      view.setFilterClient(viewer, filter);
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
		viewer.setContentProvider(new SoftwareInventoryContentProvider());
		viewer.setLabelProvider(new SoftwarePackageLabelProvider(true));
		viewer.setComparator(new SoftwarePackageComparator());
      viewer.addFilter(filter);
      view.setFilterClient(viewer, filter);
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
      final AbstractObject object = view.getObject();
      new Job(i18n.tr("Reading software package information"), view) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				if (object instanceof AbstractNode)
				{
               final List<SoftwarePackage> packages = session.getNodeSoftwarePackages(object.getObjectId());
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
                     if (viewer.getControl().isDisposed() || (object.getObjectId() != view.getObjectId()))
                        return;
                     viewer.setInput(packages.toArray());
                     ((SortableTableViewer)viewer).packColumns();
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
                     if (viewer.getControl().isDisposed() || (object.getObjectId() != view.getObjectId()))
                        return;
                     viewer.setInput(nodes);
                     ((SortableTreeViewer)viewer).packColumns();
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
	 * @param rootObjectId the rootObjectId to set
	 */
	public void setRootObjectId(long rootObjectId)
	{
		AbstractObject object = Registry.getSession().findObjectById(rootObjectId);
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

   /**
    * Get viewer filter.
    *
    * @return viewer filter
    */
   public SoftwarePackageFilter getFilter()
   {
      return filter;
   }
}
