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
package org.netxms.nxmc.modules.datacollection.propertypages.helpers;

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.GraphDefinition;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.client.users.UserGroup;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.resources.SharedIcons;

/**
 * Label provider for NetXMS objects access lists
 */
public class AccessListLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private static final Color HINT_FOREGROUND = new Color(Display.getDefault(), 192, 192, 192);

   private NXCSession session = Registry.getSession();

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
   public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex == 0)
		{
         if (element instanceof AccessListElement)
            return ((((AccessListElement)element).getUserId() & 0x40000000L) == 0) ? SharedIcons.IMG_USER : SharedIcons.IMG_GROUP;
         if (element instanceof User)
            return SharedIcons.IMG_USER; 
         if (element instanceof UserGroup)
            return SharedIcons.IMG_GROUP; 
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
			   if (element instanceof AccessListElement)
			      return getUserName((AccessListElement)element);
            if (element instanceof String)
               return (String)element;
            if (element instanceof User)
               return getUserName((User)element);
            if (element instanceof UserGroup)
               return getUserName((UserGroup)element);
            return "";
			case 1:
				AccessListElement e = (AccessListElement)element;
				StringBuilder sb = new StringBuilder(4);
            sb.append(((e.getAccessRights() & GraphDefinition.ACCESS_READ) != 0) ? 'R' : '-');
            sb.append(((e.getAccessRights() & GraphDefinition.ACCESS_WRITE) != 0) ? 'W' : '-');
				return sb.toString();
		}
		return null;
	}

   /**
    * Get user name from element
    *
    * @param element access list element
    * @return user name
    */
   private String getUserName(AccessListElement element)
   {
      int userId = element.getUserId();
      AbstractUserObject dbo = session.findUserDBObjectById(userId, null);
      return (dbo != null) ? dbo.getName() : ("{" + Long.toString(userId) + "}");
   }

   /**
    * Get user name from element
    *
    * @param element user element
    * @return user name
    */
   private String getUserName(User element)
   {
      StringBuilder sb = new StringBuilder();
      if (element.getFullName().isEmpty())
         sb.append(element.getName());
      else
         sb.append(String.format("%s <%s>", element.getName(), element.getFullName()));

      if (!element.getDescription().isEmpty())
         sb.append(String.format(" (%s)", element.getDescription()));

      return sb.toString();
   }

   /**
    * Get user name from element
    *
    * @param element group element
    * @return user name
    */
   private String getUserName(UserGroup element)
   {
      StringBuilder sb = new StringBuilder();
      sb.append(element.getName());
      
      if (!element.getDescription().isEmpty())
         sb.append(String.format(" (%s)", element.getDescription()));
      
      return sb.toString();
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      if (element instanceof String)
         return HINT_FOREGROUND;
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      return null;
   }
}
