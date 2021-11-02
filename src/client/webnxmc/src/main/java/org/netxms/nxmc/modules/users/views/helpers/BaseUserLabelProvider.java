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

import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.client.users.UserGroup;
import org.netxms.nxmc.resources.SharedIcons;

/**
 * Base user label provider
 */
public class BaseUserLabelProvider extends LabelProvider
{
   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
    */
   @Override
   public Image getImage(Object element)
   {
      return (element instanceof User) ? SharedIcons.IMG_USER : ((element instanceof UserGroup) ? SharedIcons.IMG_GROUP : null);
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
    */
   @Override
   public String getText(Object element)
   {
      if (element instanceof String)
         return (String)element;
      return ((AbstractUserObject)element).getLabel();
   }
}
