/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.xnap.commons.i18n.I18n;

/**
 * "Appearance" preference page
 */
public class LanguagePage extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(LanguagePage.class);

   /**
    * Available languages
    */
   private static final Language[] languages = {
      new Language("de", "Deutsch - German"),
      new Language("en", "English"),
      new Language("es", "Español - Spanish"),
      new Language("fr", "Français - French"),
      new Language("pt_BR", "Português - Portuguese (Brazil)"),
      new Language("ru", "Русский - Russian")
   };

   private PreferenceStore settings = PreferenceStore.getInstance();
   private LabeledCombo languageSelector;

   public LanguagePage()
   {
      super(LocalizationHelper.getI18n(LanguagePage.class).tr("Language"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      dialogArea.setLayout(layout);

      final String currentLanguage = settings.getAsString("nxmc.language", "en");
      languageSelector = new LabeledCombo(dialogArea, SWT.NONE);
      languageSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      languageSelector.setLabel(i18n.tr("Selected language"));
      for(int i = 0; i < languages.length; i++)
      {
         languageSelector.add(languages[i].name);
         if (languages[i].code.equals(currentLanguage))
            languageSelector.select(i);
      }
      
      final Composite messageArea = createMessageArea(dialogArea);
      messageArea.setVisible(false);

      languageSelector.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            int index = languageSelector.getSelectionIndex();
            if (index != -1)
            {
               String lang = languages[index].code;
               messageArea.setVisible(!lang.equals(currentLanguage));
            }
         }
      });

      return dialogArea;
   }

   /**
    * Create area with warning message
    *
    * @param parent parent composite
    */
   private Composite createMessageArea(Composite parent)
   {
      Composite messageArea = new Composite(parent, SWT.BORDER);
      messageArea.setBackground(ThemeEngine.getBackgroundColor("MessageBar"));
      GridLayout layout = new GridLayout(2, false);
      messageArea.setLayout(layout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
      gd.grabExcessHorizontalSpace = true;
      messageArea.setLayoutData(gd);

      Label imageLabel = new Label(messageArea, SWT.NONE);
      imageLabel.setBackground(messageArea.getBackground());
      imageLabel.setImage(ResourceManager.getImage("icons/warning.png"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.verticalAlignment = SWT.FILL;
      imageLabel.setLayoutData(gd);
      imageLabel.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            imageLabel.getImage().dispose();
         }
      });

      Label messageLabel = new Label(messageArea, SWT.WRAP);
      messageLabel.setBackground(messageArea.getBackground());
      messageLabel.setForeground(ThemeEngine.getForegroundColor("MessageBar"));
      messageLabel
            .setText(Registry.IS_WEB_CLIENT ? i18n.tr("You have to relogin for language change to take effect") : i18n.tr("You have to restart client for language change to take effect"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.CENTER;
      messageLabel.setLayoutData(gd);
      return messageArea;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      int index = languageSelector.getSelectionIndex();
      if (index == -1)
         return false;

      settings.set("nxmc.language", languages[index].code);
      return true;
   }

   /**
    * Language definition
    */
   private static class Language
   {
      String code;
      String name;

      public Language(String code, String name)
      {
         this.code = code;
         this.name = name;
      }
   }
}
