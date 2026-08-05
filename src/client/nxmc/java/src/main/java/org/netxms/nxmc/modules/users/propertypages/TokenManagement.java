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
package org.netxms.nxmc.modules.users.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.users.User;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.widgets.AuthenticationTokenList;

/**
 * "Authentication Tokens" property page for user object
 */
public class TokenManagement extends PropertyPage
{
   private User user;

   /**
    * Constructor
    *
    * @param user user object
    * @param messageArea message area holder
    */
   public TokenManagement(User user, MessageAreaHolder messageArea)
   {
      super(LocalizationHelper.getI18n(TokenManagement.class).tr("Authentication Tokens"), messageArea);
      this.user = user;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      return new AuthenticationTokenList(parent, SWT.NONE, user.getId(), getMessageArea(false));
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      // All changes are applied immediately
      return true;
   }
}
