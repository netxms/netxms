/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.ui.eclipse.slm.objecttabs.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.businessservices.BusinessServiceTicket;
import org.netxms.ui.eclipse.slm.objecttabs.BusinessServiceAvailability;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for business service tickets
 */
public class BusinessServiceTicketComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
	   BusinessServiceTicket t1 = (BusinessServiceTicket)e1;
	   BusinessServiceTicket t2 = (BusinessServiceTicket)e2;
		
		int result = 0;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
         case BusinessServiceAvailability.COLUMN_ID:
            result = Long.signum(t1.getId() - t2.getId());  
            break;
         case BusinessServiceAvailability.COLUMN_SERVICE_ID:
            result = Long.signum(t1.getServiceId() - t2.getServiceId());  
            break; 
         case BusinessServiceAvailability.COLUMN_CHECK_ID:
            result = Long.signum(t1.getCheckId() - t2.getCheckId());  
            break;  
         case BusinessServiceAvailability.COLUMN_CHECK_DESCRIPTION:
            result = t1.getCheckDescription().compareTo(t2.getCheckDescription());  
            break;    
         case BusinessServiceAvailability.COLUMN_CREATION_TIME:
            result = t1.getCreationTime().compareTo(t2.getCreationTime());  
            break;             
         case BusinessServiceAvailability.COLUMN_TERMINATION_TIME:
            result = t1.getCloseTime().compareTo(t2.getCloseTime());  
            break;             
         case BusinessServiceAvailability.COLUMN_REASON:
            result = t1.getReason().compareTo(t2.getReason());  
            break;     
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
