/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.dialogs;

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
 * Add/edit header
 */
public class HeaderEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(HeaderEditDialog.class);

	private LabeledText textName;
	private LabeledText textValue;
	private String name;
	private String value;
	
	/**
	 * @param parentShell
	 */
	public HeaderEditDialog(Shell parentShell, String name, String value)
	{
		super(parentShell);
		this.name = name;
		this.value = value;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((name == null) ? i18n.tr("Add Header") : i18n.tr("Edit Header"));
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
		
      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(i18n.tr("Name"));
      textName.getTextControl().setTextLimit(63);
      if (name != null)
      {
      	textName.setText(name);
      	textName.getTextControl().setEditable(false);
      }
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);
      
      textValue = new LabeledText(dialogArea, SWT.NONE);
      textValue.setLabel(i18n.tr("Value"));
      textValue.getTextControl().setTextLimit(255);
      if (value != null)
      	textValue.setText(value);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textValue.setLayoutData(gd);

      if (name != null)
      	textValue.setFocus();

		return dialogArea;
	}

	/**
	 * Get variable name
	 * 
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * Get variable value
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
      name = textName.getText().trim();
      value = textValue.getText().trim();
		super.okPressed();
	}
}
