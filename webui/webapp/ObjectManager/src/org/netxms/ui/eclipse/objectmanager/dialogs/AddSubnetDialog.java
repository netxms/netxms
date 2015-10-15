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
package org.netxms.ui.eclipse.objectmanager.dialogs;

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
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for adding address list element
 */
public class AddSubnetDialog extends Dialog
{
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

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Add Subnet");
	}

	/* (non-Javadoc)
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
		address.setLabel(Messages.get().AddAddressListElementDialog_NetworkAddress);
		address.setText("0.0.0.0"); //$NON-NLS-1$
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		address.setLayoutData(gd);		

		mask = new LabeledSpinner(dialogArea, SWT.NONE);
		mask.setLabel(Messages.get().AddAddressListElementDialog_NetworkMask);
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
			MessageDialogHelper.openWarning(getShell(), Messages.get().AddAddressListElementDialog_Warning, Messages.get().AddAddressListElementDialog_AddressValidationError);
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
