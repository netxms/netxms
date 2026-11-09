/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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

import java.util.Date;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.dhcp.DhcpLease;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.DhcpView;
import org.netxms.nxmc.tools.ViewerElementUpdater;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for DHCP lease list
 */
public class DhcpLeaseLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(DhcpLeaseLabelProvider.class);
   private final String[] typeNames = { i18n.tr("Dynamic"), i18n.tr("Reservation") };
   private final String[] stateNames = { i18n.tr("Active"), i18n.tr("Expired"), i18n.tr("Declined"), i18n.tr("Released"), i18n.tr("Offered"), i18n.tr("Inactive") };

   private NXCSession session = Registry.getSession();
   private TableViewer viewer;

   /**
    * Constructor
    *
    * @param viewer owning viewer
    */
   public DhcpLeaseLabelProvider(TableViewer viewer)
   {
      this.viewer = viewer;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      DhcpLease lease = (DhcpLease)element;
      switch(columnIndex)
      {
         case DhcpView.LEASE_IP_ADDRESS:
            return getAddressText(lease);
         case DhcpView.LEASE_MAC_ADDRESS:
            return hasMacAddress(lease) ? lease.getMacAddress().toString() : "";
         case DhcpView.LEASE_VENDOR:
            return hasMacAddress(lease) ? session.getVendorByMac(lease.getMacAddress(), new ViewerElementUpdater(viewer, element)) : "";
         case DhcpView.LEASE_HOSTNAME:
            return lease.getHostname();
         case DhcpView.LEASE_CLIENT_ID:
            return lease.getClientId();
         case DhcpView.LEASE_SCOPE:
            return lease.getScopeId();
         case DhcpView.LEASE_TYPE:
            return getTypeName(lease);
         case DhcpView.LEASE_STATE:
            return getStateName(lease);
         case DhcpView.LEASE_LAST_TRANSACTION:
            return formatDate(lease.getLastTransaction());
         case DhcpView.LEASE_EXPIRATION:
            return formatDate(lease.getExpiration());
      }
      return null;
   }

   /**
    * Get lease address as text (with prefix length for IPv6 prefix delegation).
    *
    * @param lease lease
    * @return lease address as text
    */
   public static String getAddressText(DhcpLease lease)
   {
      if ((lease.getIpAddress() == null) || (lease.getIpAddress().getAddress() == null))
         return "";
      return lease.isPrefixDelegation() ? lease.getIpAddress().toString() : lease.getIpAddress().getHostAddress();
   }

   /**
    * Check if lease has valid MAC address.
    *
    * @param lease lease
    * @return true if lease has valid MAC address
    */
   public static boolean hasMacAddress(DhcpLease lease)
   {
      return (lease.getMacAddress() != null) && !lease.getMacAddress().isNull();
   }

   /**
    * Get display name for lease type.
    *
    * @param lease lease
    * @return display name for lease type
    */
   public String getTypeName(DhcpLease lease)
   {
      int t = lease.getType().getValue();
      return ((t >= 0) && (t < typeNames.length)) ? typeNames[t] : "";
   }

   /**
    * Get display name for lease state.
    *
    * @param lease lease
    * @return display name for lease state
    */
   public String getStateName(DhcpLease lease)
   {
      int s = lease.getState().getValue();
      return ((s >= 0) && (s < stateNames.length)) ? stateNames[s] : i18n.tr("Unknown");
   }

   /**
    * Format optional date.
    */
   private static String formatDate(Date d)
   {
      return (d != null) ? DateFormatFactory.getDateTimeFormat().format(d) : "";
   }
}
