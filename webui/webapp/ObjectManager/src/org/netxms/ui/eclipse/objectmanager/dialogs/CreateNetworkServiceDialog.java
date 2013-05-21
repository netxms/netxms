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

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objects.NetworkService;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Network service object creation dialog
 *
 */
public class CreateNetworkServiceDialog extends Dialog
{
	private LabeledText nameField;
	private Combo serviceTypeField;
	private LabeledText portField;
	private LabeledText requestField;
	private LabeledText responseField;
	private Button checkCreateDci;
	
	private String name;
	private int serviceType;
	private int port;
	private String request;
	private String response;
	private boolean createDci;
	
	/**
	 * @param parentShell
	 */
	public CreateNetworkServiceDialog(Shell parentShell)
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
		newShell.setText("Create Network Service Object");
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
		
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		serviceTypeField = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Service type", gd);
		serviceTypeField.add("User-defined");
		serviceTypeField.add("SSH");
		serviceTypeField.add("POP3");
		serviceTypeField.add("SMTP");
		serviceTypeField.add("FTP");
		serviceTypeField.add("HTTP");
		serviceTypeField.add("Telnet");
		serviceTypeField.select(NetworkService.CUSTOM);
		
		portField = new LabeledText(dialogArea, SWT.NONE);
		portField.setLabel("Port");
		portField.setText("0");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		portField.setLayoutData(gd);
		
		requestField = new LabeledText(dialogArea, SWT.NONE);
		requestField.setLabel("Request");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		requestField.setLayoutData(gd);
		
		responseField = new LabeledText(dialogArea, SWT.NONE);
		responseField.setLabel("Response");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		responseField.setLayoutData(gd);

		final IDialogSettings settings = Activator.getDefault().getDialogSettings();
		checkCreateDci = new Button(dialogArea, SWT.CHECK);
		checkCreateDci.setText("&Create service status DCI at parent node");
		checkCreateDci.setSelection(settings.getBoolean("CreateNetworkServiceDialog.checkCreateDci"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		checkCreateDci.setLayoutData(gd);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		name = nameField.getText().trim();
		if (name.isEmpty())
		{
			MessageDialogHelper.openWarning(getShell(), "Warning", "Please provide non-empty object name");
			return;
		}
		
		serviceType = serviceTypeField.getSelectionIndex();
		
		try
		{
			port = Integer.parseInt(portField.getText());
			if ((port < 1) || (port > 65535))
				throw new NumberFormatException();
		}
		catch(NumberFormatException e)
		{
			MessageDialogHelper.openWarning(getShell(), "Warning", "Please enter valid port number (1 .. 65535)");
			return;
		}
		
		request = requestField.getText();
		response = responseField.getText();
		createDci = checkCreateDci.getSelection();
		
		final IDialogSettings settings = Activator.getDefault().getDialogSettings();
		settings.put("CreateNetworkServiceDialog.checkCreateDci", createDci);
		
		super.okPressed();
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}


	/**
	 * @return the serviceType
	 */
	public int getServiceType()
	{
		return serviceType;
	}


	/**
	 * @return the port
	 */
	public int getPort()
	{
		return port;
	}


	/**
	 * @return the request
	 */
	public String getRequest()
	{
		return request;
	}


	/**
	 * @return the response
	 */
	public String getResponse()
	{
		return response;
	}

	/**
	 * @return the createDci
	 */
	public boolean isCreateDci()
	{
		return createDci;
	}
}
