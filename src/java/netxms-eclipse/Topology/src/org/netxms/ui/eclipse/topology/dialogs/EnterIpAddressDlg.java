/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.topology.dialogs;

import java.net.InetAddress;
import java.net.UnknownHostException;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.objectbrowser.widgets.ZoneSelector;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.topology.Messages;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for selecting zone
 */
public class EnterIpAddressDlg extends Dialog
{
	private LabeledText ipAddressText;
	private ZoneSelector zoneSelector;
	private InetAddress ipAddress;
	private long zoneUIN;
	private boolean zoningEnabled;
	
	/**
	 * Constructor
	 * 
	 * @param parent Parent shell
	 */
	public EnterIpAddressDlg(Shell parent)
	{
		super(parent);
		zoningEnabled = ConsoleSharedData.getSession().isZoningEnabled();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().EnterIpAddressDlg_Title);
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
      dialogArea.setLayout(layout);
      
      ipAddressText = new LabeledText(dialogArea, SWT.NONE);
      ipAddressText.setLabel(Messages.get().EnterIpAddressDlg_IpAddress);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      ipAddressText.setLayoutData(gd);

      if (zoningEnabled)
      {
         zoneSelector = new ZoneSelector(dialogArea, SWT.NONE, false);
         zoneSelector.setLabel(Messages.get().EnterIpAddressDlg_Zone);
	      gd = new GridData();
	      gd.horizontalAlignment = SWT.FILL;
	      gd.grabExcessHorizontalSpace = true;
	      gd.widthHint = 300;
	      zoneSelector.setLayoutData(gd);
      }
      
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
			ipAddress = InetAddress.getByName(ipAddressText.getText());
		}
		catch(UnknownHostException e)
		{
			MessageDialogHelper.openWarning(getShell(), Messages.get().EnterIpAddressDlg_Warning, Messages.get().EnterIpAddressDlg_EnterValidAddress);
			return;
		}
		
		zoneUIN = zoningEnabled ? zoneSelector.getZoneUIN() : 0;
		super.okPressed();
	}

	/**
	 * @return the zoneObjectId
	 */
	public long getZoneId()
	{
		return zoneUIN;
	}

	/**
	 * @return the ipAddress
	 */
	public InetAddress getIpAddress()
	{
		return ipAddress;
	}
}
