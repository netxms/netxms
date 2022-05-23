/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.DecoratingLabelProvider;
import org.eclipse.swt.graphics.Color;
import org.netxms.client.ServerAction;
import org.netxms.nxmc.resources.ThemeEngine;

/**
 * Decorating label provider for server actions.
 */
public class DecoratingActionLabelProvider extends DecoratingLabelProvider
{
   private final Color disabledElementColor = ThemeEngine.getForegroundColor("List.DisabledItem");

   /**
    * @param provider
    * @param decorator
    */
   public DecoratingActionLabelProvider()
   {
      super(new BaseActionLabelProvider(), new ActionLabelDecorator());
   }

   /**
    * @see org.eclipse.jface.viewers.DecoratingLabelProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      if (((ServerAction)element).isDisabled())
         return disabledElementColor;
      return null;
   }
}
