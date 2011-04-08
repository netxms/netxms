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
package org.netxms.ui.eclipse.topology.widgets;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortInfo;

/**
 * View of switch/router ports
 *
 */
public class PortView extends Composite
{
	private long nodeId;
	private NXCSession session;
	private Map<Long, PortInfo> ports = new HashMap<Long, PortInfo>();
	private Map<Integer, SlotView> slots = new HashMap<Integer, SlotView>(); 
	
	/**
	 * @param parent
	 * @param style
	 */
	public PortView(Composite parent, int style)
	{
		super(parent, style);
		
		session = (NXCSession)ConsoleSharedData.getSession();
		
		RowLayout layout = new RowLayout();
		layout.type = SWT.VERTICAL;
		setLayout(layout);
	}
	
	/**
	 * Refresh widget
	 */
	public void refresh()
	{
		GenericObject object = session.findObjectById(nodeId);
		if ((object == null) || !(object instanceof Node))
			return;
		
		slots.clear();
		ports.clear();
		
		List<Interface> interfaces = new ArrayList<Interface>();
		for(GenericObject o: object.getAllChilds(GenericObject.OBJECT_INTERFACE))
		{
			if (((Interface)o).getSlot() > 0)
				interfaces.add((Interface)o);
		}
		Collections.sort(interfaces, new Comparator<Interface>() {
			@Override
			public int compare(Interface arg0, Interface arg1)
			{
				if (arg0.getSlot() == arg1.getSlot())
					return arg0.getPort() - arg1.getPort();
				return arg0.getSlot() - arg1.getSlot();
			}
		});
		
		for(Interface iface : interfaces)
		{
			int slot = iface.getSlot();
			ports.put(iface.getObjectId(), new PortInfo(iface));
			SlotView sv = slots.get(slot);
			if (sv == null)
			{
				sv = new SlotView(this, SWT.NONE);
				slots.put(slot, sv);
			}
		}
		
		layout();
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
		refresh();
	}
}
