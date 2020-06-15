/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.agentmanager.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.UserAgentNotification;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.agentmanager.views.UserAgentNotificationView;
import org.netxms.ui.eclipse.console.UserRefreshRunnable;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Label provider for user agent notification
 */
public class UserAgentNotificationLabelProvider extends LabelProvider implements ITableLabelProvider
{   
   private NXCSession session;
   private SortableTableViewer viewer;
   
   
   /**
    * Constructor
    */
   public UserAgentNotificationLabelProvider(SortableTableViewer viewer)
   {
      this.viewer = viewer;
      session = ConsoleSharedData.getSession();      
   }
   
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
	   UserAgentNotification uaMessage = (UserAgentNotification)element;
		switch(columnIndex)
		{
			case UserAgentNotificationView.COL_ID:
				return Long.toString(uaMessage.getId());
			case UserAgentNotificationView.COL_OBJECTS:
				return uaMessage.getObjectNames();
			case UserAgentNotificationView.COL_MESSAGE:
				return uaMessage.getMessage();
			case UserAgentNotificationView.COL_IS_RECALLED:
				return uaMessage.isRecalled() ? "Yes" : "No";
         case UserAgentNotificationView.COL_IS_STARTUP:
            return uaMessage.isStartupNotification() ? "Yes" : "No";
			case UserAgentNotificationView.COL_START_TIME:
				return uaMessage.getStartTime().getTime() == 0 ? "" : RegionalSettings.getDateTimeFormat().format(uaMessage.getStartTime());
			case UserAgentNotificationView.COL_END_TIME:
				return uaMessage.getEndTime().getTime() == 0 ? "" : RegionalSettings.getDateTimeFormat().format(uaMessage.getEndTime());
         case UserAgentNotificationView.COL_CREATION_TIME:
            return uaMessage.getCreationTime().getTime() == 0 ? "" : RegionalSettings.getDateTimeFormat().format(uaMessage.getCreationTime());
         case UserAgentNotificationView.COL_CREATED_BY:
            return getUserName(uaMessage); //$NON-NLS-1$ //$NON-NLS-2$   
		}
		return null;
	}

   public String getUserName(UserAgentNotification uam)
   {
      AbstractUserObject user = session.findUserDBObjectById(uam.getCreatedBy(), new UserRefreshRunnable(viewer, uam)); 
      return (user != null) ? user.getName() : ("[" + Long.toString(uam.getCreatedBy()) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
   }
}
