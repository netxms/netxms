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
package org.netxms.nxmc.modules.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Save graph as predefined
 */
public class SaveGraphDlg extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(SaveGraphDlg.class);

   public static final int OVERRIDE = 101;

   private LabeledText fieldName;
	private Label errorMessage;
	private String name;
	private Button checkOverwrite;
	private String errorMessageText;
	private boolean havePermissionToOverwrite;
	
	/**
	 * @param parentShell
	 */
	public SaveGraphDlg(Shell parentShell, String initialName, String message, boolean havePermissionToOverwrite)
	{
		super(parentShell);
		name = initialName;
		errorMessageText = message;
		this.havePermissionToOverwrite = havePermissionToOverwrite;
	}
	
   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Save Graph"));
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
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		dialogArea.setLayout(layout);
		
		fieldName = new LabeledText(dialogArea, SWT.NONE);
      fieldName.setLabel(i18n.tr("Name"));
		fieldName.setText(name);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 400;
		fieldName.setLayoutData(gd);

      if (errorMessageText != null)
      {
         errorMessage = new Label(dialogArea, SWT.LEFT);
         errorMessage.setForeground(ThemeEngine.getForegroundColor("List.Error"));
         errorMessage.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
         errorMessage.setText(errorMessageText);         
      }   

      if (havePermissionToOverwrite)
      {
         checkOverwrite = new Button(dialogArea, SWT.CHECK);
         checkOverwrite.setText(i18n.tr("Overwrite existing graph"));
      }
		
		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		name = fieldName.getText().trim();
		if (name.isEmpty())
		{
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Predefined graph name must not be empty!"));
			return;
		}
		if (havePermissionToOverwrite && checkOverwrite.getSelection())
		{
		   setReturnCode(OVERRIDE);
		   super.close();
		}
		else
		{
		   super.okPressed();
		}
	}
	
	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}
}
