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
package org.netxms.ui.eclipse.topology.views.helpers;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Interface;
import org.netxms.client.topology.Port;
import org.netxms.client.topology.VlanInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.topology.views.VlanView;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortInfo;

/**
 * Label provider for VLAN list
 */
public class VlanLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private static final Color HIGHLIGHT = new Color(Display.getDefault(), 255, 216, 0);
   
	private NXCSession session;
	private PortInfo selectedPort = null;
	
	/**
	 * Default constructor
	 */
	public VlanLabelProvider()
	{
		super();
		session = (NXCSession)ConsoleSharedData.getSession();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		VlanInfo vlan = (VlanInfo)element;
		switch(columnIndex)
		{
			case VlanView.COLUMN_VLAN_ID:
				return Integer.toString(vlan.getVlanId());
			case VlanView.COLUMN_NAME:
				return vlan.getName();
			case VlanView.COLUMN_PORTS:
				return buildPortList(vlan);
			default:
				break;
		}
		return null;
	}
	
	/**
	 * Build list of ports in VLAN
	 * 
	 * @param vlan VLAN object
	 * @return port list
	 */
	private String buildPortList(VlanInfo vlan)
	{
		List<Port> ports = new ArrayList<Port>(Arrays.asList(vlan.getPorts()));
		if (ports.size() == 0)
			return ""; //$NON-NLS-1$
		
		StringBuilder sb = new StringBuilder();

		// Add non-physical ports first
		Iterator<Port> it = ports.iterator();
		while(it.hasNext())
		{
			Port p = it.next();
			Interface iface = (Interface)session.findObjectById(p.getObjectId(), Interface.class);
			if (iface != null)
			{
				if ((iface.getFlags() & Interface.IF_PHYSICAL_PORT) == 0)
				{
					if (sb.length() > 0)
						sb.append(',');
					sb.append(iface.getObjectName());
					it.remove();
				}
			}
		}
		
		if (ports.size() == 0)
			return sb.toString();
		
		int slot = ports.get(0).getSlot();
		int lastPort = ports.get(0).getPort();
		int firstPort = lastPort;

		if (sb.length() > 0)
			sb.append(',');
		sb.append(slot);
		sb.append('/');
		sb.append(firstPort);
		
		int i;
		for(i = 1; i < ports.size(); i++)
		{
			if ((ports.get(i).getSlot() == slot) && (ports.get(i).getPort() == lastPort + 1))
			{
				lastPort++;
				continue;
			}
			
			// If previous series was not single port, add ending port
			if (ports.get(i - 1).getPort() != firstPort)
			{
				if (lastPort - firstPort > 1)
					sb.append('-');
				else
					sb.append(',');
				sb.append(slot);
				sb.append('/');
				sb.append(lastPort);
			}
			
			slot = ports.get(i).getSlot();
			lastPort = ports.get(i).getPort();
			firstPort = lastPort;
			
			sb.append(',');
			sb.append(slot);
			sb.append('/');
			sb.append(lastPort);
		}

		// If previous series was not single port, add ending port
		if (ports.get(i - 1).getPort() != firstPort)
		{
			if (lastPort - firstPort > 1)
				sb.append('-');
			else
				sb.append(',');
			sb.append(slot);
			sb.append('/');
			sb.append(lastPort);
		}
		
		return sb.toString();
	}

   /**
    * @param selectedPort the selectedPort to set
    */
   public boolean setSelectedPort(PortInfo selectedPort)
   {
      if (selectedPort == this.selectedPort)
         return false;
      this.selectedPort = selectedPort;
      return true;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      return (selectedPort != null) && ((VlanInfo)element).containsPort(selectedPort.getSlot(), selectedPort.getPort()) ? HIGHLIGHT : null;
   }
}
