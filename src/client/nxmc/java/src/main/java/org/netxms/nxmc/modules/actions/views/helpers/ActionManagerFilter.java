/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.ServerAction;
import org.netxms.nxmc.base.helpers.TokenizedFilter;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Filter for action manager
 */
public final class ActionManagerFilter extends ViewerFilter implements AbstractViewerFilter
{
   private TokenizedFilter filter = new TokenizedFilter(null);
   private ActionLabelProvider labelProvider;
   
   public ActionManagerFilter(ActionLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if (filter.isEmpty())
         return true;

      final ServerAction action = (ServerAction)element;
      return filter.matches(action.getName(), labelProvider.actionType[action.getType().getValue()], action.getRecipientAddress(),
            action.getEmailSubject(), action.getData(), action.getChannelName());
   }

   public void setFilterString(String text)
   {
      filter = new TokenizedFilter(text);
   }

   /**
    * @return the filterString
    */
   public String getFilterString()
   {
      return filter.getFilterString();
   }
}