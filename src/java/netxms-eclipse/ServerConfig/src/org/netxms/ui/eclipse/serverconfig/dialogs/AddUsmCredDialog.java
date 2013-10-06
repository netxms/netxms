/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.serverconfig.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Add USM credentials dialog
 */
public class AddUsmCredDialog extends Dialog
{
	private LabeledText name;
	private LabeledText authPasswd;
	private LabeledText privPasswd;
	private Combo authMethod;
	private Combo privMethod;
	private SnmpUsmCredential value;
			
	/**
	 * @param parentShell
	 */
	public AddUsmCredDialog(Shell parentShell)
	{
		super(parentShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Add SNMP USM Credentials");
	}

	/* (non-Javadoc)
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
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		dialogArea.setLayout(layout);
		
		name = new LabeledText(dialogArea, SWT.NONE);
		name.setLabel("User name");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		name.setLayoutData(gd);
		
		authMethod = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Authentication", WidgetHelper.DEFAULT_LAYOUT_DATA);
		authMethod.add("NONE");
		authMethod.add("MD5");
		authMethod.add("SHA1");
		authMethod.select(2);

		privMethod = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Encryption", WidgetHelper.DEFAULT_LAYOUT_DATA);
		privMethod.add("NONE");
		privMethod.add("DES");
		privMethod.add("AES");
		privMethod.select(2);
		
		authPasswd = new LabeledText(dialogArea, SWT.NONE);
		authPasswd.setLabel("Authentication password");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		authPasswd.setLayoutData(gd);
		
		privPasswd = new LabeledText(dialogArea, SWT.NONE);
		privPasswd.setLabel("Encryption password");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		privPasswd.setLayoutData(gd);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		value = new SnmpUsmCredential();
		value.setName(name.getText().trim());
		value.setAuthMethod(authMethod.getSelectionIndex());
		value.setPrivMethod(privMethod.getSelectionIndex());
		value.setAuthPassword(authPasswd.getText());
		value.setPrivPassword(privPasswd.getText());
		super.okPressed();
	}

	/**
	 * @return the value
	 */
	public SnmpUsmCredential getValue()
	{
		return value;
	}
}
