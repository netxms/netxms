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
package org.netxms.nxmc.base.preferencepages;

import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Appearance" preference page
 */
public class Appearance extends PreferencePage
{
   private static final I18n i18n = LocalizationHelper.getI18n(Appearance.class);

   private Button checkVerticalLayout;
   private Button checkShowServerClock;

   public Appearance()
   {
      super(i18n.tr("Appearance"));
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
      checkVerticalLayout.setText("&Vertical layout of perspective switcher");
      checkVerticalLayout.setSelection(settings.getAsBoolean("Appearance.VerticalLayout", true));

      checkShowServerClock = new Button(dialogArea, SWT.CHECK);
      checkShowServerClock.setText("Show server &clock");
      checkShowServerClock.setSelection(settings.getAsBoolean("Appearance.ShowServerClock", false));

      return dialogArea;
   }

   /**
    * Apply changes
    */
   private void doApply()
   {
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set("Appearance.VerticalLayout", checkVerticalLayout.getSelection());
      settings.set("Appearance.ShowServerClock", checkShowServerClock.getSelection());
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected void performApply()
   {
      doApply();
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      doApply();
      return true;
   }
}
