/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.agentmanagement.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.UserAgentNotification;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.agentmanagement.views.UserAgentNotificationView;

/**
 * User support application notification comparator
 */
public class UserAgentNotificationComparator extends ViewerComparator
{
   private UserAgentNotificationLabelProvider provider;
   
   /**
    * Constructor
    * 
    * @param provider label provider
    */
   public UserAgentNotificationComparator(UserAgentNotificationLabelProvider provider)
   {
      this.provider = provider;      
   }
   
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
	   UserAgentNotification uam1 = (UserAgentNotification)e1;
	   UserAgentNotification uam2 = (UserAgentNotification)e2;
		int result;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case UserAgentNotificationView.COL_ID:
				result = Long.signum(uam1.getId() - uam2.getId());
				break;
			case UserAgentNotificationView.COL_OBJECTS:
				result = uam1.getObjectNames().compareToIgnoreCase(uam2.getObjectNames());
				break;
			case UserAgentNotificationView.COL_MESSAGE:
				result = uam1.getMessage().compareTo(uam2.getMessage());
				break;
			case UserAgentNotificationView.COL_IS_RECALLED:
				result = Boolean.compare(uam1.isRecalled(), uam2.isRecalled());
				break;
         case UserAgentNotificationView.COL_IS_STARTUP:
            result = Boolean.compare(uam1.isStartupNotification(), uam2.isStartupNotification());
            break;
			case UserAgentNotificationView.COL_START_TIME:
				result = uam1.getStartTime().compareTo(uam2.getStartTime());
				break;
			case UserAgentNotificationView.COL_END_TIME:
            result = uam1.getEndTime().compareTo(uam2.getEndTime());
				break;
         case UserAgentNotificationView.COL_CREATION_TIME:
            result = uam1.getCreationTime().compareTo(uam2.getCreationTime());
            break;
         case UserAgentNotificationView.COL_CREATED_BY:
            result = provider.getUserName(uam1).compareTo(provider.getUserName(uam2)); //$NON-NLS-1$ //$NON-NLS-2$            
            break;
			default:
				result = 0;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
