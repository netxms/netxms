/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.configs.CustomAttribute;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.JsonViewer;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;
import com.google.gson.JsonElement;
import com.google.gson.JsonParser;
import com.google.gson.JsonSyntaxException;

/**
 * Object's custom attribute edit dialog. Value can be edited either as plain text or as JSON document;
 * value type is selected with radio buttons, and the type selected when dialog is closed with OK
 * determines attribute type (plain text or structured).
 */
public class AttributeEditDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(AttributeEditDialog.class);
   private LabeledText textName;
   private Button radioText;
   private Button radioJson;
   private Composite valueArea;
   private LabeledText textValue;
   private JsonViewer jsonValue;
   private Label jsonValueLabel;
   private Button checkInherite;
   private String name;
   private String value;
   private long flags;
   private boolean inherited;
   private long source;

   /**
    * @param parentShell
    * @param source
    */
   public AttributeEditDialog(Shell parentShell, String name, String value, long flags, long source)
   {
      super(parentShell);
      this.name = name;
      this.value = value;
      this.flags = flags;
      this.inherited = (source != 0);
      this.source = source;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);

      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(i18n.tr("Name"));
      textName.getTextControl().setTextLimit(127);
      if (name != null)
      {
         textName.setText(name);
      }
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textName.setLayoutData(gd);

      boolean isJson = (flags & CustomAttribute.JSON) != 0;

      Group typeSelector = new Group(dialogArea, SWT.NONE);
      typeSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      typeSelector.setLayout(layout);
      typeSelector.setText(i18n.tr("Value type"));

      radioText = new Button(typeSelector, SWT.RADIO);
      radioText.setText(i18n.tr("Text"));
      radioText.setSelection(!isJson);

      radioJson = new Button(typeSelector, SWT.RADIO);
      radioJson.setText(i18n.tr("JSON"));
      radioJson.setSelection(isJson);

      valueArea = new Composite(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      valueArea.setLayout(layout);

      createValueEditor(isJson, (value != null) ? value : "");
      if (name != null)
         (isJson ? jsonValue : textValue).setFocus();

      SelectionAdapter typeChangeListener = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            if (!((Button)e.widget).getSelection())
               return; // Deselection event of the other radio button

            boolean json = (e.widget == radioJson);
            if (json == (jsonValue != null))
               return; // Editor of selected type is already displayed

            String currentValue = json ? textValue.getText() : jsonValue.getContent();
            if (json)
            {
               textValue.dispose();
            }
            else
            {
               jsonValue.dispose();
               jsonValueLabel.dispose();
            }
            createValueEditor(json, currentValue);
            (json ? jsonValue : textValue).setFocus();

            dialogArea.layout(true, true);
            getShell().pack();
         }
      };
      radioText.addSelectionListener(typeChangeListener);
      radioJson.addSelectionListener(typeChangeListener);

      checkInherite = new Button(dialogArea, SWT.CHECK);
      if (inherited)
      {
         NXCSession session = Registry.getSession();
         checkInherite.setText(String.format(i18n.tr("Inheritable (enforced by %s [%d])"), session.getObjectName(source), source));
      }
      else
      {
         checkInherite.setText(i18n.tr("Inheritable"));
      }
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      checkInherite.setLayoutData(gd);

      checkInherite.setSelection(inherited || (flags & CustomAttribute.INHERITABLE) > 0);
      checkInherite.setEnabled(!inherited);

      return dialogArea;
   }

   /**
    * Create value editor of given type inside value area. Only one editor exists at a time; caller is responsible for disposing
    * previous editor before calling this method. Layout data of value area is adjusted to match editor type.
    *
    * @param json true to create JSON editor, false to create plain text editor
    * @param initialValue initial editor content
    */
   private void createValueEditor(boolean json, String initialValue)
   {
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      if (json)
      {
         textValue = null;
         jsonValueLabel = new Label(valueArea, SWT.NONE);
         jsonValueLabel.setText(i18n.tr("Value"));

         jsonValue = new JsonViewer(valueArea, SWT.BORDER);
         jsonValue.setEditable(true);
         jsonValue.setContent(initialValue, true);
         gd.verticalAlignment = SWT.FILL;
         gd.grabExcessVerticalSpace = true;
         gd.heightHint = 250;
         jsonValue.setLayoutData(gd);
      }
      else
      {
         jsonValue = null;
         textValue = new LabeledText(valueArea, SWT.NONE);
         textValue.setLabel(i18n.tr("Value"));
         textValue.setText(initialValue);
         textValue.setLayoutData(gd);
      }

      GridData areaLayoutData = new GridData();
      areaLayoutData.horizontalAlignment = SWT.FILL;
      areaLayoutData.grabExcessHorizontalSpace = true;
      areaLayoutData.verticalAlignment = SWT.FILL;
      areaLayoutData.grabExcessVerticalSpace = json;
      valueArea.setLayoutData(areaLayoutData);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((name == null) ? i18n.tr("Add Attribute") : i18n.tr("Modify Attribute"));
   }

   /**
    * Get variable name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get variable value
    */
   public String getValue()
   {
      return value;
   }

   /**
    * Get variable flags
    */
   public long getFlags()
   {
      return flags;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      boolean isJson = (jsonValue != null);
      if (isJson)
      {
         String text = jsonValue.getContent().trim();
         try
         {
            JsonElement json = JsonParser.parseString(text);
            if (!json.isJsonObject() && !json.isJsonArray())
            {
               MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Structured value must be a JSON object or array"));
               return;
            }
         }
         catch(JsonSyntaxException e)
         {
            MessageDialogHelper.openError(getShell(), i18n.tr("Error"), String.format(i18n.tr("Value is not a valid JSON document (%s)"), e.getLocalizedMessage()));
            return;
         }
         value = text;
      }
      else
      {
         value = textValue.getText();
      }

      name = textName.getText().trim();
      flags = 0;
      if (checkInherite.getSelection())
         flags |= CustomAttribute.INHERITABLE;
      if (inherited)
         flags |= CustomAttribute.REDEFINED | CustomAttribute.INHERITABLE;
      if (isJson)
         flags |= CustomAttribute.JSON;
      super.okPressed();
   }
}
