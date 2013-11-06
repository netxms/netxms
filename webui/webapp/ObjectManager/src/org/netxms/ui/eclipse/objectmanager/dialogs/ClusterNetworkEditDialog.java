/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;


/**
 * Cluster's sync networks edit dialog
 */
public class ClusterNetworkEditDialog extends Dialog
{
	private Text textAddress;
	private Text textMask;
	private InetAddress address;
	private InetAddress mask;
	
	/**
	 * @param parentShell
	 */
	public ClusterNetworkEditDialog(Shell parentShell, InetAddress address, InetAddress mask)
	{
		super(parentShell);
		this.address = address;
		this.mask = mask;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		FillLayout layout = new FillLayout();
      layout.type = SWT.VERTICAL;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);
		
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Address");
      
      textAddress = new Text(dialogArea, SWT.SINGLE | SWT.BORDER);
      textAddress.setTextLimit(15);
      if (address != null)
      	textAddress.setText(address.getHostAddress());
      
      label = new Label(dialogArea, SWT.NONE);
      label.setText("");

      label = new Label(dialogArea, SWT.NONE);
      label.setText("Mask");

      textMask = new Text(dialogArea, SWT.SINGLE | SWT.BORDER);
      textMask.setTextLimit(15);
      textMask.getShell().setMinimumSize(300, 0);
      if (mask != null)
      	textMask.setText(mask.getHostAddress());
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText((address == null) ? "Add Network" : "Modify Network");
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		try
		{
			address = InetAddress.getByName(textAddress.getText());
			mask = InetAddress.getByName(textMask.getText());
		}
		catch(UnknownHostException e)
		{
			MessageDialogHelper.openWarning(getShell(), "Warning", "Please enter valid IP address and network mask");
			return;
		}
		super.okPressed();
	}

	/**
	 * @return the address
	 */
	public InetAddress getAddress()
	{
		return address;
	}

	/**
	 * @return the mask
	 */
	public InetAddress getMask()
	{
		return mask;
	}
}
