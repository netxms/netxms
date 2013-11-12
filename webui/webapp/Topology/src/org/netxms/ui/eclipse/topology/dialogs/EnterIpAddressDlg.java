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
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Zone;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
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
	private ObjectSelector objectSelector;
	private InetAddress ipAddress;
	private long zoneId;
	private boolean zoningEnabled;
	
	/**
	 * Constructor
	 * 
	 * @param parent Parent shell
	 */
	public EnterIpAddressDlg(Shell parent)
	{
		super(parent);
		zoningEnabled = ((NXCSession)ConsoleSharedData.getSession()).isZoningEnabled();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.EnterIpAddressDlg_Title);
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
      ipAddressText.setLabel(Messages.EnterIpAddressDlg_IpAddress);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      ipAddressText.setLayoutData(gd);

      if (zoningEnabled)
      {
	      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, false);
	      objectSelector.setLabel(Messages.EnterIpAddressDlg_Zone);
	      objectSelector.setObjectClass(Zone.class);
	      objectSelector.setClassFilter(ObjectSelectionDialog.createZoneSelectionFilter());
	      gd = new GridData();
	      gd.horizontalAlignment = SWT.FILL;
	      gd.grabExcessHorizontalSpace = true;
	      gd.widthHint = 300;
	      objectSelector.setLayoutData(gd);
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
			MessageDialogHelper.openWarning(getShell(), Messages.EnterIpAddressDlg_Warning, Messages.EnterIpAddressDlg_EnterValidAddress);
			return;
		}
		
		if (zoningEnabled)
		{
			long objectId = objectSelector.getObjectId();
			if (objectId == 0)
			{
				MessageDialogHelper.openWarning(getShell(), Messages.EnterIpAddressDlg_Warning, Messages.EnterIpAddressDlg_SelectZone);
				return;
			}
			AbstractObject object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(objectId);
			if ((object == null) || !(object instanceof Zone))
			{
				MessageDialogHelper.openWarning(getShell(), Messages.EnterIpAddressDlg_Warning, Messages.EnterIpAddressDlg_SelectZone);
				return;
			}
			zoneId = ((Zone)object).getZoneId();
		}
		else
		{
			zoneId = 0;
		}
		super.okPressed();
	}

	/**
	 * @return the zoneObjectId
	 */
	public long getZoneId()
	{
		return zoneId;
	}

	/**
	 * @return the ipAddress
	 */
	public InetAddress getIpAddress()
	{
		return ipAddress;
	}
}
