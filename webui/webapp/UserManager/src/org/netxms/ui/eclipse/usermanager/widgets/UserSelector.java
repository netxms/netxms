/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.ui.eclipse.usermanager.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.usermanager.dialogs.UserSelectionDialog;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * Selector for user objects
 */
public class UserSelector extends AbstractSelector
{
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
      this(parent, style, 0);
   }

   /**
    * Create user selector.
    *
    * @param parent parent composite
    * @param style widget style
    * @param options selector options
    */
   public UserSelector(Composite parent, int style, int options)
   {
      super(parent, style, options);
      setText("<none>");
      imageUser = Activator.getImageDescriptor("icons/user.png").createImage();
      imageGroup = Activator.getImageDescriptor("icons/group.png").createImage();
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
            setText("<none>");
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
      setText("<none>");
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
         AbstractUserObject object = ConsoleSharedData.getSession().findUserDBObjectById(userId, () -> {
            AbstractUserObject updatedObject = ConsoleSharedData.getSession().findUserDBObjectById(userId, null);
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
         setText("<none>");
         setImage(null);
      }
      fireModifyListeners();
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#getSelectionButtonToolTip()
    */
   @Override
   protected String getSelectionButtonToolTip()
   {
      return "Select user";
   }
}
