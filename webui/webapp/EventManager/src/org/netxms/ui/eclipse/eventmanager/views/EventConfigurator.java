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
package org.netxms.ui.eclipse.eventmanager.views;

import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.eventmanager.widgets.EventObjectList;

/**
 * Event configuration view
 * 
 */
public class EventConfigurator extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.eventmanager.view.event_configurator"; //$NON-NLS-1$
	public static final String JOB_FAMILY = "EventConfiguratorJob"; //$NON-NLS-1$
	
   private static final String TABLE_CONFIG_PREFIX = "EventTemplateList"; //$NON-NLS-1$

	private EventObjectList dataView;

   /* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{		
      parent.setLayout(new FormLayout());
      
      dataView = new EventObjectList(this, parent, SWT.NONE, TABLE_CONFIG_PREFIX);
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(100, 0);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      dataView.setLayoutData(fd);
      
		dataView.getViewer().addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
			   dataView.getActionEdit().run();
			}
		});
		
		contributeToActionBars();
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
		manager.add(dataView.getActionNewTemplate());
		manager.add(new Separator());
      manager.add(dataView.getActionShowFilter());
      manager.add(new Separator());
      manager.add(dataView.getActionShowGroups());
		manager.add(dataView.getActionRefresh());
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(dataView.getActionNewTemplate());
      manager.add(dataView.getActionShowFilter());
		manager.add(new Separator());
		manager.add(dataView.getActionRefresh());
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(dataView.getActionRefresh());
		mgr.add(dataView.getActionDelete());
      mgr.add(dataView.getActionNewGroup());
		mgr.add(dataView.getActionRemoveFromGroup());
		mgr.add(new Separator());
		mgr.add(dataView.getActionShowGroups());
		mgr.add(dataView.getActionEdit());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		dataView.getViewer().getControl().setFocus();
	}
}
