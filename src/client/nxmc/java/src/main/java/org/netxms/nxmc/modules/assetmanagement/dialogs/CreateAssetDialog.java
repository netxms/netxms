/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.assetmanagement.dialogs;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.assetmanagement.widgets.AssetPropertyEditor;
import org.netxms.nxmc.tools.ObjectNameValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for creating asset objects
 */
public class CreateAssetDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(CreateAssetDialog.class);

   private Map<String, AssetAttribute> schema = Registry.getSession().getAssetManagementSchema();
   private LabeledText nameField;
   private LabeledText aliasField;
   private List<AssetPropertyEditor> propertyEditors = new ArrayList<AssetPropertyEditor>(schema.size());
   private String name;
   private String alias;
   private Map<String, String> properties;

   /**
    * Create dialog.
    *
    * @param parentShell parent shell
    */
   public CreateAssetDialog(Shell parentShell)
   {
      super(parentShell);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Create Asset"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);

      nameField = new LabeledText(dialogArea, SWT.NONE);
      nameField.setLabel(i18n.tr("Name"));
      nameField.getTextControl().setTextLimit(63);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      gd.horizontalSpan = 2;
      nameField.setLayoutData(gd);

      aliasField = new LabeledText(dialogArea, SWT.NONE);
      aliasField.setLabel(i18n.tr("Alias"));
      aliasField.getTextControl().setTextLimit(63);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      aliasField.setLayoutData(gd);
      
      schema.values().stream().sorted((a1, a2) -> a1.getName().compareToIgnoreCase(a2.getName())).forEach((a) -> createPropertyEditor(dialogArea, a));

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("* denotes mandatory fields"));
      gd = new GridData();
      gd.horizontalSpan = layout.numColumns;
      gd.verticalIndent = WidgetHelper.OUTER_SPACING * 2;
      label.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * Create property editor.
    *
    * @param dialogArea dialog area
    * @param attribute attribute to create editor for
    */
   private void createPropertyEditor(Composite dialogArea, AssetAttribute attribute)
   {
      AssetPropertyEditor editor = new AssetPropertyEditor(dialogArea, SWT.NONE, attribute);
      GridData gd = new GridData();
      gd.widthHint = 300;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.TOP;
      editor.setLayoutData(gd);
      propertyEditors.add(editor);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      boolean success = WidgetHelper.validateTextInput(nameField, new ObjectNameValidator());
      for(AssetPropertyEditor e : propertyEditors)
      {
         if (!e.validateInput())
            success = false;
      }
      if (!success)
      {
         WidgetHelper.adjustWindowSize(this);
         return;
      }

      alias = aliasField.getText().trim();
      name = nameField.getText().trim();

      properties = new HashMap<>();
      for(AssetPropertyEditor e : propertyEditors)
      {
         String value = e.getValue();
         if (!value.isEmpty())
            properties.put(e.getAttribute().getName(), e.getValue());
      }

      super.okPressed();
   }

   /**
    * Get name for new object
    *
    * @return name for new object
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get alias for new object
    *
    * @return alias for new object
    */
   public String getAlias()
   {
      return alias;
   }

   /**
    * Get properties for new asset.
    *
    * @return properties for new asset
    */
   public Map<String, String> getProperties()
   {
      return properties;
   }
}
