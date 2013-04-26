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
package org.netxms.ui.eclipse.objectview.objecttabs;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.TreeViewerColumn;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.PhysicalComponent;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.ComponentTreeContentProvider;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.ComponentTreeLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Components" tab
 */
public class ComponentsTab extends ObjectTab
{
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_CLASS = 1;
	public static final int COLUMN_MODEL = 2;
	public static final int COLUMN_FIRMWARE = 3;
	public static final int COLUMN_SERIAL = 4;
	public static final int COLUMN_VENDOR = 5;
	
	private TreeViewer viewer;
	private Action actionCollapseAll;
	private Action actionExpandAll;

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		viewer = new TreeViewer(parent, SWT.FULL_SELECTION | SWT.MULTI);
		addColumn("Name", 200);
		addColumn("Class", 100);
		addColumn("Model", 150);
		addColumn("Firmware", 100);
		addColumn("Serial Number", 150);
		addColumn("Vendor", 150);
		viewer.setLabelProvider(new ComponentTreeLabelProvider());
		viewer.setContentProvider(new ComponentTreeContentProvider());
		viewer.getTree().setHeaderVisible(true);
		viewer.getTree().setLinesVisible(true);
		WidgetHelper.restoreColumnSettings(viewer.getTree(), Activator.getDefault().getDialogSettings(), "ComponentTree");
		viewer.getTree().addDisposeListener(new DisposeListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveColumnSettings(viewer.getTree(), Activator.getDefault().getDialogSettings(), "ComponentTree");
			}
		});
		createActions();
		createPopupMenu();
	}
	
	/**
	 * @param name
	 */
	private void addColumn(String name, int width)
	{
		TreeViewerColumn tc = new TreeViewerColumn(viewer, SWT.LEFT);
		tc.getColumn().setText(name);
		tc.getColumn().setWidth(width);
	}

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			private static final long serialVersionUID = 1L;

			public void menuAboutToShow(IMenuManager manager)
			{
				fillContextMenu(manager);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
		if (getViewPart() != null)
			getViewPart().getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		/*
		manager.add(actionCopy);
		manager.add(actionCopyName);
		manager.add(actionCopyModel);
		manager.add(actionCopySerial);
		manager.add(new Separator());
		*/
		manager.add(actionCollapseAll);
		manager.add(actionExpandAll);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionCollapseAll = new Action("C&ollapse all", SharedIcons.COLLAPSE_ALL) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				viewer.collapseAll();
			}
		};

		actionExpandAll = new Action("&Expand all", SharedIcons.EXPAND_ALL) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				viewer.expandAll();
			}
		};
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void objectChanged(final AbstractObject object)
	{
		viewer.setInput(new Object[0]);
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		ConsoleJob job = new ConsoleJob("Get node components", getViewPart(), Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				try
				{
					final PhysicalComponent root = session.getNodePhysicalComponents(object.getObjectId());
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							if (viewer.getTree().isDisposed())
								return;
							
							if ((ComponentsTab.this.getObject() != null) &&
							    (ComponentsTab.this.getObject().getObjectId() == object.getObjectId()))
							{
								viewer.setInput(new Object[] { root });
								viewer.expandAll();
							}
						}
					});
				}
				catch(NXCException e)
				{
					if (e.getErrorCode() != RCC.NO_COMPONENT_DATA)
						throw e;
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							if (viewer.getTree().isDisposed())
								return;
							
							if ((ComponentsTab.this.getObject() != null) &&
							    (ComponentsTab.this.getObject().getObjectId() == object.getObjectId()))
							{
								viewer.setInput(new Object[0]);
							}
						}
					});
				}
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get component information for node " + object.getObjectName();
			}
		};
		job.setUser(false);
		job.start();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean showForObject(AbstractObject object)
	{
		if (object instanceof Node)
		{
			return (((Node)object).getFlags() & Node.NF_HAS_ENTITY_MIB) != 0;
		}
		return false;
	}
}
