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
package org.netxms.ui.eclipse.serverconfig.dialogs;

import java.net.InetAddress;
import java.net.UnknownHostException;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.IpAddressListElement;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for adding address list element
 */
public class AddAddressListElementDialog extends Dialog
{
	private Button radioSubnet;
	private Button radioRange;
	private LabeledText textAddr1;
	private LabeledText textAddr2;
	private InetAddress address1;
	private InetAddress address2;
	private int type;
	
	/**
	 * @param parentShell
	 */
	public AddAddressListElementDialog(Shell parentShell)
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
		newShell.setText(Messages.get().AddAddressListElementDialog_Title);
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
		dialogArea.setLayout(layout);
		
		radioSubnet = new Button(dialogArea, SWT.RADIO);
		radioSubnet.setText(Messages.get().AddAddressListElementDialog_Subnet);
		radioSubnet.setSelection(true);
		radioSubnet.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				textAddr1.setLabel(Messages.get().AddAddressListElementDialog_NetAddr);
				textAddr2.setLabel(Messages.get().AddAddressListElementDialog_NetMask);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		radioRange = new Button(dialogArea, SWT.RADIO);
		radioRange.setText(Messages.get().AddAddressListElementDialog_AddrRange);
		radioRange.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				textAddr1.setLabel(Messages.get().AddAddressListElementDialog_StartAddr);
				textAddr2.setLabel(Messages.get().AddAddressListElementDialog_EndAddr);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		textAddr1 = new LabeledText(dialogArea, SWT.NONE);
		textAddr1.setLabel(Messages.get().AddAddressListElementDialog_NetAddr);
		textAddr1.setText("0.0.0.0"); //$NON-NLS-1$
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		textAddr1.setLayoutData(gd);
		
		textAddr2 = new LabeledText(dialogArea, SWT.NONE);
		textAddr2.setLabel(Messages.get().AddAddressListElementDialog_NetMask);
		textAddr2.setText("255.255.255.0"); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textAddr2.setLayoutData(gd);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		type = radioSubnet.getSelection() ? IpAddressListElement.SUBNET : IpAddressListElement.RANGE;
		try
		{
			address1 = InetAddress.getByName(textAddr1.getText());
			address2 = InetAddress.getByName(textAddr2.getText());
		}
		catch(UnknownHostException e)
		{
			MessageDialogHelper.openWarning(getShell(), Messages.get().AddAddressListElementDialog_Warning, Messages.get().AddAddressListElementDialog_EnterValidData);
			return;
		}
		super.okPressed();
	}

	/**
	 * @return the address1
	 */
	public InetAddress getAddress1()
	{
		return address1;
	}

	/**
	 * @return the address2
	 */
	public InetAddress getAddress2()
	{
		return address2;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}
}
