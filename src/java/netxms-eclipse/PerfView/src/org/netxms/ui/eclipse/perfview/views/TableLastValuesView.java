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
package org.netxms.ui.eclipse.perfview.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.MobileDevice;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.perfview.widgets.TableValue;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Display last value of table DCI
 */
public class TableLastValuesView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.perfview.views.TableLastValues";
	
	private long objectId;
	private long dciId;
	private TableValue viewer;
	private Action actionRefresh;
	private Action actionExportAllToCsv;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		
		// Secondary ID must by in form nodeId&dciId
		String[] parts = site.getSecondaryId().split("&");
		if (parts.length != 2)
			throw new PartInitException("Internal error");
		
		objectId = Long.parseLong(parts[0]);
		AbstractObject object = session.findObjectById(objectId);
		if ((object == null) || (!(object instanceof AbstractNode) && !(object instanceof Cluster) && !(object instanceof MobileDevice)))
			throw new PartInitException("Invalid object ID");
		
		dciId = Long.parseLong(parts[1]);
		
		setPartName(object.getObjectName() + ": [" + Long.toString(dciId) + "]");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		viewer = new TableValue(parent, SWT.NONE, this);

		createActions();
		contributeToActionBars();
	
		viewer.setObject(objectId, dciId);
		refreshTable();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refreshTable();
			}
		};

		actionExportAllToCsv = new ExportToCsvAction(this, viewer.getViewer(), false);
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
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionExportAllToCsv);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionExportAllToCsv);
		manager.add(actionRefresh);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.setFocus();
	}
	
	/**
	 * Refresh table
	 */
	private void refreshTable()
	{
		viewer.refresh(new Runnable() {	
			@Override
			public void run()
			{
				setPartName(viewer.getObjectName() + ": " + viewer.getTitle());
			}
		});
	}
}
