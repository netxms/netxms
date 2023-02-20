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
package org.netxms.nxmc.base.menus;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.netxms.nxmc.BrandingManager;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.ExternalWebBrowser;
import org.xnap.commons.i18n.I18n;

/**
 * Menu manager for help menu
 */
public class HelpMenuManager extends MenuManager
{
   private final I18n i18n = LocalizationHelper.getI18n(HelpMenuManager.class);

   private Action actionShowAdminGuide;
   private Action actionShowSupportOptions;

   /**
    * Create new instance of user menu manager
    */
   public HelpMenuManager()
   {
      super();
      createActions();
      setRemoveAllWhenShown(true);
      addMenuListener(new IMenuListener() {
         @Override
         public void menuAboutToShow(IMenuManager manager)
         {
            add(actionShowAdminGuide);
            add(actionShowSupportOptions);
         }
      });
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionShowAdminGuide = new Action(i18n.tr("Open &administrator guide")) {
         @Override
         public void run()
         {
            ExternalWebBrowser.open(BrandingManager.getAdministratorGuideURL());
         }
      };

      actionShowSupportOptions = new Action(i18n.tr("&Show support options")) {
         @Override
         public void run()
         {
            ExternalWebBrowser.open("https://go.netxms.com/commercial-support");
         }
      };
   }
}
