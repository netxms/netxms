/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.usermanager.views.helpers;

import org.eclipse.jface.viewers.DecoratingLabelProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.ui.eclipse.usermanager.Messages;
import org.netxms.ui.eclipse.usermanager.views.UserManagementView;

/**
 * Label provider for user manager
 */
public class UserLabelProvider extends DecoratingLabelProvider implements ITableLabelProvider
{
   /**
    * Constructor
    */
   public UserLabelProvider()
   {
      super(new WorkbenchLabelProvider(), PlatformUI.getWorkbench().getDecoratorManager().getLabelDecorator());
   }
   
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return (columnIndex == 0) ? getImage(element) : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case UserManagementView.COLUMN_NAME:
				return ((AbstractUserObject)element).getName();
			case UserManagementView.COLUMN_TYPE:
				return (element instanceof User) ? Messages.get().UserLabelProvider_User : Messages.get().UserLabelProvider_Group;
			case UserManagementView.COLUMN_FULLNAME:
				return (element instanceof User) ? ((User) element).getFullName() : null;
			case UserManagementView.COLUMN_DESCRIPTION:
				return ((AbstractUserObject)element).getDescription();
			case UserManagementView.COLUMN_GUID:
				return ((AbstractUserObject)element).getGuid().toString();
		}
		return null;
	}
}
