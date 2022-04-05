/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.businessservices.BusinessServiceCheck;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.businessservice.views.BusinessServiceChecksView;

/**
 * Comparator for 802.1x port state table
 */
public class BusinessServiceComparator extends ViewerComparator
{
   private BusinessServiceCheckLabelProvider labelProvider;
   
   public BusinessServiceComparator(BusinessServiceCheckLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
	   BusinessServiceCheck c1 = (BusinessServiceCheck)e1;
	   BusinessServiceCheck c2 = (BusinessServiceCheck)e2;
		
		int result = 0;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
         case BusinessServiceChecksView.COLUMN_ID:    
            result = Long.signum(c1.getId() - c2.getId());
            break;
         case BusinessServiceChecksView.COLUMN_DESCRIPTION:    
            result = c1.getDescription().compareTo(c2.getDescription());
            break;
         case BusinessServiceChecksView.COLUMN_TYPE:  
            result = c1.getCheckType().compareTo(c2.getCheckType());
            break;
         case BusinessServiceChecksView.COLUMN_OBJECT: 
            result = labelProvider.getObjectName(c1).compareTo(labelProvider.getObjectName(c2));  
            break; 
         case BusinessServiceChecksView.COLUMN_DCI:  
            result = labelProvider.getDciName(c1).compareTo(labelProvider.getDciName(c2));  
            break; 
         case BusinessServiceChecksView.COLUMN_STATUS:    
            result = c1.getState().compareTo(c2.getState());
            break;
         case BusinessServiceChecksView.COLUMN_FAIL_REASON:    
            result = c1.getFailureReason().compareTo(c2.getFailureReason());
            break;
         case BusinessServiceChecksView.COLUMN_ORIGIN:
            result = labelProvider.getOriginName(c1).compareTo(labelProvider.getOriginName(c2));  
            break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
