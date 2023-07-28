/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.base.menus;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.window.Window;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.LogoutAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.dialogs.ChangePasswordDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Menu manager for user menu
 */
public class UserMenuManager extends MenuManager
{
   private final I18n i18n = LocalizationHelper.getI18n(UserMenuManager.class);

   private Action actionChangePassword;
   private Action actionLogout;

   /**
    * Create new instance of user menu manager
    */
   public UserMenuManager()
   {
      super();
      createActions();
      setRemoveAllWhenShown(true);
      addMenuListener(new IMenuListener() {
         @Override
         public void menuAboutToShow(IMenuManager manager)
         {
            add(actionChangePassword);
            add(actionLogout);
         }
      });
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionChangePassword = new Action(i18n.tr("&Change password...")) {
         @Override
         public void run()
         {
            changePassword();
         }
      };

      actionLogout = new LogoutAction(i18n.tr("&Logout"));
   }

   /**
    * Change password for currently logged in user
    */
   private void changePassword()
   {
      final MainWindow window = Registry.getMainWindow();
      final ChangePasswordDialog dlg = new ChangePasswordDialog(window.getShell(), true);
      if (dlg.open() == Window.OK)
      {
         final NXCSession session = Registry.getSession();
         Job job = new Job(i18n.tr("Change password"), null, window) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.setUserPassword(session.getUserId(), dlg.getPassword(), dlg.getOldPassword());
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     window.addMessage(MessageArea.SUCCESS, "Password changed successfully");
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot change password";
            }
         };
         job.start();
      }
   }
}
