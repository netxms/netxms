/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import java.net.Inet4Address;
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
import org.netxms.base.InetAddressEx;
import org.netxms.client.MacAddress;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.tools.IPAddressValidator;
import org.netxms.ui.eclipse.tools.IPNetMaskValidator;
import org.netxms.ui.eclipse.tools.MacAddressValidator;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.NumericTextFieldValidator;
import org.netxms.ui.eclipse.tools.ObjectNameValidator;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Interface object creation dialog
 *
 */
public class CreateInterfaceDialog extends Dialog
{
   private static final int DEFAULT_MASK_BITS = 8;
   
	private LabeledText nameField;
	private LabeledText macAddrField;
	private LabeledText ipAddrField;
	private LabeledText ipMaskField;
	private Button checkIsPhy;
	private LabeledText slotField;
	private LabeledText portField;
	
	private String name;
	private MacAddress macAddress;
	private InetAddressEx ipAddress;
	private int ifIndex;
	private int ifType;
	private int slot;
	private int port;
	private boolean physicalPort;
	
	/**
	 * @param parentShell
	 */
	public CreateInterfaceDialog(Shell parentShell)
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
		newShell.setText(Messages.get().CreateInterfaceDialog_Title);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		dialogArea.setLayout(layout);
		
		nameField = new LabeledText(dialogArea, SWT.NONE);
		nameField.setLabel(Messages.get().CreateInterfaceDialog_Name);
		nameField.getTextControl().setTextLimit(255);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		gd.horizontalSpan = 2;
		nameField.setLayoutData(gd);
		
		macAddrField = new LabeledText(dialogArea, SWT.NONE);
		macAddrField.setLabel(Messages.get().CreateInterfaceDialog_MACAddr);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		macAddrField.setLayoutData(gd);
		
		ipAddrField = new LabeledText(dialogArea, SWT.NONE);
		ipAddrField.setLabel(Messages.get().CreateInterfaceDialog_IPAddr);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		ipAddrField.setLayoutData(gd);
		
		ipMaskField = new LabeledText(dialogArea, SWT.NONE);
		ipMaskField.setLabel(Messages.get().CreateInterfaceDialog_IPNetMak);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		ipMaskField.setLayoutData(gd);
		
		checkIsPhy = new Button(dialogArea, SWT.CHECK);
		checkIsPhy.setText(Messages.get().CreateInterfaceDialog_IsPhysicalPort);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		checkIsPhy.setLayoutData(gd);
		checkIsPhy.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				slotField.setEnabled(checkIsPhy.getSelection());
				portField.setEnabled(checkIsPhy.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		slotField = new LabeledText(dialogArea, SWT.NONE);
		slotField.setLabel(Messages.get().CreateInterfaceDialog_Slot);
		slotField.setText("0"); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		slotField.setLayoutData(gd);
		slotField.setEnabled(false);
		
		portField = new LabeledText(dialogArea, SWT.NONE);
		portField.setLabel(Messages.get().CreateInterfaceDialog_Port);
		portField.setText("0"); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		portField.setLayoutData(gd);
		portField.setEnabled(false);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		physicalPort = checkIsPhy.getSelection();
		
		if (!WidgetHelper.validateTextInput(nameField, new ObjectNameValidator(), null) ||
		    !WidgetHelper.validateTextInput(macAddrField, new MacAddressValidator(true), null) ||
		    !WidgetHelper.validateTextInput(ipAddrField, new IPAddressValidator(true), null) ||
		    !WidgetHelper.validateTextInput(ipMaskField, new IPNetMaskValidator(true), null) ||
		    (physicalPort && !WidgetHelper.validateTextInput(slotField, new NumericTextFieldValidator(0, 4096), null)) ||
		    (physicalPort && !WidgetHelper.validateTextInput(portField, new NumericTextFieldValidator(0, 4096), null)))
			return;
		
		try
		{
			name = nameField.getText().trim();
			macAddress = macAddrField.getText().trim().isEmpty() ? new MacAddress() : MacAddress.parseMacAddress(macAddrField.getText());
			InetAddress addr = ipAddrField.getText().trim().isEmpty() ? InetAddress.getByName("0.0.0.0") : InetAddress.getByName(ipAddrField.getText()); //$NON-NLS-1$
			ipAddress = new InetAddressEx(addr, getMaskBits(ipMaskField.getText().trim(), addr instanceof Inet4Address ? 32 : 128));
			slot = physicalPort ? Integer.parseInt(slotField.getText()) : 0;
			port = physicalPort ? Integer.parseInt(portField.getText()) : 0;
			
			super.okPressed();
		}
		catch(Exception e)
		{
			MessageDialogHelper.openError(getShell(), Messages.get().CreateInterfaceDialog_Error, String.format("Internal error: %s", e.getMessage())); //$NON-NLS-1$
		}
	}
	
	private int getMaskBits(String mask, int maxBits)
	{
	   if (mask.isEmpty())
	      return DEFAULT_MASK_BITS;
	   
	   try
	   {
	      int bits = Integer.parseInt(mask);
	      return ((bits >= 0) && (bits <= maxBits)) ? bits : DEFAULT_MASK_BITS;
	   }
	   catch(NumberFormatException e)
	   {
	   }
	   
	   try
	   {
	      InetAddress addr = InetAddress.getByName(mask);
	      return InetAddressEx.bitsInMask(addr);
	   }
	   catch(UnknownHostException e)
	   {
	   }
	   
	   return DEFAULT_MASK_BITS;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the port
	 */
	public int getPort()
	{
		return port;
	}

	/**
	 * @return the macAddress
	 */
	public MacAddress getMacAddress()
	{
		return macAddress;
	}

	/**
	 * @return the ipAddress
	 */
	public InetAddressEx getIpAddress()
	{
		return ipAddress;
	}

	/**
	 * @return the ifIndex
	 */
	public int getIfIndex()
	{
		return ifIndex;
	}

	/**
	 * @return the ifType
	 */
	public int getIfType()
	{
		return ifType;
	}

	/**
	 * @return the slot
	 */
	public int getSlot()
	{
		return slot;
	}

	/**
	 * @return the physicalPort
	 */
	public boolean isPhysicalPort()
	{
		return physicalPort;
	}
}
