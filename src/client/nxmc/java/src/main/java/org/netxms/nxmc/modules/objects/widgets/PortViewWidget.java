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
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.DashboardComposite;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortInfo;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortSelectionListener;
import org.netxms.nxmc.tools.FontTools;

/**
 * Widget for displaying switch/router ports
 */
public class PortViewWidget extends DashboardComposite
{
	private long nodeId;
	private NXCSession session;
	private Map<Long, PortInfo> ports = new HashMap<Long, PortInfo>();
	private Map<Long, SlotViewWidget> slots = new HashMap<Long, SlotViewWidget>();
	private Label header = null;
	private Font headerFont = null;
	private int portDisplayMode = SlotViewWidget.DISPLAY_MODE_STATE;
	private boolean headerVisible = false;
	private Set<PortSelectionListener> selectionListeners = new HashSet<PortSelectionListener>();
	private PortSelectionListener listener;

	/**
	 * @param parent
	 * @param style
	 */
	public PortViewWidget(Composite parent, int style)
	{
		super(parent, style);

      session = Registry.getSession();

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
		for(SlotViewWidget s : slots.values())
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
			public int compare(Interface i1, Interface i2)
			{
				if (i1.getModule() == i2.getModule())
            {
               if (i1.getPIC() == i2.getPIC())
                  return i1.getPort() - i2.getPort();
               return i1.getPIC() - i2.getPIC();
            }
				return i1.getModule() - i2.getModule();
			}
		});

		for(Interface iface : interfaces)
		{
			long hash = ((long)iface.getChassis() << 32) | (long)iface.getModule();
			SlotViewWidget sv = slots.get(hash);
			if (sv == null)
			{
				sv = new SlotViewWidget(this, SWT.NONE, String.format("%d/%d", iface.getChassis(), iface.getModule()), ((Node)object).getPortRowCount(), ((Node)object).getPortNumberingScheme());
				sv.setPortDisplayMode(portDisplayMode);
				slots.put(hash, sv);
			}

			PortInfo p = new PortInfo(iface);
			ports.put(iface.getObjectId(), p);
			sv.addPort(p);
		}

      layout(true, true);

		for(SlotViewWidget sv : slots.values())
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
	public int getPortDisplayMode()
	{
		return portDisplayMode;
	}

	/**
	 * @param portStatusVisible the portStatusVisible to set
	 */
	public void setPortDisplayMode(int portDisplayMode)
	{
		this.portDisplayMode = portDisplayMode;
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
		   if ((p.getModule() == 0) && (p.getPort() == 0))
		   {
		      Interface iface = session.findObjectById(p.getObjectId(), Interface.class);
		      if (iface == null)
		         continue;
            if (!iface.isPhysicalPort())
            {
               Interface parent = iface.getParentInterface();
               if ((parent == null) || !parent.isPhysicalPort())  
                  continue;
               p = new Port(parent.getObjectId(), parent.getIfIndex(), parent.getChassis(), parent.getModule(), parent.getPIC(), parent.getPort());
            }
		   }
			SlotViewWidget sv = slots.get(((long)p.getChassis() << 32) | (long)p.getModule());
			if (sv != null)
			{
				sv.addHighlight(p);
			}
		}

		for(SlotViewWidget sv : slots.values())
			sv.redraw();
	}
	
	/**
	 * Clear port highlight.
	 * 
	 * @param doRedraw if true, control will be redrawn
	 */
	public void clearHighlight(boolean doRedraw)
	{
		for(SlotViewWidget sv : slots.values())
		{
			sv.clearHighlight();
			if (doRedraw)
				sv.redraw();
		}
	}

   /**
    * @see org.eclipse.swt.widgets.Control#setMenu(org.eclipse.swt.widgets.Menu)
    */
	@Override
	public void setMenu(Menu menu)
	{
		super.setMenu(menu);
		for(SlotViewWidget sv : slots.values())
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
