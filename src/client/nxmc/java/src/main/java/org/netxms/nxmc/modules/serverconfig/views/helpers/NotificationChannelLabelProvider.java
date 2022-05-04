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
package org.netxms.nxmc.modules.serverconfig.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NotificationChannel;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.views.NotificationChannels;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for notification channel elements
 */
public class NotificationChannelLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(NotificationChannelLabelProvider.class);
   private final String[] sendStatusText = { i18n.tr("Unknown"), i18n.tr("Success"), i18n.tr("Failure") };
   private Image imageInactive;
   private Image imageActive;

   /**
    * Create label provider for notification channels
    */
   public NotificationChannelLabelProvider()
   {
      imageInactive = ResourceManager.getImageDescriptor("icons/inactive.gif").createImage();
      imageActive = ResourceManager.getImageDescriptor("icons/active.gif").createImage();
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
      if (columnIndex == 0)
         return ((NotificationChannel)element).isActive() ? imageActive : imageInactive;
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case NotificationChannels.COLUMN_DESCRIPTION:
				return ((NotificationChannel)element).getDescription();
         case NotificationChannels.COLUMN_DRIVER:
            return ((NotificationChannel)element).getDriverName();
         case NotificationChannels.COLUMN_ERROR_MESSAGE:
            return ((NotificationChannel)element).getErrorMessage();
         case NotificationChannels.COLUMN_FAILED_MESSAGES:
            return Integer.toString(((NotificationChannel)element).getFailureCount());
         case NotificationChannels.COLUMN_LAST_STATUS:
            return sendStatusText[((NotificationChannel)element).getSendStatus()];
         case NotificationChannels.COLUMN_NAME:
            return ((NotificationChannel)element).getName();
         case NotificationChannels.COLUMN_TOTAL_MESSAGES:
            return Integer.toString(((NotificationChannel)element).getMessageCount());
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      imageInactive.dispose();
      imageActive.dispose();
   }
}
