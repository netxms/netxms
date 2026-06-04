/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
import org.netxms.client.EventForwarder;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.views.EventForwarders;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for event forwarder elements
 */
public class EventForwarderLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(EventForwarderLabelProvider.class);
   private final String[] sendStatusText = { i18n.tr("Unknown"), i18n.tr("Success"), i18n.tr("Failure") };
   private Image imageInactive;
   private Image imageActive;

   /**
    * Create label provider for event forwarders
    */
   public EventForwarderLabelProvider()
   {
      imageInactive = ResourceManager.getImageDescriptor("icons/inactive.png").createImage();
      imageActive = ResourceManager.getImageDescriptor("icons/active.png").createImage();
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex == 0)
         return ((EventForwarder)element).isDriverInitialized() ? imageActive : imageInactive;
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
         case EventForwarders.COLUMN_DESCRIPTION:
            return ((EventForwarder)element).getDescription();
         case EventForwarders.COLUMN_DRIVER:
            return ((EventForwarder)element).getDriverName();
         case EventForwarders.COLUMN_ERROR_MESSAGE:
            return ((EventForwarder)element).getErrorMessage();
         case EventForwarders.COLUMN_DROPPED_EVENTS:
            return Integer.toString(((EventForwarder)element).getDroppedCount());
         case EventForwarders.COLUMN_FAILED_EVENTS:
            return Integer.toString(((EventForwarder)element).getFailureCount());
         case EventForwarders.COLUMN_LAST_STATUS:
            return sendStatusText[((EventForwarder)element).getSendStatus()];
         case EventForwarders.COLUMN_NAME:
            return ((EventForwarder)element).getName();
         case EventForwarders.COLUMN_QUEUE_SIZE:
            return Integer.toString(((EventForwarder)element).getQueueSize());
         case EventForwarders.COLUMN_TOTAL_EVENTS:
            return Integer.toString(((EventForwarder)element).getMessageCount());
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
