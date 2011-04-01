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

import java.util.List;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.topology.VlanInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Display VLAN configuration on a node
 */
public class VlanView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.topology.views.VlanView";
	
	private long nodeId;
	private List<VlanInfo> vlans;
	private NXCSession session;
	
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
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		// TODO Auto-generated method stub

	}

	/**
	 * @param vlans the vlans to set
	 */
	public void setVlans(List<VlanInfo> vlans)
	{
		this.vlans = vlans;
	}
}
