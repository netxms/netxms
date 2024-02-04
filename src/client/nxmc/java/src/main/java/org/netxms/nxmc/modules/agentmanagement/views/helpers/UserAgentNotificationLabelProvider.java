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
package org.netxms.nxmc.modules.agentmanagement.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.UserAgentNotification;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.views.UserAgentNotificationView;
import org.netxms.nxmc.tools.ViewerElementUpdater;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for user support application notification
 */
public class UserAgentNotificationLabelProvider extends LabelProvider implements ITableLabelProvider
{   
   private final I18n i18n = LocalizationHelper.getI18n(UserAgentNotificationLabelProvider.class);

   private NXCSession session;
   private SortableTableViewer viewer;

   /**
    * Constructor
    */
   public UserAgentNotificationLabelProvider(SortableTableViewer viewer)
   {
      this.viewer = viewer;
      session = Registry.getSession();      
   }
   
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
				return uaMessage.isRecalled() ? i18n.tr("Yes") : i18n.tr("No");
         case UserAgentNotificationView.COL_IS_STARTUP:
            return uaMessage.isStartupNotification() ? i18n.tr("Yes") : i18n.tr("No");
			case UserAgentNotificationView.COL_START_TIME:
				return uaMessage.getStartTime().getTime() == 0 ? "" : DateFormatFactory.getDateTimeFormat().format(uaMessage.getStartTime());
			case UserAgentNotificationView.COL_END_TIME:
				return uaMessage.getEndTime().getTime() == 0 ? "" : DateFormatFactory.getDateTimeFormat().format(uaMessage.getEndTime());
         case UserAgentNotificationView.COL_CREATION_TIME:
            return uaMessage.getCreationTime().getTime() == 0 ? "" : DateFormatFactory.getDateTimeFormat().format(uaMessage.getCreationTime());
         case UserAgentNotificationView.COL_CREATED_BY:
            return getUserName(uaMessage); //$NON-NLS-1$ //$NON-NLS-2$   
		}
		return null;
	}

   /**
    * Get user name from notification object.
    *
    * @param uam notification object
    * @return user name
    */
   public String getUserName(UserAgentNotification n)
   {
      AbstractUserObject user = session.findUserDBObjectById(n.getCreatedBy(), new ViewerElementUpdater(viewer, n));
      return (user != null) ? user.getName() : ("[" + Long.toString(n.getCreatedBy()) + "]");
   }
}
