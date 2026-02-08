/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.base.MacAddress;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Interface;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.InterfacesView;
import org.netxms.nxmc.modules.objects.widgets.helpers.DecoratingObjectLabelProvider;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.ViewerElementUpdater;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for interface list
 */
public class InterfaceListLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(InterfaceListLabelProvider.class);
   private final String[] ifaceExpectedState = { i18n.tr("Up"), i18n.tr("Down"), i18n.tr("Ignore"), i18n.tr("Auto") };

	private AbstractObject object = null;
   private NXCSession session = Registry.getSession();
   private TableViewer viewer;
   private DecoratingObjectLabelProvider objectLabelProvider;

   /**
    * Constructor
    * 
    * @param viewer viewer
    */
   public InterfaceListLabelProvider(TableViewer viewer)
   {
      this.viewer = viewer;
      objectLabelProvider = new DecoratingObjectLabelProvider();
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      Interface iface = (Interface)element;
      switch((Integer)viewer.getTable().getColumn(columnIndex).getData("ID"))
      {
         case InterfacesView.COLUMN_NODE:
            return objectLabelProvider.getImage(iface.getParentNode());
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{	         
	   if (object == null)
         return "";

		Interface iface = (Interface)element;
      MacAddress macAddr;
		switch((Integer)viewer.getTable().getColumn(columnIndex).getData("ID"))
		{
         case InterfacesView.COLUMN_NODE:
            return iface.getParentNode().getNameWithAlias();
         case InterfacesView.COLUMN_8021X_BACKEND_STATE:
				if (iface.getParentNode().is8021xSupported() && iface.isPhysicalPort())
					return iface.getDot1xBackendStateAsText();
				return null;
         case InterfacesView.COLUMN_8021X_PAE_STATE:
				if (iface.getParentNode().is8021xSupported() && iface.isPhysicalPort())
					return iface.getDot1xPaeStateAsText();
				return null;
         case InterfacesView.COLUMN_ADMIN_STATE:
				return iface.getAdminStateAsText();
         case InterfacesView.COLUMN_ALIAS:
            return iface.getIfAlias();
         case InterfacesView.COLUMN_DESCRIPTION:
				return iface.getDescription();
         case InterfacesView.COLUMN_EXPECTED_STATE:
				return ifaceExpectedState[iface.getExpectedState()];
         case InterfacesView.COLUMN_ID:
				return Long.toString(iface.getObjectId());
         case InterfacesView.COLUMN_INDEX:
				return Integer.toString(iface.getIfIndex());
         case InterfacesView.COLUMN_IP_ADDRESS:
            return iface.getIpAddressListAsString();
         case InterfacesView.COLUMN_NIC_VENDOR:
            return session.getVendorByMac(iface.getMacAddress(), new ViewerElementUpdater(viewer, element));
         case InterfacesView.COLUMN_MAC_ADDRESS:
            return iface.getMacAddress().toString();
         case InterfacesView.COLUMN_MAX_SPEED:
            return (iface.getMaxSpeed() > 0) ? ifSpeedTotext(iface.getMaxSpeed()) : "";
         case InterfacesView.COLUMN_MTU:
            return Integer.toString(iface.getMtu());
         case InterfacesView.COLUMN_NAME:
				return iface.getObjectName();
         case InterfacesView.COLUMN_OPER_STATE:
				return iface.getOperStateAsText();
         case InterfacesView.COLUMN_OSPF_AREA:
            return iface.isOSPF() ? iface.getOSPFArea().getHostAddress() : "";
         case InterfacesView.COLUMN_OSPF_STATE:
            return iface.isOSPF() ? iface.getOSPFState().getText() : "";
         case InterfacesView.COLUMN_OSPF_TYPE:
            return iface.isOSPF() ? iface.getOSPFType().getText() : "";
         case InterfacesView.COLUMN_PEER_INTERFACE:
            return getPeerInterfaceName(iface);
         case InterfacesView.COLUMN_PEER_IP_ADDRESS:
            return getPeerIpAddress(iface);
         case InterfacesView.COLUMN_PEER_LAST_UPDATED:
            return (iface.getPeerLastUpdateTime().getTime() != 0) ? DateFormatFactory.getDateTimeFormat().format(iface.getPeerLastUpdateTime()) : "";
         case InterfacesView.COLUMN_PEER_MAC_ADDRESS:
            macAddr = getPeerMacAddress(iface);
            return (macAddr != null) ? macAddr.toString() : null;
         case InterfacesView.COLUMN_PEER_NIC_VENDOR:
            macAddr = getPeerMacAddress(iface);
            return (macAddr != null) ? session.getVendorByMac(macAddr, new ViewerElementUpdater(viewer, element)) : null;
         case InterfacesView.COLUMN_PEER_NODE:
            return getPeerNodeName(iface);
         case InterfacesView.COLUMN_PEER_PROTOCOL:
            return getPeerProtocol(iface);
         case InterfacesView.COLUMN_PHYSICAL_LOCATION:
            if (iface.isPhysicalPort())
               return iface.getPhysicalLocation();
            return null;
         case InterfacesView.COLUMN_SPEED:
            return ((iface.getOperState() == Interface.OPER_STATE_UP) && (iface.getSpeed() > 0)) ? ifSpeedTotext(iface.getSpeed()) : "";
         case InterfacesView.COLUMN_STATUS:
            return StatusDisplayInfo.getStatusText(iface.getStatus());
         case InterfacesView.COLUMN_STP_STATE:
            return iface.getStpPortState().getText();
         case InterfacesView.COLUMN_TYPE:
            String typeName = iface.getIfTypeName();
            return (typeName != null) ? String.format("%d (%s)", iface.getIfType(), typeName) : Integer.toString(iface.getIfType());
         case InterfacesView.COLUMN_UTILIZATION:
            int in = iface.getInboundUtilization();
            int out = iface.getOutboundUtilization();
            if ((in < 0) && (out < 0))
               return "";
            return i18n.tr("In: {0} / Out: {1}", (in >= 0) ? formatUtilizationValue(in) : "?", (out >= 0) ? formatUtilizationValue(out) : "?");
         case InterfacesView.COLUMN_VLAN:
            return getVlanList(iface);
		}
		return null;
	}

   /**
    * Format interface utilization value.
    *
    * @param v interface utilization value
    * @return formatted string
    */
   private static String formatUtilizationValue(int v)
   {
      return Integer.toString(v / 10) + "." + Integer.toString(v % 10) + "%";
   }

	/**
	 * @param iface
	 * @return
	 */
	private String getPeerIpAddress(Interface iface)
	{
      AbstractObject peer = session.findObjectById(iface.getPeerNodeId());
		if (peer == null)
			return null;

      if (peer instanceof AbstractNode)
      {
         if (!((AbstractNode)peer).getPrimaryIP().isValidUnicastAddress())
            return null;
         return ((AbstractNode)peer).getPrimaryIP().getHostAddress();
      }

      if (peer instanceof AccessPoint)
      {
         if (!((AccessPoint)peer).getIpAddress().isValidUnicastAddress())
            return null;
         return ((AccessPoint)peer).getIpAddress().getHostAddress();
      }

      return null;
	}

	/**
	 * @param iface
	 * @return
	 */
	public MacAddress getPeerMacAddress(Interface iface)
	{
      AbstractObject peer = session.findObjectById(iface.getPeerInterfaceId());
      if (peer == null)
         return null;
      if (peer instanceof Interface)
         return ((Interface)peer).getMacAddress();
      if (peer instanceof AccessPoint)
         return ((AccessPoint)peer).getMacAddress();
      return null;
	}

   /**
    * @param iface
    * @return
    */
   private String getPeerProtocol(Interface iface)
   {
      return (iface.getPeerInterfaceId() != 0) ? iface.getPeerDiscoveryProtocol().toString() : null;
   }

	/**
	 * @param iface
	 * @return
	 */
	private String getPeerNodeName(Interface iface)
	{
      AbstractObject peer = session.findObjectById(iface.getPeerNodeId());
		return (peer != null) ? peer.getObjectName() : null;
	}

   /**
    * @param iface
    * @return
    */
   private String getPeerInterfaceName(Interface iface)
   {
      Interface peer = session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      return (peer != null) ? peer.getObjectName() : null;
   }

	/**
	 * @param object the object to set
	 */
	public void setNode(AbstractObject object)
	{
		this.object = object;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
    */
	@Override
	public Color getForeground(Object element, int columnIndex)
	{
		Interface iface = (Interface)element;
		switch((Integer)viewer.getTable().getColumn(columnIndex).getData("ID"))
		{
         case InterfacesView.COLUMN_STATUS:
				return StatusDisplayInfo.getStatusColor(iface.getStatus());
         case InterfacesView.COLUMN_OPER_STATE:
				switch(iface.getOperState())
				{
					case Interface.OPER_STATE_UP:
						return StatusDisplayInfo.getStatusColor(ObjectStatus.NORMAL);
					case Interface.OPER_STATE_DOWN:
						return StatusDisplayInfo.getStatusColor((iface.getAdminState() == Interface.ADMIN_STATE_DOWN) ? ObjectStatus.DISABLED : ObjectStatus.CRITICAL);
					case Interface.OPER_STATE_TESTING:
						return StatusDisplayInfo.getStatusColor(ObjectStatus.TESTING);
               case Interface.OPER_STATE_DORMANT:
                  return StatusDisplayInfo.getStatusColor(ObjectStatus.MINOR);
               case Interface.OPER_STATE_NOT_PRESENT:
                  return StatusDisplayInfo.getStatusColor(ObjectStatus.DISABLED);
				}
				return StatusDisplayInfo.getStatusColor(ObjectStatus.UNKNOWN);
         case InterfacesView.COLUMN_ADMIN_STATE:
				switch(iface.getAdminState())
				{
					case Interface.ADMIN_STATE_UP:
						return StatusDisplayInfo.getStatusColor(ObjectStatus.NORMAL);
					case Interface.ADMIN_STATE_DOWN:
						return StatusDisplayInfo.getStatusColor(ObjectStatus.DISABLED);
					case Interface.ADMIN_STATE_TESTING:
						return StatusDisplayInfo.getStatusColor(ObjectStatus.TESTING);
				}
				return StatusDisplayInfo.getStatusColor(ObjectStatus.UNKNOWN);
			default:
				return null;
		}
	}

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
    */
	@Override
	public Color getBackground(Object element, int columnIndex)
	{
		return null;
	}

   /**
    * @param speed
    * @param power
    * @return
    */
   private static String divideSpeed(long speed, int power)
   {
      String s = Long.toString(speed);
      StringBuilder sb = new StringBuilder(s.substring(0, s.length() - power));
      char[] rem = s.substring(s.length() - power, s.length()).toCharArray();
      int l = rem.length - 1;
      while((l >= 0) && (rem[l] == '0'))
         l--;
      if (l >= 0)
      {
         sb.append('.');
         for(int i = 0; i <= l; i++)
            sb.append(rem[i]);
      }
      return sb.toString();
   }

   /**
    * Convert interface speed in bps to human readable form
    * 
    * @param speed
    * @return
    */
   public static String ifSpeedTotext(long speed)
   {
      if (speed < 1000)
         return Long.toString(speed) + " bps";

      if (speed < 1000000)
         return divideSpeed(speed, 3) + " Kbps";

      if (speed < 1000000000)
         return divideSpeed(speed, 6) + " Mbps";

      if (speed < 1000000000000L)
         return divideSpeed(speed, 9) + " Gbps";

      return divideSpeed(speed, 12) + " Tbps";
   }

   /**
    * Get VLAN list from interface as string
    * 
    * @param iface interface object
    * @return VLAN list as string
    */
   public static String getVlanList(Interface iface)
   {
      long[] vlans = iface.getVlans();
      if (vlans.length == 0)
         return "";
      StringBuilder sb = new StringBuilder();
      sb.append(vlans[0]);
      for(int i = 1; i < vlans.length; i++)
      {
         sb.append(", ");
         sb.append(vlans[i]);
      }
      return sb.toString();
   }
}
