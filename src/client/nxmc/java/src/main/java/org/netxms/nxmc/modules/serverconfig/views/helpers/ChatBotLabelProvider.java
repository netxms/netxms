/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
import org.netxms.client.ChatBot;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.views.ChatBots;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for chat bot elements
 */
public class ChatBotLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(ChatBotLabelProvider.class);
   private Image imageInactive;
   private Image imageActive;

   /**
    * Create label provider for chat bots
    */
   public ChatBotLabelProvider()
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
         return ((ChatBot)element).isDriverActive() ? imageActive : imageInactive;
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      ChatBot bot = (ChatBot)element;
      switch(columnIndex)
      {
         case ChatBots.COLUMN_NAME:
            return bot.getName();
         case ChatBots.COLUMN_DESCRIPTION:
            return bot.getDescription();
         case ChatBots.COLUMN_DRIVER:
            return bot.getDriverName();
         case ChatBots.COLUMN_HEALTH:
            return bot.isHealthy() ? i18n.tr("OK") : i18n.tr("Failed");
         case ChatBots.COLUMN_SESSIONS:
            return Integer.toString(bot.getActiveSessionCount());
         case ChatBots.COLUMN_LAST_MESSAGE:
            return ((bot.getLastInboundMessageTime() != null) && (bot.getLastInboundMessageTime().getTime() > 0))
                  ? DateFormatFactory.getDateTimeFormat().format(bot.getLastInboundMessageTime()) : "";
         case ChatBots.COLUMN_ERROR_MESSAGE:
            return bot.getErrorMessage();
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
