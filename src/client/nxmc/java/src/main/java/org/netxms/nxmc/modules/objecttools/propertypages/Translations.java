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
package org.netxms.nxmc.modules.objecttools.propertypages;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.TreeMap;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objecttools.dialogs.TranslationEditDialog;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Translations" property page for object tool — lets admins provide localized
 * name/description/confirmation/command_name/command_short_name per BCP-47 language.
 */
public class Translations extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(Translations.class);

   private final ObjectToolDetails objectTool;

   /** Working copy: language -&gt; (fieldTag -&gt; value). TreeMap so rows show alphabetically. */
   private TreeMap<String, Map<String, String>> translations;

   private TableViewer viewer;
   private Button buttonAdd;
   private Button buttonEdit;
   private Button buttonRemove;

   /**
    * Create translations page
    *
    * @param toolDetails object tool details
    */
   public Translations(ObjectToolDetails toolDetails)
   {
      super(LocalizationHelper.getI18n(Translations.class).tr("Translations"));
      noDefaultAndApplyButton();
      this.objectTool = toolDetails;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      translations = deepCopy(objectTool.getTranslations());

      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 1;
      dialogArea.setLayout(layout);

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT * 8;
      viewer.getTable().setLayoutData(gd);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new TranslationLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         @SuppressWarnings("unchecked")
         public int compare(Viewer v, Object e1, Object e2)
         {
            return ((Map.Entry<String, Map<String, String>>)e1).getKey()
                  .compareToIgnoreCase(((Map.Entry<String, Map<String, String>>)e2).getKey());
         }
      });
      setupTableColumns();
      viewer.addSelectionChangedListener((e) -> {
         IStructuredSelection selection = viewer.getStructuredSelection();
         buttonEdit.setEnabled(selection.size() == 1);
         buttonRemove.setEnabled(selection.size() > 0);
      });
      viewer.addDoubleClickListener((e) -> editTranslation());
      refreshViewer();

      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.verticalIndent = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;
      buttons.setLayoutData(gd);

      buttonAdd = new Button(buttons, SWT.PUSH);
      buttonAdd.setText(i18n.tr("&Add..."));
      buttonAdd.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addTranslation();
         }
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonAdd.setLayoutData(rd);

      buttonEdit = new Button(buttons, SWT.PUSH);
      buttonEdit.setText(i18n.tr("&Edit..."));
      buttonEdit.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editTranslation();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonEdit.setLayoutData(rd);
      buttonEdit.setEnabled(false);

      buttonRemove = new Button(buttons, SWT.PUSH);
      buttonRemove.setText(i18n.tr("&Delete"));
      buttonRemove.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            removeTranslation();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonRemove.setLayoutData(rd);
      buttonRemove.setEnabled(false);

      return dialogArea;
   }

   /**
    * Setup table columns
    */
   private void setupTableColumns()
   {
      addColumn(i18n.tr("Language"), 80);
      addColumn(i18n.tr("Name"), 180);
      addColumn(i18n.tr("Description"), 180);
      addColumn(i18n.tr("Confirmation"), 180);
      addColumn(i18n.tr("Command name"), 140);
      addColumn(i18n.tr("Command short name"), 120);
      viewer.getTable().setHeaderVisible(true);
      WidgetHelper.restoreColumnSettings(viewer.getTable(), "TranslationsPropertyPage");
   }

   /**
    * Add column
    *
    * @param label label
    * @param width default width
    */
   private void addColumn(String label, int width)
   {
      TableColumn column = new TableColumn(viewer.getTable(), SWT.LEFT);
      column.setText(label);
      column.setWidth(width);
   }

   /**
    * Refresh viewer
    */
   private void refreshViewer()
   {
      viewer.setInput(translations.entrySet().toArray());
   }

   /**
    * Add translation
    */
   private void addTranslation()
   {
      TranslationEditDialog dlg = new TranslationEditDialog(getShell(), true, "", null, translations.keySet());
      if (dlg.open() == Window.OK)
         applyTranslationEdit(dlg.getLanguage(), dlg.getValues());
   }

   /**
    * Edit translation
    */
   @SuppressWarnings("unchecked")
   private void editTranslation()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;

      Map.Entry<String, Map<String, String>> entry = (Map.Entry<String, Map<String, String>>)selection.getFirstElement();
      String language = entry.getKey();
      TranslationEditDialog dlg = new TranslationEditDialog(getShell(), false, language, entry.getValue(), translations.keySet());
      if (dlg.open() == Window.OK)
         applyTranslationEdit(language, dlg.getValues());
   }

   /**
    * Apply translation edit results
    * 
    * @param language language code
    * @param values translated strings
    */
   private void applyTranslationEdit(String language, Map<String, String> values)
   {
      if (values.isEmpty())
         translations.remove(language);
      else
         translations.put(language, new HashMap<String, String>(values));
      refreshViewer();
      if (!values.isEmpty())
         selectLanguageRow(language);
   }

   /**
    * Remove translation
    */
   @SuppressWarnings("unchecked")
   private void removeTranslation()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      Iterator<?> it = selection.iterator();
      while(it.hasNext())
      {
         Map.Entry<String, Map<String, String>> entry = (Map.Entry<String, Map<String, String>>)it.next();
         translations.remove(entry.getKey());
      }
      refreshViewer();
   }

   /**
    * Select row with given language code
    *
    * @param language language code
    */
   private void selectLanguageRow(String language)
   {
      for(Map.Entry<String, Map<String, String>> entry : translations.entrySet())
      {
         if (entry.getKey().equals(language))
         {
            viewer.setSelection(new StructuredSelection(entry));
            return;
         }
      }
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      objectTool.setTranslations(deepCopy(translations));
      WidgetHelper.saveColumnSettings(viewer.getTable(), "TranslationsPropertyPage");
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performCancel()
    */
   @Override
   public boolean performCancel()
   {
      if (isControlCreated())
         WidgetHelper.saveColumnSettings(viewer.getTable(), "TranslationsPropertyPage");
      return super.performCancel();
   }

   /**
    * Map deep copy
    *
    * @param src source
    * @return deep copy of the source
    */
   private static TreeMap<String, Map<String, String>> deepCopy(Map<String, Map<String, String>> src)
   {
      TreeMap<String, Map<String, String>> copy = new TreeMap<String, Map<String, String>>();
      if (src == null)
         return copy;
      for(Map.Entry<String, Map<String, String>> e : src.entrySet())
         copy.put(e.getKey(), new HashMap<String, String>(e.getValue()));
      return copy;
   }

   /**
    * Per-row label provider — pulls each translatable field by tag.
    */
   private static class TranslationLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      @SuppressWarnings("unchecked")
      public String getColumnText(Object element, int columnIndex)
      {
         Map.Entry<String, Map<String, String>> entry = (Map.Entry<String, Map<String, String>>)element;
         switch(columnIndex)
         {
            case 0:
               return entry.getKey();
            case 1:
               return safe(entry.getValue().get(ObjectToolDetails.TRANSLATION_FIELD_NAME));
            case 2:
               return safe(entry.getValue().get(ObjectToolDetails.TRANSLATION_FIELD_DESCRIPTION));
            case 3:
               return safe(entry.getValue().get(ObjectToolDetails.TRANSLATION_FIELD_CONFIRMATION_TEXT));
            case 4:
               return safe(entry.getValue().get(ObjectToolDetails.TRANSLATION_FIELD_COMMAND_NAME));
            case 5:
               return safe(entry.getValue().get(ObjectToolDetails.TRANSLATION_FIELD_COMMAND_SHORT_NAME));
            default:
               return "";
         }
      }

      private static String safe(String s)
      {
         return (s != null) ? s : "";
      }
   }
}
