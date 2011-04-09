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
package org.netxms.ui.eclipse.topology.views;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.topology.VlanInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.topology.views.helpers.VlanLabelProvider;
import org.netxms.ui.eclipse.topology.widgets.DeviceView;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Display VLAN configuration on a node
 */
public class VlanView extends ViewPart implements ISelectionChangedListener
{
	public static final String ID = "org.netxms.ui.eclipse.topology.views.VlanView";
	
	public static final int COLUMN_VLAN_ID = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_PORTS = 2;
	
	private long nodeId;
	private List<VlanInfo> vlans = new ArrayList<VlanInfo>(0);
	private NXCSession session;
	private SortableTableViewer vlanList;
	private DeviceView deviceView;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		nodeId = Long.parseLong(site.getSecondaryId());
		session = (NXCSession)ConsoleSharedData.getSession();
		GenericObject object = session.findObjectById(nodeId);
		setPartName("VLAN View - " + ((object != null) ? object.getObjectName() : "<" + site.getSecondaryId() + ">"));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		parent.setLayout(layout);
		
		final String[] names = { "ID", "Name", "Ports" };
		final int[] widths = { 80, 180, 400 };
		vlanList = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		vlanList.setContentProvider(new ArrayContentProvider());
		vlanList.setLabelProvider(new VlanLabelProvider());
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		vlanList.getTable().setLayoutData(gd);
		vlanList.setInput(vlans.toArray());
		vlanList.addSelectionChangedListener(this);
		
		deviceView = new DeviceView(parent, SWT.NONE);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		deviceView.setLayoutData(gd);
		deviceView.setPortStatusVisible(false);
		deviceView.setNodeId(nodeId);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		vlanList.getControl().setFocus();
	}

	/**
	 * @param vlans the vlans to set
	 */
	public void setVlans(List<VlanInfo> vlans)
	{
		this.vlans = vlans;
		vlanList.setInput(vlans.toArray());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ISelectionChangedListener#selectionChanged(org.eclipse.jface.viewers.SelectionChangedEvent)
	 */
	@Override
	public void selectionChanged(SelectionChangedEvent event)
	{
		IStructuredSelection selection = (IStructuredSelection)vlanList.getSelection();
		VlanInfo vlan = (VlanInfo)selection.getFirstElement();
		if (vlan != null)
		{
			
		}
		else
		{
			
		}
	}
}
