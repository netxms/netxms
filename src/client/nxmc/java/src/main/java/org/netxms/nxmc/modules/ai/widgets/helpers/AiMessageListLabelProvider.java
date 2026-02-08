/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.ai.widgets.helpers;

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ITableFontProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.ai.AiMessage;
import org.netxms.client.constants.AiMessageStatus;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.widgets.AiMessageList;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for AI message list
 */
public class AiMessageListLabelProvider extends LabelProvider implements ITableLabelProvider, ITableFontProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(AiMessageListLabelProvider.class);

   private final String[] typeNames = { i18n.tr("Info"), i18n.tr("Alert"), i18n.tr("Approval") };
   private final String[] statusNames = { i18n.tr("Pending"), i18n.tr("Read"), i18n.tr("Approved"), i18n.tr("Rejected"), i18n.tr("Expired") };
   private final Image iconInformational = ResourceManager.getImage("icons/ai/ai-message-info.png");
   private final Image iconAlert = ResourceManager.getImage("icons/ai/ai-message-alert.png");
   private final Image iconApproval = ResourceManager.getImage("icons/ai/ai-message-request.png");

   private NXCSession session = Registry.getSession();
   private Font boldFont = null;

   /**
    * Default constructor
    */
   public AiMessageListLabelProvider()
   {
      boldFont = JFaceResources.getFontRegistry().getBold(JFaceResources.DEFAULT_FONT);
   }

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      iconInformational.dispose();
      iconAlert.dispose();
      iconApproval.dispose();
      super.dispose();
   }

   /**
    * Get type name for message
    *
    * @param message AI message
    * @return type name
    */
   public String getTypeName(AiMessage message)
   {
      int value = message.getMessageType().getValue();
      if (value >= 0 && value < typeNames.length)
         return typeNames[value];
      return i18n.tr("Unknown");
   }

   /**
    * Get status name for message
    *
    * @param message AI message
    * @return status name
    */
   public String getStatusName(AiMessage message)
   {
      int value = message.getStatus().getValue();
      if (value >= 0 && value < statusNames.length)
         return statusNames[value];
      return i18n.tr("Unknown");
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex > 0)
         return null;

      AiMessage message = (AiMessage)element;
      switch(message.getMessageType())
      {
         case ALERT:
            return iconAlert;
         case APPROVAL_REQUEST:
            return iconApproval;
         case INFORMATIONAL:
            return iconInformational;
         default:
            return null;
      }
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      AiMessage message = (AiMessage)element;
      switch(columnIndex)
      {
         case AiMessageList.COLUMN_ID:
            return Long.toString(message.getId());
         case AiMessageList.COLUMN_TYPE:
            return getTypeName(message);
         case AiMessageList.COLUMN_STATUS:
            return getStatusName(message);
         case AiMessageList.COLUMN_TITLE:
            return message.getTitle();
         case AiMessageList.COLUMN_RELATED_OBJECT:
            long objectId = message.getRelatedObjectId();
            if (objectId == 0)
               return "";
            AbstractObject object = session.findObjectById(objectId);
            return (object != null) ? object.getObjectName() : ("[" + objectId + "]");
         case AiMessageList.COLUMN_CREATED:
            return DateFormatFactory.getDateTimeFormat().format(message.getCreationTime());
         case AiMessageList.COLUMN_EXPIRES:
            return DateFormatFactory.getDateTimeFormat().format(message.getExpirationTime());
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableFontProvider#getFont(java.lang.Object, int)
    */
   @Override
   public Font getFont(Object element, int columnIndex)
   {
      AiMessage message = (AiMessage)element;
      if (message.getStatus() == AiMessageStatus.PENDING)
         return boldFont;
      return null;
   }
}
