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
         if ((iface != null) && !iface.isPhysicalPort())
			{
            Interface parent = iface.getParentInterface();
            if ((parent != null) && parent.isPhysicalPort())
               p = new Port(parent);
            else
               continue;
			}
         if (p.isLocationValid())
			   physicalPorts.add(p);
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
            if (diff == 0)
               diff = p1.getLane() - p2.getLane();
            return diff;
         }
      });

      // Consecutive ports of same PIC, as well as consecutive lanes of same split port, are shown as range
      StringBuilder sb = new StringBuilder();
      Port first = physicalPorts.get(0);
      Port last = first;
      for(int i = 1; i < physicalPorts.size(); i++)
		{
         Port p = physicalPorts.get(i);
         if (isNextInSeries(last, p))
			{
            last = p;
				continue;
			}
         appendPortSeries(sb, first, last);
         first = p;
         last = p;
		}
      appendPortSeries(sb, first, last);
		return sb.toString();
	}

   /**
    * Check if given port directly follows previous one (next port on same PIC or next lane on same split port).
    *
    * @param prev previous port
    * @param next port to check
    * @return true if given port directly follows previous one
    */
   private static boolean isNextInSeries(Port prev, Port next)
   {
      if ((prev.getChassis() != next.getChassis()) || (prev.getModule() != next.getModule()) || (prev.getPIC() != next.getPIC()))
         return false;
      if ((prev.getLane() < 0) && (next.getLane() < 0))
         return next.getPort() == prev.getPort() + 1;
      return (prev.getLane() >= 0) && (next.getLane() >= 0) && (next.getPort() == prev.getPort()) && (next.getLane() == prev.getLane() + 1);
   }

   /**
    * Append series of ports to port list.
    *
    * @param sb string builder for port list
    * @param first first port in series
    * @param last last port in series
    */
   private static void appendPortSeries(StringBuilder sb, Port first, Port last)
   {
      if (sb.length() > 0)
         sb.append(", ");
      appendPortLocation(sb, first);
      if (last != first)
      {
         int distance = (first.getLane() >= 0) ? (last.getLane() - first.getLane()) : (last.getPort() - first.getPort());
         sb.append((distance > 1) ? " - " : ", ");
         appendPortLocation(sb, last);
      }
   }

   /**
    * Append port location to port list.
    *
    * @param sb string builder for port list
    * @param p port
    */
   private static void appendPortLocation(StringBuilder sb, Port p)
   {
      sb.append(p.getChassis());
      sb.append('/');
      sb.append(p.getModule());
      sb.append('/');
      sb.append(p.getPIC());
      sb.append('/');
      sb.append(p.getPort());
      if (p.getLane() >= 0)
      {
         sb.append(':');
         sb.append(p.getLane());
      }
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
      return (selectedPort != null) && ((VlanInfo)element).containsPort(new Port(selectedPort.getInterfaceObjectId(), 0, selectedPort.getChassis(), selectedPort.getModule(),
            selectedPort.getPIC(), selectedPort.getPort(), selectedPort.getLane(), true)) ? HIGHLIGHT : null;
   }
}
