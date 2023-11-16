/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.InetAddressEx;
import org.netxms.base.MacAddress;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.IPAddressValidator;
import org.netxms.nxmc.tools.IPNetMaskValidator;
import org.netxms.nxmc.tools.MacAddressValidator;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.NumericTextFieldValidator;
import org.netxms.nxmc.tools.ObjectNameValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Interface object creation dialog
 *
 */
public class CreateInterfaceDialog extends Dialog
{
   private static final int DEFAULT_MASK_BITS = 8;
   
   private I18n i18n = LocalizationHelper.getI18n(CreateInterfaceDialog.class);

	private LabeledText nameField;
   private LabeledText aliasField;
	private LabeledText macAddrField;
	private LabeledText ipAddrField;
	private LabeledText ipMaskField;
	private Button checkIsPhy;
	private LabeledText slotField;
	private LabeledText portField;

	private String name;
   private String alias;
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

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Create Interface"));
	}

   /**
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
      nameField.setLabel(i18n.tr("Name"));
		nameField.getTextControl().setTextLimit(255);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		gd.horizontalSpan = 2;
		nameField.setLayoutData(gd);

      aliasField = new LabeledText(dialogArea, SWT.NONE);
      aliasField.setLabel(i18n.tr("Alias"));
      aliasField.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      aliasField.setLayoutData(gd);

		macAddrField = new LabeledText(dialogArea, SWT.NONE);
      macAddrField.setLabel(i18n.tr("MAC address"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		macAddrField.setLayoutData(gd);

		ipAddrField = new LabeledText(dialogArea, SWT.NONE);
      ipAddrField.setLabel(i18n.tr("IP address"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		ipAddrField.setLayoutData(gd);

		ipMaskField = new LabeledText(dialogArea, SWT.NONE);
      ipMaskField.setLabel(i18n.tr("IP network mask"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		ipMaskField.setLayoutData(gd);

		checkIsPhy = new Button(dialogArea, SWT.CHECK);
      checkIsPhy.setText(i18n.tr("This interface is a physical port"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		checkIsPhy.setLayoutData(gd);
      checkIsPhy.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				slotField.setEnabled(checkIsPhy.getSelection());
				portField.setEnabled(checkIsPhy.getSelection());
			}
		});

		slotField = new LabeledText(dialogArea, SWT.NONE);
      slotField.setLabel(i18n.tr("Slot"));
      slotField.setText("0");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		slotField.setLayoutData(gd);
		slotField.setEnabled(false);

		portField = new LabeledText(dialogArea, SWT.NONE);
      portField.setLabel(i18n.tr("Port"));
      portField.setText("0");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		portField.setLayoutData(gd);
		portField.setEnabled(false);

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		physicalPort = checkIsPhy.getSelection();
		
		if (!WidgetHelper.validateTextInput(nameField, new ObjectNameValidator()) ||
		    !WidgetHelper.validateTextInput(macAddrField, new MacAddressValidator(true)) ||
		    !WidgetHelper.validateTextInput(ipAddrField, new IPAddressValidator(true)) ||
          !WidgetHelper.validateTextInput(ipMaskField, new IPNetMaskValidator(true, ipAddrField.getText().trim())) ||
		    (physicalPort && !WidgetHelper.validateTextInput(slotField, new NumericTextFieldValidator(0, 4096))) ||
		    (physicalPort && !WidgetHelper.validateTextInput(portField, new NumericTextFieldValidator(0, 4096))))
      {
         WidgetHelper.adjustWindowSize(this);
			return;
      }

		try
		{
			name = nameField.getText().trim();
         alias = aliasField.getText().trim();
			macAddress = macAddrField.getText().trim().isEmpty() ? new MacAddress() : MacAddress.parseMacAddress(macAddrField.getText());
         InetAddress addr = ipAddrField.getText().trim().isEmpty() ? InetAddress.getByName("0.0.0.0") : InetAddress.getByName(ipAddrField.getText());
			ipAddress = new InetAddressEx(addr, getMaskBits(ipMaskField.getText().trim(), addr instanceof Inet4Address ? 32 : 128));
			slot = physicalPort ? Integer.parseInt(slotField.getText()) : 0;
			port = physicalPort ? Integer.parseInt(portField.getText()) : 0;

			super.okPressed();
		}
		catch(Exception e)
		{
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), String.format(i18n.tr("Internal error: %s"), e.getLocalizedMessage()));
         WidgetHelper.adjustWindowSize(this);
		}
	}

	/**
	 * @param mask
	 * @param maxBits
	 * @return
	 */
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
    * @return the alias
    */
   public String getAlias()
   {
      return alias;
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
