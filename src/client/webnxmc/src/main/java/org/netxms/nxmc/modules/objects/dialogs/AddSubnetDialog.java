/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

import java.net.InetAddress;
import java.net.UnknownHostException;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.InetAddressEx;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for adding address list element
 */
public class AddSubnetDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(AddSubnetDialog.class);
	private LabeledText address;
	private LabeledSpinner mask;
	private InetAddressEx subnet;
	
	/**
	 * @param parentShell
	 */
	public AddSubnetDialog(Shell parentShell)
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
		newShell.setText(i18n.tr("Add Subnet"));
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
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		
		address = new LabeledText(dialogArea, SWT.NONE);
		address.setLabel(i18n.tr("Network address"));
		address.setText("0.0.0.0"); //$NON-NLS-1$
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		address.setLayoutData(gd);		

		mask = new LabeledSpinner(dialogArea, SWT.NONE);
		mask.setLabel(i18n.tr("Network mask"));
		mask.getSpinnerControl().setMinimum(0);
      mask.getSpinnerControl().setMaximum(128);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      mask.setLayoutData(gd);    
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		try
		{
			InetAddress a = InetAddress.getByName(address.getText().trim());
			subnet = new InetAddressEx(a, mask.getSelection());
		}
		catch(UnknownHostException e)
		{
			MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Address validation error"));
			return;
		}
		super.okPressed();
	}

	/**
	 * @return subnet address
	 */
	public InetAddressEx getSubnet()
	{
		return subnet;
	}
}
