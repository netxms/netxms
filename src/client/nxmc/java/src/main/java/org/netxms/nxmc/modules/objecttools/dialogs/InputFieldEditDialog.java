/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.InputField;
import org.netxms.client.constants.InputFieldType;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for creating or editing object tool input field
 */
public class InputFieldEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(InputFieldEditDialog.class);
   private final String[] typeNames = { i18n.tr("Text"), i18n.tr("Password"), i18n.tr("Number") };

	private boolean create;
	private InputField field;
	private LabeledText name;
	private Combo type;
	private LabeledText displayName;
	private Button checkValidatePassword;

	/**
	 * 
	 * @param parentShell
	 * @param create
	 * @param snmpColumn
	 */
	public InputFieldEditDialog(Shell parentShell, boolean create, InputField field)
	{
		super(parentShell);
		this.create = create;
		this.field = field;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(create ? i18n.tr("Add Input Field") : i18n.tr("Edit Input Field"));
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

		name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("Name"));
		name.setText(field.getName());
      name.setEditable(create);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 350;
		name.setLayoutData(gd);

      type = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Type"), WidgetHelper.DEFAULT_LAYOUT_DATA);
		for(int i = 0; i < typeNames.length; i++)
			type.add(typeNames[i]);
		type.select(field.getType().getValue());
      type.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            checkValidatePassword.setVisible(InputFieldType.getByValue(type.getSelectionIndex()) == InputFieldType.PASSWORD);
         }
      });

		displayName = new LabeledText(dialogArea, SWT.NONE);
      displayName.setLabel(i18n.tr("Display name"));
		displayName.setText(field.getDisplayName());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 350;
		displayName.setLayoutData(gd);

		checkValidatePassword = new Button(dialogArea, SWT.CHECK);
      checkValidatePassword.setText(i18n.tr("Validate password after entry"));
		checkValidatePassword.setVisible(field.getType() == InputFieldType.PASSWORD);
      checkValidatePassword.setSelection(field.isPasswordValidationNeeded());

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
	   if (create)
	      field.setName(name.getText());    
      field.setType(InputFieldType.getByValue(type.getSelectionIndex()));
	   field.setDisplayName(displayName.getText());

	   if (field.getType() == InputFieldType.PASSWORD)
	   {
         int flags = field.getFlags();
         if (checkValidatePassword.getSelection())
            flags |= InputField.VALIDATE_PASSWORD;
         else
            flags &= ~InputField.VALIDATE_PASSWORD;
         field.setFlags(flags);
	   }

		super.okPressed();
	}
}
