/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
import org.eclipse.swt.graphics.Image;
import org.netxms.client.ServerAction;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.actions.views.ActionManager;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for action list 
 */
public class ActionLabelProvider extends DecoratingActionLabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(ActionLabelProvider.class);

   public final String[] actionType = 
	   { 
	      LocalizationHelper.getI18n(ActionLabelProvider.class).tr("Local Command"), 
	      LocalizationHelper.getI18n(ActionLabelProvider.class).tr("Agent Command"), 
	      LocalizationHelper.getI18n(ActionLabelProvider.class).tr("SSH Command"), 
	      LocalizationHelper.getI18n(ActionLabelProvider.class).tr("Notification"), 
	      LocalizationHelper.getI18n(ActionLabelProvider.class).tr("Forward Event"), 
   	   LocalizationHelper.getI18n(ActionLabelProvider.class).tr("NXSL Script")
	   };

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
      return (columnIndex == 0) ? getImage(element) : null;
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
               return actionType[action.getType().getValue()];
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
}
