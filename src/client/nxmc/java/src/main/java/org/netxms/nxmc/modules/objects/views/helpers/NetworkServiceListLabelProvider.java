/**
 * NetXMS - open source network management system
 * Copyright (C) 2009 Raden Solutions
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
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.NetworkService;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.objects.views.NetworkServiceView;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * Label provider for VPNConnector
 */
public class NetworkServiceListLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
   public final String[] types = {"User defined", "SSH", "POP3", "SMTP", "FTP", "HTTP", "HTTPS", "Telnet"};
   public final String[] protocol = new String[256];
   private NXCSession session = Registry.getSession();

   @Override
   public Color getForeground(Object element, int columnIndex)
   {
      NetworkService ns = (NetworkService)element;
      switch(columnIndex)
      {
         case NetworkServiceView.COLUMN_STATUS:
            StatusDisplayInfo.getStatusColor(ns.getStatus());
      }
      initProtocolArray();
      return null;
   }

   @Override
   public Color getBackground(Object element, int columnIndex)
   {
      return null;
   }

   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      NetworkService ns = (NetworkService)element;
      switch(columnIndex)
      {
         case NetworkServiceView.COLUMN_ID:
            return Long.toString(ns.getObjectId());
         case NetworkServiceView.COLUMN_NAME:
            return ns.getObjectName();
         case NetworkServiceView.COLUMN_STATUS:
            return StatusDisplayInfo.getStatusText(ns.getStatus());
         case NetworkServiceView.COLUMN_SERVICE_TYPE:
            return types[ns.getServiceType()];
         case NetworkServiceView.COLUMN_ADDRESS:
            return ns.getIpAddress().getHostAddress();
         case NetworkServiceView.COLUMN_PORT:
            return Integer.toString(ns.getPort());
         case NetworkServiceView.COLUMN_REQUEST:
            return ns.getRequest();
         case NetworkServiceView.COLUMN_RESPONSE:
            return ns.getResponse();
         case NetworkServiceView.COLUMN_POLLER_NODE:
            return getPollerName(ns);
         case NetworkServiceView.COLUMN_POLL_COUNT:
            return Integer.toString(ns.getPollCount());
            
      }
      return null;
   }

   /**
    * @param iface
    * @return
    */
   public String getPollerName(NetworkService ns)
   {
      AbstractNode peer = (AbstractNode)session.findObjectById(ns.getPollerNode(), AbstractNode.class);
      return (peer != null) ? peer.getObjectName() : null;
   }
   
   /**
    * Initialize protocol array
    */
   private void initProtocolArray()
   {
      protocol[0] = "Dummy TCP";
      
   }
}
