/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.ai.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.ai.AiMessage;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Filter for AI message list
 */
public final class AiMessageListFilter extends ViewerFilter implements AbstractViewerFilter
{
   private String filterString;
   private AiMessageListLabelProvider labelProvider;

   /**
    * Create filter
    *
    * @param labelProvider label provider for getting display text
    */
   public AiMessageListFilter(AiMessageListLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || (filterString.isEmpty()))
         return true;

      final AiMessage message = (AiMessage)element;
      if (message.getTitle().toLowerCase().contains(filterString) ||
          message.getText().toLowerCase().contains(filterString) ||
          labelProvider.getTypeName(message).toLowerCase().contains(filterString) ||
          labelProvider.getStatusName(message).toLowerCase().contains(filterString))
      {
         return true;
      }
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.AbstractViewerFilter#setFilterString(java.lang.String)
    */
   @Override
   public void setFilterString(String text)
   {
      filterString = text.toLowerCase();
   }
}
