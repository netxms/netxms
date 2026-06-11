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
package org.netxms.nxmc.modules.objecttools.dialogs;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for adding or editing translations of an object tool's user-facing strings
 * for a single language.
 */
public class TranslationEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(TranslationEditDialog.class);

   /**
    * Supported languages — mirrors the list on the client's Language preference page so
    * admins can only define translations for languages users can actually pick.
    */
   private final Language[] languages = {
      new Language("de", i18n.tr("German")),
      new Language("en", i18n.tr("English")),
      new Language("es", i18n.tr("Spanish")),
      new Language("fr", i18n.tr("French")),
      new Language("pt_BR", i18n.tr("Portuguese (Brazil)")),
      new Language("ru", i18n.tr("Russian"))
   };

   private final boolean create;
   private final boolean readOnlyLanguage;
   private final Set<String> takenLanguages;
   private String language;
   private Map<String, String> values;

   /** Languages actually shown in the combo, in display order. */
   private List<Language> comboLanguages;

   private LabeledCombo languageSelector;
   private LabeledText nameField;
   private LabeledText descriptionField;
   private LabeledText confirmationField;
   private LabeledText commandNameField;
   private LabeledText commandShortNameField;

   /**
    * @param parentShell parent shell
    * @param create true if creating a new translation entry, false if editing an existing one
    * @param language existing language code (may be empty when creating)
    * @param values existing field values for this language (may be null when creating)
    * @param takenLanguages languages already present, used to prevent duplicates
    */
   public TranslationEditDialog(Shell parentShell, boolean create, String language, Map<String, String> values, Set<String> takenLanguages)
   {
      super(parentShell);
      this.create = create;
      this.readOnlyLanguage = !create;
      this.language = (language != null) ? language : "";
      this.values = (values != null) ? new HashMap<String, String>(values) : new HashMap<String, String>();
      this.takenLanguages = takenLanguages;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(create ? i18n.tr("Add Translation") : i18n.tr("Edit Translation"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      comboLanguages = buildComboList();

      languageSelector = new LabeledCombo(dialogArea, SWT.NONE);
      languageSelector.setLabel(i18n.tr("Language"));
      languageSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      int selectedIndex = -1;
      for(int i = 0; i < comboLanguages.size(); i++)
      {
         Language lang = comboLanguages.get(i);
         languageSelector.add(lang.code + " - " + lang.name);
         if (lang.code.equals(language))
            selectedIndex = i;
      }
      if (selectedIndex == -1 && !comboLanguages.isEmpty())
         selectedIndex = 0;
      if (selectedIndex >= 0)
         languageSelector.select(selectedIndex);
      languageSelector.getComboControl().setEnabled(!readOnlyLanguage);

      nameField = new LabeledText(dialogArea, SWT.NONE);
      nameField.setLabel(i18n.tr("Name"));
      nameField.setText(safe(values.get(ObjectToolDetails.TRANSLATION_FIELD_NAME)));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      nameField.setLayoutData(gd);

      descriptionField = new LabeledText(dialogArea, SWT.NONE);
      descriptionField.setLabel(i18n.tr("Description"));
      descriptionField.setText(safe(values.get(ObjectToolDetails.TRANSLATION_FIELD_DESCRIPTION)));
      descriptionField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      confirmationField = new LabeledText(dialogArea, SWT.NONE);
      confirmationField.setLabel(i18n.tr("Confirmation message"));
      confirmationField.setText(safe(values.get(ObjectToolDetails.TRANSLATION_FIELD_CONFIRMATION_TEXT)));
      confirmationField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      commandNameField = new LabeledText(dialogArea, SWT.NONE);
      commandNameField.setLabel(i18n.tr("Command name"));
      commandNameField.setText(safe(values.get(ObjectToolDetails.TRANSLATION_FIELD_COMMAND_NAME)));
      commandNameField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      commandShortNameField = new LabeledText(dialogArea, SWT.NONE);
      commandShortNameField.setLabel(i18n.tr("Command short name"));
      commandShortNameField.setText(safe(values.get(ObjectToolDetails.TRANSLATION_FIELD_COMMAND_SHORT_NAME)));
      commandShortNameField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      return dialogArea;
   }

   /**
    * Build the language list shown in the combo: when adding, hide already-taken languages so
    * duplicates can't be created; when editing, show only the current language. If editing a
    * legacy code that isn't in the predefined list, fall back to a synthetic entry.
    */
   private List<Language> buildComboList()
   {
      List<Language> list = new ArrayList<Language>(languages.length);
      if (readOnlyLanguage)
      {
         for(Language l : languages)
         {
            if (l.code.equals(language))
            {
               list.add(l);
               return list;
            }
         }
         list.add(new Language(language, language));
         return list;
      }
      for(Language l : languages)
      {
         if (!takenLanguages.contains(l.code))
            list.add(l);
      }
      return list;
   }

   private static String safe(String s)
   {
      return (s != null) ? s : "";
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      int index = languageSelector.getSelectionIndex();
      if ((index < 0) || (index >= comboLanguages.size()))
      {
         MessageDialog.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please select a language."));
         return;
      }
      language = comboLanguages.get(index).code;

      values.clear();
      putIfNotEmpty(values, ObjectToolDetails.TRANSLATION_FIELD_NAME, nameField.getText());
      putIfNotEmpty(values, ObjectToolDetails.TRANSLATION_FIELD_DESCRIPTION, descriptionField.getText());
      putIfNotEmpty(values, ObjectToolDetails.TRANSLATION_FIELD_CONFIRMATION_TEXT, confirmationField.getText());
      putIfNotEmpty(values, ObjectToolDetails.TRANSLATION_FIELD_COMMAND_NAME, commandNameField.getText());
      putIfNotEmpty(values, ObjectToolDetails.TRANSLATION_FIELD_COMMAND_SHORT_NAME, commandShortNameField.getText());

      super.okPressed();
   }

   private static void putIfNotEmpty(Map<String, String> map, String key, String value)
   {
      if ((value != null) && !value.isEmpty())
         map.put(key, value);
   }

   /**
    * Get language code
    *
    * @return language code
    */
   public String getLanguage()
   {
      return language;
   }

   /**
    * Get translated texts
    *
    * @return translated texts
    */
   public Map<String, String> getValues()
   {
      return values;
   }

   /**
    * Language definition: code (BCP-47) plus human-readable, translatable display name.
    */
   private static class Language
   {
      final String code;
      final String name;

      Language(String code, String name)
      {
         this.code = code;
         this.name = name;
      }
   }
}
