/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.base.preferencepages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.windows.TrayIconManager;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Appearance" preference page
 */
public class AppearancePage extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(AppearancePage.class);

   private Button checkVerticalLayout;
   private Button checkShowServerClock;
   private Button checkShowWelcomePage;
   private Button checkShowTrayIcon;

   public AppearancePage()
   {
      super(LocalizationHelper.getI18n(AppearancePage.class).tr("Appearance"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      PreferenceStore settings = PreferenceStore.getInstance();

      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      dialogArea.setLayout(layout);

      checkVerticalLayout = new Button(dialogArea, SWT.CHECK);
      checkVerticalLayout.setText(i18n.tr("&Vertical layout of perspective switcher"));
      checkVerticalLayout.setSelection(settings.getAsBoolean("Appearance.VerticalLayout", true));

      checkShowServerClock = new Button(dialogArea, SWT.CHECK);
      checkShowServerClock.setText(i18n.tr("Show server &clock"));
      checkShowServerClock.setSelection(settings.getAsBoolean("Appearance.ShowServerClock", false));

      checkShowWelcomePage = new Button(dialogArea, SWT.CHECK);
      checkShowWelcomePage.setText(i18n.tr("Show post-upgrade &welcome page on startup"));
      checkShowWelcomePage.setSelection(!settings.getAsBoolean("WelcomePage.Disabled", !Registry.getSession().getClientConfigurationHintAsBoolean("EnableWelcomePage", true)));

      if (!Registry.IS_WEB_CLIENT)
      {
         checkShowTrayIcon = new Button(dialogArea, SWT.CHECK);
         checkShowTrayIcon.setText(i18n.tr("Show tray &icon (required for notification pop-ups)"));
         checkShowTrayIcon.setSelection(settings.getAsBoolean("Appearance.ShowTrayIcon", true));
      }

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set("Appearance.VerticalLayout", checkVerticalLayout.getSelection());
      settings.set("Appearance.ShowServerClock", checkShowServerClock.getSelection());
      settings.set("WelcomePage.Disabled", !checkShowWelcomePage.getSelection());
      if (!Registry.IS_WEB_CLIENT)
      {
         settings.set("Appearance.ShowTrayIcon", checkShowTrayIcon.getSelection());
         if (checkShowTrayIcon.getSelection())
            TrayIconManager.showTrayIcon();
         else
            TrayIconManager.hideTrayIcon();
      }
      return true;
   }
}
