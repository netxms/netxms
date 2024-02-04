/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.users.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.dialogs.UserSelectionDialog;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Selector for user objects
 */
public class UserSelector extends AbstractSelector
{
   private final I18n i18n = LocalizationHelper.getI18n(UserSelector.class);

   private int userId = 0;
   private Image imageUser;
   private Image imageGroup;

   /**
    * Create user selector.
    *
    * @param parent parent composite
    * @param style widget style
    */
   public UserSelector(Composite parent, int style)
   {
      this(parent, style, new SelectorConfigurator());
   }

   /**
    * Create user selector.
    *
    * @param parent parent composite
    * @param style widget style
    * @param configurator selector configurator
    */
   public UserSelector(Composite parent, int style, SelectorConfigurator configurator)
   {
      super(parent, style, configurator
            .setSelectionButtonToolTip(LocalizationHelper.getI18n(UserSelector.class).tr("Select user")));

      setText(i18n.tr("<none>"));
      imageUser = ResourceManager.getImage("icons/user.png");
      imageGroup = ResourceManager.getImage("icons/group.png");
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            imageUser.dispose();
            imageGroup.dispose();
         }
      });
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
    */
   @Override
   protected void selectionButtonHandler()
   {
      UserSelectionDialog dlg = new UserSelectionDialog(getShell(), null);
      dlg.enableMultiSelection(false);
      if (dlg.open() == Window.OK)
      {
         long prevUserId = userId;
         AbstractUserObject[] users = dlg.getSelection();
         if (users.length > 0)
         {
            userId = users[0].getId();
            setText(users[0].getName());
            setImage(users[0] instanceof User ? imageUser : imageGroup);
         }
         else
         {
            userId = 0;
            setText(i18n.tr("<none>"));
            setImage(null);
         }
         if (prevUserId != userId)
            fireModifyListeners();
      }
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#clearButtonHandler()
    */
   @Override
   protected void clearButtonHandler()
   {
      if (userId == 0)
         return;

      userId = 0;
      setText(i18n.tr("<none>"));
      setImage(null);
      getTextControl().setToolTipText(null);
      fireModifyListeners();
   }

   /**
    * Get ID of selected user
    * 
    * @return Selected user's ID
    */
   public int getUserId()
   {
      return userId;
   }

   /**
    * Set user ID
    * 
    * @param userId new user ID
    */
   public void setUserId(int userId)
   {
      if (this.userId == userId)
         return; // nothing to change

      this.userId = userId;
      if (userId != 0)
      {
         AbstractUserObject object = Registry.getSession().findUserDBObjectById(userId, () -> {
            AbstractUserObject updatedObject = Registry.getSession().findUserDBObjectById(userId, null);
            if (updatedObject != null)
            {
               setText(updatedObject.getName());
               setImage(updatedObject instanceof User ? imageUser : imageGroup);
            }
         });
         if (object != null)
         {
            setText(object.getName());
            setImage(object instanceof User ? imageUser : imageGroup);
         }
         else
         {
            setText("[" + userId + "]");
            setImage(null);
         }
      }
      else
      {
         setText(i18n.tr("<none>"));
         setImage(null);
      }
      fireModifyListeners();
   }
}
