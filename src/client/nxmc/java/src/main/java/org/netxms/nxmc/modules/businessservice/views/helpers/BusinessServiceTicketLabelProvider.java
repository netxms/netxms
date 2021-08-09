/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.businessservice.views.helpers;

import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.businessservices.BusinessServiceTicket;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.modules.businessservice.views.BusinessServiceAvailabilityView;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * Label provider for interface list
 */
public class BusinessServiceTicketLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{	
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
	   BusinessServiceTicket ticket = (BusinessServiceTicket)element;
		switch(columnIndex)
		{
         case BusinessServiceAvailabilityView.COLUMN_ID:				
				return Long.toString(ticket.getId());
         case BusinessServiceAvailabilityView.COLUMN_SERVICE_ID:    
            return Long.toString(ticket.getServiceId());
         case BusinessServiceAvailabilityView.COLUMN_CHECK_ID:  
            return Long.toString(ticket.getCheckId());
         case BusinessServiceAvailabilityView.COLUMN_CHECK_DESCRIPTION:    
            return ticket.getCheckDescription();  
         case BusinessServiceAvailabilityView.COLUMN_CREATION_TIME:    
            return DateFormatFactory.getDateTimeFormat().format(ticket.getCreationTime());
         case BusinessServiceAvailabilityView.COLUMN_TERMINATION_TIME:    
            return ticket.getCloseTime().getTime() == 0 ? "" : DateFormatFactory.getDateTimeFormat().format(ticket.getCloseTime());
         case BusinessServiceAvailabilityView.COLUMN_REASON:    
            return ticket.getReason();
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
    */
	@Override
	public Color getForeground(Object element, int columnIndex)
	{
	   return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
    */
	@Override
	public Color getBackground(Object element, int columnIndex)
	{
      BusinessServiceTicket check = (BusinessServiceTicket)element;
		return check.getCloseTime().getTime() == 0 ? StatusDisplayInfo.getStatusColor(ObjectStatus.MAJOR) : StatusDisplayInfo.getStatusColor(ObjectStatus.NORMAL);  
	}
}
