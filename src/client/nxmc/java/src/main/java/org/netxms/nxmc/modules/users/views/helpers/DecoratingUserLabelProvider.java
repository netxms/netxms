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
package org.netxms.nxmc.modules.users.views.helpers;

import org.eclipse.jface.viewers.DecoratingLabelProvider;
import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.views.UserManagementView;
import org.netxms.nxmc.resources.ThemeEngine;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for user manager
 */
public class DecoratingUserLabelProvider extends DecoratingLabelProvider implements ITableLabelProvider, ITableColorProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(DecoratingUserLabelProvider.class);

   public static final String[] AUTH_METHOD = { 
         LocalizationHelper.getI18n(DecoratingUserLabelProvider.class).tr("Password"),
         LocalizationHelper.getI18n(DecoratingUserLabelProvider.class).tr("RADIUS"), 
         LocalizationHelper.getI18n(DecoratingUserLabelProvider.class).tr("Certificate"), 
         LocalizationHelper.getI18n(DecoratingUserLabelProvider.class).tr("Certificate or password"), 
         LocalizationHelper.getI18n(DecoratingUserLabelProvider.class).tr("Certificate or RADIUS"),
         LocalizationHelper.getI18n(DecoratingUserLabelProvider.class).tr("LDAP")
   };

   private final Color disabledElementColor = ThemeEngine.getForegroundColor("List.DisabledItem");

   /**
    * Constructor
    */
   public DecoratingUserLabelProvider()
   {
      super(new BaseUserLabelProvider(), new UserLabelDecorator());
   }

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
		switch(columnIndex)
		{
         case UserManagementView.COLUMN_AUTH_METHOD:
            if (!(element instanceof User))
               return ""; //$NON-NLS-1$
            try
            {
               return AUTH_METHOD[((User)element).getAuthMethod().getValue()];
            }
            catch(ArrayIndexOutOfBoundsException e)
            {
               return i18n.tr("Unknown");
            }
         case UserManagementView.COLUMN_DESCRIPTION:
            return ((AbstractUserObject)element).getDescription();
         case UserManagementView.COLUMN_FULLNAME:
            return (element instanceof User) ? ((User) element).getFullName() : null;
         case UserManagementView.COLUMN_GUID:
            return ((AbstractUserObject)element).getGuid().toString();
         case UserManagementView.COLUMN_LDAP_DN:
            return ((AbstractUserObject)element).getLdapDn();
			case UserManagementView.COLUMN_NAME:
				return ((AbstractUserObject)element).getName();
         case UserManagementView.COLUMN_SOURCE:
            return ((((AbstractUserObject)element).getFlags() & AbstractUserObject.LDAP_USER) != 0) ? i18n.tr("LDAP") : i18n.tr("Local");
			case UserManagementView.COLUMN_TYPE:
            return (element instanceof User) ? i18n.tr("User") : i18n.tr("Group");
			case UserManagementView.COLUMN_LAST_LOGIN:
            return (element instanceof User)
                  ? (((User)element).getLastLogin().getTime() == 0 ? i18n.tr("Never")
                        : DateFormatFactory.getDateTimeFormat().format(((User)element).getLastLogin()))
                  : "";
			case UserManagementView.COLUMN_CREATED:
            return ((AbstractUserObject)element).getCreationTime().getTime() == 0 ? ""
                  : DateFormatFactory.getDateTimeFormat().format(((AbstractUserObject)element).getCreationTime());
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
    */
   @Override
   public Color getForeground(Object element, int columnIndex)
   {
      if (((AbstractUserObject)element).isDisabled())
         return disabledElementColor;
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
    */
   @Override
   public Color getBackground(Object element, int columnIndex)
   {
      return null;
   }
}
