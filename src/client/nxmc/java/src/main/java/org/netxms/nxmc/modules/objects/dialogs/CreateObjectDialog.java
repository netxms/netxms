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
package org.netxms.nxmc.modules.objects.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Generic object creation dialog
 */
public class CreateObjectDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(CreateObjectDialog.class);
	private String objectClassName;
	private String objectName;
   private String objectAlias;
	private Text textName;
   private Text textAlias;

	/**
	 * Constructor
	 * 
	 * @param parentShell Parent shell
	 * @param objectClassName Object class - this string will be added to dialog's title
	 */
	public CreateObjectDialog(Shell parentShell, String objectClassName)
	{
		super(parentShell);
		this.objectClassName = objectClassName;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(String.format(i18n.tr("Create %s"), objectClassName));
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
      dialogArea.setLayout(layout);
		
      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, i18n.tr("Name"), "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      textName.getShell().setMinimumSize(300, 0);
      textName.setTextLimit(63);
      textName.setFocus();
      
      textAlias = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, i18n.tr("Alias"), "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      textAlias.setTextLimit(255);

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		objectName = textName.getText().trim();
		if (objectName.isEmpty())
		{
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Object name cannot be empty"));
			return;
		}
      objectAlias = textAlias.getText().trim();
		super.okPressed();
	}

	/**
	 * @return the objectName
	 */
	public String getObjectName()
	{
		return objectName;
	}

   /**
    * @return the objectAlias
    */
   public String getObjectAlias()
   {
      return objectAlias;
   }
}
