/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Raden Solutions
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
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.Port;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.FontTools;
import org.netxms.ui.eclipse.topology.Messages;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortInfo;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortSelectionListener;
import org.netxms.ui.eclipse.widgets.DashboardComposite;

/**
 * View of switch/router ports
 *
 */
public class DeviceView extends DashboardComposite
{
	private long nodeId;
	private NXCSession session;
	private Map<Long, PortInfo> ports = new HashMap<Long, PortInfo>();
	private Map<Integer, SlotView> slots = new HashMap<Integer, SlotView>();
	private Label header = null;
	private Font headerFont = null;
	private boolean portStatusVisible = true;
	private boolean headerVisible = false;
	private Set<PortSelectionListener> selectionListeners = new HashSet<PortSelectionListener>();
	private PortSelectionListener listener;
	
	/**
	 * @param parent
	 * @param style
	 */
	public DeviceView(Composite parent, int style)
	{
		super(parent, style);
		
		session = (NXCSession)ConsoleSharedData.getSession();
		
		RowLayout layout = new RowLayout();
		layout.type = SWT.VERTICAL;
		layout.fill = true;
		layout.wrap = false;
		setLayout(layout);
		
		setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
		
		listener = new PortSelectionListener() {
			@Override
			public void portSelected(PortInfo port)
			{
				for(PortSelectionListener l : selectionListeners)
					l.portSelected(port);
			}
		};
	}
	
	/**
	 * Refresh widget
	 */
	public void refresh()
	{
		AbstractObject object = session.findObjectById(nodeId);
		if ((object == null) || !(object instanceof Node))
			return;

		if (!headerVisible && (header != null))
		{
		   header.dispose();
		   header = null;
		}
		for(SlotView s : slots.values())
		{
			s.setMenu(null);
			s.dispose();
		}
		slots.clear();
		ports.clear();
		
		if (headerVisible)
		{
		   if (header == null)
		   {
		      header = new Label(this, SWT.NONE);
		      header.setBackground(getBackground());
		      if (headerFont == null)
		      {
		         headerFont = FontTools.createTitleFont();
		         addDisposeListener(new DisposeListener() {
                  @Override
                  public void widgetDisposed(DisposeEvent e)
                  {
                     headerFont.dispose();
                  }
               });
		      }
		      header.setFont(headerFont);
		   }
		   header.setText(object.getObjectName());
		}
		
		List<Interface> interfaces = new ArrayList<Interface>();
		for(AbstractObject o: object.getAllChildren(AbstractObject.OBJECT_INTERFACE))
		{
			if (((Interface)o).isPhysicalPort())
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
			SlotView sv = slots.get(slot);
			if (sv == null)
			{
				sv = new SlotView(this, SWT.NONE, String.format(Messages.get().DeviceView_SlotName, slot), ((Node)object).getPortRowCount(), ((Node)object).getPortNumberingScheme());
				sv.setPortStatusVisible(portStatusVisible);
				slots.put(slot, sv);
			}

			PortInfo p = new PortInfo(iface);
			ports.put(iface.getObjectId(), p);
			sv.addPort(p);
		}
		
		layout();
		
		for(SlotView sv : slots.values())
		{
			sv.setMenu(getMenu());
			sv.addSelectionListener(listener);
		}
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

	/**
	 * @return the portStatusVisible
	 */
	public boolean isPortStatusVisible()
	{
		return portStatusVisible;
	}

	/**
	 * @param portStatusVisible the portStatusVisible to set
	 */
	public void setPortStatusVisible(boolean portStatusVisible)
	{
		this.portStatusVisible = portStatusVisible;
	}
	
	/**
    * @return the headerVisible
    */
   public boolean isHeaderVisible()
   {
      return headerVisible;
   }

   /**
    * @param headerVisible the headerVisible to set
    */
   public void setHeaderVisible(boolean headerVisible)
   {
      this.headerVisible = headerVisible;
   }

   /**
	 * Set port highlight
	 * 
	 * @param ports
	 */
	public void setHighlight(Port[] ports)
	{
		clearHighlight(false);
		
		for(Port p : ports)
		{
		   // Some devices has slots and ports numbering started at 0, so
		   // port 0/0 could be valid - in that case check interface object
		   // for physical port flag
		   if ((p.getSlot() == 0) && (p.getPort() == 0))
		   {
		      Interface iface = session.findObjectById(p.getObjectId(), Interface.class);
		      if (iface == null)
		         continue;
            if (!iface.isPhysicalPort())
            {
               Interface parent = iface.getParentInterface();
               if ((parent == null) || !parent.isPhysicalPort())  
                  continue;
               p = new Port(parent.getObjectId(), parent.getIfIndex(), parent.getSlot(), parent.getPort());
            }
		   }
			SlotView sv = slots.get(p.getSlot());
			if (sv != null)
			{
				sv.addHighlight(p);
			}
		}
		
		for(SlotView sv : slots.values())
			sv.redraw();
	}
	
	/**
	 * Clear port highlight.
	 * 
	 * @param doRedraw if true, control will be redrawn
	 */
	public void clearHighlight(boolean doRedraw)
	{
		for(SlotView sv : slots.values())
		{
			sv.clearHighlight();
			if (doRedraw)
				sv.redraw();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Control#setMenu(org.eclipse.swt.widgets.Menu)
	 */
	@Override
	public void setMenu(Menu menu)
	{
		super.setMenu(menu);
		for(SlotView sv : slots.values())
			sv.setMenu(getMenu());
	}

	/**
	 * Add selection listener
	 * 
	 * @param listener
	 */
	public void addSelectionListener(PortSelectionListener listener)
	{
		selectionListeners.add(listener);
	}
	
	/**
	 * Remove selection listener
	 * 
	 * @param listener
	 */
	public void removeSelectionListener(PortSelectionListener listener)
	{
		selectionListeners.remove(listener);
	}
}
