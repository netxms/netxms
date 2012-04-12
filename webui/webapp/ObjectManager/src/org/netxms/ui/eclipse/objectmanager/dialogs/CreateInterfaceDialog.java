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
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.MacAddress;
import org.netxms.ui.eclipse.console.tools.IPAddressValidator;
import org.netxms.ui.eclipse.console.tools.IPNetMaskValidator;
import org.netxms.ui.eclipse.console.tools.MacAddressValidator;
import org.netxms.ui.eclipse.console.tools.ObjectNameValidator;
import org.netxms.ui.eclipse.tools.NumericTextFieldValidator;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Interface object creation dialog
 *
 */
public class CreateInterfaceDialog extends Dialog
{
	private static final long serialVersionUID = 1L;

	private LabeledText nameField;
	private LabeledText macAddrField;
	private LabeledText ipAddrField;
	private LabeledText ipMaskField;
	private Button checkIsPhy;
	private LabeledText slotField;
	private LabeledText portField;
	
	private String name;
	private MacAddress macAddress;
	private InetAddress ipAddress;
	private InetAddress ipNetMask;
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
		newShell.setText("Create Interface Object");
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
		nameField.setLabel("Name");
		nameField.getTextControl().setTextLimit(255);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		gd.horizontalSpan = 2;
		nameField.setLayoutData(gd);
		
		macAddrField = new LabeledText(dialogArea, SWT.NONE);
		macAddrField.setLabel("MAC Address");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		macAddrField.setLayoutData(gd);
		
		ipAddrField = new LabeledText(dialogArea, SWT.NONE);
		ipAddrField.setLabel("IP Address");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		ipAddrField.setLayoutData(gd);
		
		ipMaskField = new LabeledText(dialogArea, SWT.NONE);
		ipMaskField.setLabel("IP Network Mask");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		ipMaskField.setLayoutData(gd);
		
		checkIsPhy = new Button(dialogArea, SWT.CHECK);
		checkIsPhy.setText("This interface is a physical port");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		checkIsPhy.setLayoutData(gd);
		checkIsPhy.addSelectionListener(new SelectionListener() {
			private static final long serialVersionUID = 1L;

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
		slotField.setLabel("Slot");
		slotField.setText("0");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		slotField.setLayoutData(gd);
		slotField.setEnabled(false);
		
		portField = new LabeledText(dialogArea, SWT.NONE);
		portField.setLabel("Port");
		portField.setText("0");
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
			ipAddress = ipAddrField.getText().trim().isEmpty() ? InetAddress.getByName("0.0.0.0") : InetAddress.getByName(ipAddrField.getText());
			ipNetMask = ipMaskField.getText().trim().isEmpty() ? InetAddress.getByName("0.0.0.0") : InetAddress.getByName(ipMaskField.getText());
			slot = physicalPort ? Integer.parseInt(slotField.getText()) : 0;
			port = physicalPort ? Integer.parseInt(portField.getText()) : 0;
			
			super.okPressed();
		}
		catch(Exception e)
		{
			MessageDialog.openError(getShell(), "Error", "Internal error: " + e.getMessage());
		}
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
	public InetAddress getIpAddress()
	{
		return ipAddress;
	}

	/**
	 * @return the ipNetMask
	 */
	public InetAddress getIpNetMask()
	{
		return ipNetMask;
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
