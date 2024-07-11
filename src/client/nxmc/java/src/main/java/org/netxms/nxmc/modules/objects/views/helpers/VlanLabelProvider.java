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
package org.netxms.nxmc.modules.objects.views.helpers;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
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
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.objects.views.PortView;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortInfo;

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
		session = Registry.getSession();
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
			case PortView.COLUMN_VLAN_ID:
				return Integer.toString(vlan.getVlanId());
			case PortView.COLUMN_NAME:
				return vlan.getName();
			case PortView.COLUMN_PORTS:
				return buildPortList(vlan);
         case PortView.COLUMN_INTERFACES:
            return buildInterfaceList(vlan);
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
		List<Port> physicalPorts = new ArrayList<Port>();
		for(Port p : vlan.getPorts())
		{
			Interface iface = session.findObjectById(p.getObjectId(), Interface.class);
			if (iface != null)
			{
				if (!iface.isPhysicalPort())
				{
				   Interface parent = iface.getParentInterface();
				   if ((parent != null) && parent.isPhysicalPort())
				   {
				      physicalPorts.add(new Port(parent.getObjectId(), parent.getIfIndex(), parent.getChassis(), parent.getModule(), parent.getPIC(), parent.getPort()));
				   }
				}
				else
				{
				   physicalPorts.add(p);
				}
			}
			else
			{
			   physicalPorts.add(p);
			}
		}
		
		if (physicalPorts.isEmpty())
			return "";
		
		Collections.sort(physicalPorts, new Comparator<Port>() {
         @Override
         public int compare(Port p1, Port p2)
         {
            int diff = p1.getChassis() - p2.getChassis();
            if (diff == 0)
               diff = p1.getModule() - p2.getModule();
            if (diff == 0)
               diff = p1.getPIC() - p2.getPIC();
            if (diff == 0)
               diff = p1.getPort() - p2.getPort();
            return diff;
         }
      });
		
      int chassis = physicalPorts.get(0).getChassis();
		int module = physicalPorts.get(0).getModule();
      int pic = physicalPorts.get(0).getPIC();
		int lastPort = physicalPorts.get(0).getPort();
		int firstPort = lastPort;

      StringBuilder sb = new StringBuilder();

		sb.append(chassis);
		sb.append('/');
      sb.append(module);
      sb.append('/');
      sb.append(pic);
      sb.append('/');
		sb.append(firstPort);
		
		int i;
		for(i = 1; i < physicalPorts.size(); i++)
		{
			if ((physicalPorts.get(i).getChassis() == chassis) &&
			    (physicalPorts.get(i).getModule() == module) &&
			    (physicalPorts.get(i).getPIC() == pic) &&
			    (physicalPorts.get(i).getPort() == lastPort + 1))
			{
				lastPort++;
				continue;
			}
			
			// If previous series was not single port, add ending port
			if (physicalPorts.get(i - 1).getPort() != firstPort)
			{
				if (lastPort - firstPort > 1)
					sb.append(" - ");
				else
				   sb.append(", ");
		      sb.append(chassis);
		      sb.append('/');
		      sb.append(module);
		      sb.append('/');
		      sb.append(pic);
				sb.append('/');
				sb.append(lastPort);
			}
			
         chassis = physicalPorts.get(i).getChassis();
         module = physicalPorts.get(i).getModule();
			pic = physicalPorts.get(i).getPIC();
			lastPort = physicalPorts.get(i).getPort();
			firstPort = lastPort;
			
			sb.append(", ");
	      sb.append(chassis);
	      sb.append('/');
	      sb.append(module);
	      sb.append('/');
	      sb.append(pic);
			sb.append('/');
			sb.append(lastPort);
		}

		// If previous series was not single port, add ending port
		if (physicalPorts.get(i - 1).getPort() != firstPort)
		{
			if (lastPort - firstPort > 1)
				sb.append(" - ");
			else
			   sb.append(", ");
	      sb.append(chassis);
	      sb.append('/');
	      sb.append(module);
	      sb.append('/');
	      sb.append(pic);
			sb.append('/');
			sb.append(lastPort);
		}
		
		return sb.toString();
	}

   /**
    * Build list of interfaces in VLAN
    *
    * @param vlan VLAN object
    * @return port list
    */
   private String buildInterfaceList(VlanInfo vlan)
   {
      StringBuilder sb = new StringBuilder();
      for(Port p : vlan.getPorts())
      {
         Interface iface = session.findObjectById(p.getObjectId(), Interface.class);
         if (iface != null)
         {
            if (sb.length() > 0)
               sb.append(", ");
            sb.append(iface.getObjectName());
         }
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
      return (selectedPort != null) && ((VlanInfo)element).containsPort(selectedPort.getChassis(), selectedPort.getModule(), selectedPort.getPIC(), selectedPort.getPort()) ? HIGHLIGHT : null;
   }
}
