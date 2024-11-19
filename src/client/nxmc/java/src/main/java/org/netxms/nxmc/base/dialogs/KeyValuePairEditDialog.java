/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.base.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Edit key/value pair
 */
public class KeyValuePairEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(KeyValuePairEditDialog.class);

	private LabeledText textKey;
	private LabeledText textValue;
	private String key;
	private String value;
	private String keyLabel;
   private String valueLabel;
	private boolean showValue;
   private boolean isNew;

	/**
    * Create dialog.
    *
    * @param parentShell parent shell
    * @param key key being edited
    * @param value current value
    * @param showValue true if value should be shown in dialog
    * @param isNew true if new element is being created
    */
	public KeyValuePairEditDialog(Shell parentShell, String key, String value, boolean showValue, boolean isNew, String keyLabel, String valueLabel)
	{
		super(parentShell);
      this.key = key;
      this.value = value;
		this.showValue = showValue;
		this.isNew = isNew;
		this.keyLabel = keyLabel;
      this.valueLabel = valueLabel;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(isNew ? i18n.tr("Create Value") : i18n.tr("Edit Value"));
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

      textKey = new LabeledText(dialogArea, SWT.NONE);
      textKey.setLabel(keyLabel);
      textKey.getTextControl().setTextLimit(256);
      if (key != null)
      {
      	textKey.setText(key);
      }
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textKey.setLayoutData(gd);
      if (showValue && !isNew)
         textKey.setEditable(false);

      if (showValue)
      {
         textValue = new LabeledText(dialogArea, SWT.NONE);
         textValue.setLabel(valueLabel);
         if (value != null)
         	textValue.setText(value);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         textValue.setLayoutData(gd);

         if (key != null)
         	textValue.setFocus();
      }

		return dialogArea;
	}

	/**
	 * Get attribute name
	 * 
	 */
	public String getKey()
	{
		return key;
	}

	/**
	 * Get attribute value
	 * 
	 */
	public String getValue()
	{
		return value;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		key = textKey.getText();
      if (showValue)
		   value = textValue.getText();
		super.okPressed();
	}
}
