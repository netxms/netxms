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
package org.netxms.nxmc.modules.actions.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.ServerAction;
import org.netxms.client.constants.ServerActionType;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.actions.views.ActionManager;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for action list 
 */
public class ActionLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private static final I18n i18n = LocalizationHelper.getI18n(ActionLabelProvider.class);

	public static final String[] ACTION_TYPE = { 
   	   i18n.tr("Local Command"), 
   	   i18n.tr("Agent Command"), 
   	   i18n.tr("SSH Command"), 
   	   i18n.tr("Notification"), 
   	   i18n.tr("Forward Event"), 
         i18n.tr("NXSL Script"), 
   	   i18n.tr("XMPP Message") 
	   };

   private Image[] images;

   /**
    * Create label provider.
    */
   public ActionLabelProvider()
   {
      images = new Image[7];
      images[ServerActionType.LOCAL_COMMAND.getValue()] = ResourceManager.getImage("icons/actions/exec_local.png");
      images[ServerActionType.AGENT_COMMAND.getValue()] = ResourceManager.getImage("icons/actions/exec_remote.png");
      images[ServerActionType.SSH_COMMAND.getValue()] = ResourceManager.getImage("icons/actions/exec_remote.png");
      images[ServerActionType.NOTIFICATION.getValue()] = ResourceManager.getImage("icons/actions/email.png");
      images[ServerActionType.FORWARD_EVENT.getValue()] = ResourceManager.getImage("icons/actions/fwd_event.png");
      images[ServerActionType.NXSL_SCRIPT.getValue()] = ResourceManager.getImage("icons/actions/exec_script.png");
      images[ServerActionType.XMPP_MESSAGE.getValue()] = ResourceManager.getImage("icons/actions/xmpp.png");
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
      if (columnIndex != 0)
         return null;

      try
      {
         return images[((ServerAction)element).getType().getValue()];
      }
      catch(IndexOutOfBoundsException e)
      {
         return null;
      }
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		ServerAction action = (ServerAction)element;
		switch(columnIndex)
		{
			case ActionManager.COLUMN_NAME:
				return action.getName();
			case ActionManager.COLUMN_TYPE:
				try
				{
               return ACTION_TYPE[action.getType().getValue()];
				}
				catch(IndexOutOfBoundsException e)
				{
               return i18n.tr("Unknown");
				}
			case ActionManager.COLUMN_RCPT:
				return action.getRecipientAddress();
			case ActionManager.COLUMN_SUBJ:
				return action.getEmailSubject();
			case ActionManager.COLUMN_DATA:
				return action.getData();
         case ActionManager.COLUMN_CHANNEL:
            return action.getChannelName();
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      for(Image i : images)
         if (i != null)
            i.dispose();

      super.dispose();
   }
}
