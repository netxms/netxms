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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AccessPoint;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.AccessPointsView;
import org.netxms.nxmc.tools.ComparatorHelper;

/**
 * Comparator for access point list
 */
public class AccessPointListComparator extends ViewerComparator
{
   private NXCSession session = Registry.getSession();
   
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
      final AccessPoint ap1 = (AccessPoint)e1;
      final AccessPoint ap2 = (AccessPoint)e2;
      final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");

		int result;
		switch(column)
		{
         case AccessPointsView.COLUMN_CONTROLLER:
            AbstractNode controller1 = session.findObjectById(ap1.getControllerId(), AbstractNode.class);
            AbstractNode controller2 = session.findObjectById(ap2.getControllerId(), AbstractNode.class);
            if ((controller1 != null) && (controller2 != null))
            {
               result = controller1.getObjectName().compareToIgnoreCase(controller2.getObjectName());
            }
            else
            {
               result = Long.signum(controller1.getPhysicalContainerId() - controller2.getPhysicalContainerId());
            }
            break;
         case AccessPointsView.COLUMN_ID:
            result = Long.signum(ap1.getObjectId() - ap2.getObjectId());
            break;
         case AccessPointsView.COLUMN_IP_ADDRESS:
            result = ComparatorHelper.compareInetAddresses(ap1.getIpAddress().getAddress(), ap2.getIpAddress().getAddress());
            break;
         case AccessPointsView.COLUMN_MAC_ADDRESS:
            result = ap1.getMacAddress().compareTo(ap2.getMacAddress());
            break;
         case AccessPointsView.COLUMN_MODEL:
            result = ap1.getModel().compareToIgnoreCase(ap2.getModel());
            break;
         case AccessPointsView.COLUMN_NAME:
            result = ap1.getNameWithAlias().compareToIgnoreCase(ap2.getNameWithAlias());
            break;
         case AccessPointsView.COLUMN_SERIAL_NUMBER:
            result = ap1.getSerialNumber().compareToIgnoreCase(ap2.getSerialNumber());
            break;
         case AccessPointsView.COLUMN_STATE:
            result = ap1.getState().compareTo(ap2.getState());
            break;
         case AccessPointsView.COLUMN_STATUS:
            result = ap1.getStatus().compareTo(ap2.getStatus());
            break;
         case AccessPointsView.COLUMN_VENDOR:
            result = ap1.getVendor().compareToIgnoreCase(ap2.getVendor());
            break;
			default:
				result = 0;
				break;
		}

		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
