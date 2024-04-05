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
package org.netxms.nxmc.modules.objects.propertypages.helpers;

import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.ThemeEngine;

/**
 * Label provider for NetXMS objects access lists
 */
public class AccessListLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
   private static final AccessBit[] ACCESS_BITS = {
      new AccessBit(UserAccessRights.OBJECT_ACCESS_READ, 'R'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_READ_AGENT, 'a'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_READ_SNMP, 's'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_MODIFY, 'M'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_EDIT_COMMENTS, 'M'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_DELEGATED_READ, 'r'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_CREATE, 'C'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_EDIT_RESP_USERS, 'u'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_DELETE, 'D'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_CONTROL, 'O'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_SEND_EVENTS, 'E'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_READ_ALARMS, 'V'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_UPDATE_ALARMS, 'K'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_TERM_ALARMS, 'T'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_CREATE_ISSUE, 'I'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_PUSH_DATA, 'P'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_ACL, 'A'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_DOWNLOAD, 'N'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_UPLOAD, 'U'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_MANAGE_FILES, 'F'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_CONFIGURE_AGENT, 'c'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_SCREENSHOT, 'S'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_MAINTENANCE, 'M'),
      new AccessBit(UserAccessRights.OBJECT_ACCESS_EDIT_MNT_JOURNAL, 'J')
   };

   private final NXCSession session = Registry.getSession();
   private final Color inheritedElementColor = ThemeEngine.getForegroundColor("List.DisabledItem");

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
      if ((columnIndex == 0) && (element instanceof AccessListElement))
      {
         int userId = ((AccessListElement)element).getUserId();
         AbstractUserObject userObject = session.findUserDBObjectById(userId, null);
         if (userObject != null)
         {
            return userObject instanceof User ? SharedIcons.IMG_USER : SharedIcons.IMG_GROUP;
         }
      }
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
			case 0:
            int userId = ((AccessListElement)element).getUserId();
            AbstractUserObject userObject = session.findUserDBObjectById(userId, null);
            return (userObject != null) ? userObject.getName() : "[" + userId + "]";
			case 1:
				AccessListElement e = (AccessListElement)element;
            StringBuilder sb = new StringBuilder(32);
            for(int i = 0; i < ACCESS_BITS.length; i++)
               sb.append(((e.getAccessRights() & ACCESS_BITS[i].mask) != 0) ? ACCESS_BITS[i].symbol : '-');
				return sb.toString();
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
    */
   @Override
   public Color getForeground(Object element, int columnIndex)
   {
      return ((AccessListElement)element).isInherited() ? inheritedElementColor : null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
    */
   @Override
   public Color getBackground(Object element, int columnIndex)
   {
      return null;
   }

   /**
    * Symbolic representation of access bit
    */
   private static class AccessBit
   {
      int mask;
      char symbol;

      AccessBit(int mask, char symbol)
      {
         this.mask = mask;
         this.symbol = symbol;
      }
   }
}
