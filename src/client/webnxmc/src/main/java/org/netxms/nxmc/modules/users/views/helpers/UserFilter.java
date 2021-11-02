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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Filter for user list
 */
public final class UserFilter extends ViewerFilter
{
   private final I18n i18n = LocalizationHelper.getI18n(UserFilter.class);

   private String filterString;
   private DecoratingUserLabelProvider labelProvider;

   /**
    * Create filter for user list.
    *
    * @param labelProvider related label provider
    */
   public UserFilter(DecoratingUserLabelProvider labelProvider)
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
      
      final AbstractUserObject user = (AbstractUserObject)element;
      boolean found = false;
      if (element instanceof User)
      {
         String authMethod = "";
         try
         {
            authMethod = labelProvider.AUTH_METHOD[((User)user).getAuthMethod().getValue()];
         }
         catch(ArrayIndexOutOfBoundsException e)
         {
            authMethod = i18n.tr("Unknown");
         }

         if (authMethod.toLowerCase().contains(filterString) || ((User)user).getFullName().toLowerCase().contains(filterString))
            return true;
      }

      String objClass = (element instanceof User) ? i18n.tr("User") : i18n.tr("Group");
      String objType = ((user.getFlags() & AbstractUserObject.LDAP_USER) != 0) ? i18n.tr("LDAP") : i18n.tr("Local");
      if (objClass.toLowerCase().contains(filterString) || 
            objType.toLowerCase().contains(filterString) ||
            user.getName().toLowerCase().contains(filterString) || 
            user.getDescription().toLowerCase().contains(filterString) ||
            user.getLdapDn().toLowerCase().contains(filterString))
      {
         found = true;
      }

      return found;
   }

   /**
    * Set filter string.
    *
    * @param text new filter string
    */
   public void setFilterString(String text)
   {
      filterString = text.trim().toLowerCase();
   }

   /**
    * Get current filter string.
    *
    * @return current filter string
    */
   public String getFilterString()
   {
      return filterString;
   }
}
