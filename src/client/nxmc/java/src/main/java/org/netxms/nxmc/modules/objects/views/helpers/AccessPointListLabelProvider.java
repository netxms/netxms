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
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AccessPoint;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.objects.views.AccessPointsView;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * Label provider for access point list
 */
public class AccessPointListLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
   private NXCSession session = Registry.getSession();

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
      AccessPoint ap = (AccessPoint)element;
		switch(columnIndex)
		{
         case AccessPointsView.COLUMN_CONTROLLER:
            return (ap.getControllerId() != 0) ? session.getObjectNameWithAlias(ap.getControllerId()) : null;
         case AccessPointsView.COLUMN_ID:
            return Long.toString(ap.getObjectId());
         case AccessPointsView.COLUMN_IP_ADDRESS:
            return ap.getIpAddress().isValidUnicastAddress() ? ap.getIpAddress().getHostAddress() : null;
         case AccessPointsView.COLUMN_MAC_ADDRESS:
            return !ap.getMacAddress().isNull() ? ap.getMacAddress().toString() : null;
         case AccessPointsView.COLUMN_MODEL:
            return ap.getModel();
         case AccessPointsView.COLUMN_NAME:
            return ap.getNameWithAlias();
         case AccessPointsView.COLUMN_SERIAL_NUMBER:
            return ap.getSerialNumber();
         case AccessPointsView.COLUMN_STATE:
            return ap.getState().toString();
         case AccessPointsView.COLUMN_STATUS:
            return StatusDisplayInfo.getStatusText(ap.getStatus());
         case AccessPointsView.COLUMN_VENDOR:
            return ap.getVendor();
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
    */
	@Override
	public Color getForeground(Object element, int columnIndex)
	{
      AccessPoint ap = (AccessPoint)element;
		switch(columnIndex)
		{
         case AccessPointsView.COLUMN_STATE:
            switch(ap.getState())
            {
               case DOWN:
                  return StatusDisplayInfo.getStatusColor(ObjectStatus.CRITICAL);
               case UNKNOWN:
                  return StatusDisplayInfo.getStatusColor(ObjectStatus.UNKNOWN);
               case UNPROVISIONED:
                  return StatusDisplayInfo.getStatusColor(ObjectStatus.MAJOR);
               case UP:
                  return StatusDisplayInfo.getStatusColor(ObjectStatus.NORMAL);
            }
            return null;
         case AccessPointsView.COLUMN_STATUS:
            return StatusDisplayInfo.getStatusColor(ap.getStatus());
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
}
