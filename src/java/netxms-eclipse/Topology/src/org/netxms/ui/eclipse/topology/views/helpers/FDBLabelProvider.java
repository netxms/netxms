/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.FdbEntry;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.topology.views.SwitchForwardingDatabaseView;

/**
 * Label provider for switch forwarding database view
 */
public class FDBLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   
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
      FdbEntry e = (FdbEntry)element;
      switch(columnIndex)
      {
         case SwitchForwardingDatabaseView.COLUMN_INTERFACE:
            return e.getInterfaceName();
         case SwitchForwardingDatabaseView.COLUMN_MAC_ADDRESS:
            return e.getAddress().toString();
         case SwitchForwardingDatabaseView.COLUMN_NODE:
            if (e.getNodeId() == 0)
               return ""; //$NON-NLS-1$
            return session.getObjectName(e.getNodeId());
         case SwitchForwardingDatabaseView.COLUMN_PORT:
            if (e.getPort() == 0)
               return ""; //$NON-NLS-1$
            return Integer.toString(e.getPort());
         case SwitchForwardingDatabaseView.COLUMN_VLAN:
            if (e.getVlanId() == 0)
               return ""; //$NON-NLS-1$
            return Integer.toString(e.getVlanId());
      }
      return null;
   }
}
