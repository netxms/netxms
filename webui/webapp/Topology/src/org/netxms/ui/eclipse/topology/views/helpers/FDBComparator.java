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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.FdbEntry;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.topology.views.SwitchForwardingDatabaseView;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for FDB records
 */
public class FDBComparator extends ViewerComparator
{
   private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      FdbEntry fdb1 = (FdbEntry)e1;
      FdbEntry fdb2 = (FdbEntry)e2;
      
      int result;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
      {
         case SwitchForwardingDatabaseView.COLUMN_INTERFACE:
            result = fdb1.getInterfaceName().compareToIgnoreCase(fdb2.getInterfaceName());
            break;
         case SwitchForwardingDatabaseView.COLUMN_MAC_ADDRESS:
            result = fdb1.getAddress().compareTo(fdb2.getAddress());
            break;
         case SwitchForwardingDatabaseView.COLUMN_NODE:
            String n1 = (fdb1.getNodeId() != 0) ? session.getObjectName(fdb1.getNodeId()) : ""; //$NON-NLS-1$
            String n2 = (fdb2.getNodeId() != 0) ? session.getObjectName(fdb2.getNodeId()) : ""; //$NON-NLS-1$
            result = n1.compareToIgnoreCase(n2);
            break;
         case SwitchForwardingDatabaseView.COLUMN_PORT:
            result = fdb1.getPort() - fdb2.getPort();
            break;
         case SwitchForwardingDatabaseView.COLUMN_VLAN:
            result = fdb1.getVlanId() - fdb2.getVlanId();
            break;
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.DOWN) ? result : -result;
   }
}
