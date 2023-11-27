/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objects.NetworkService;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Network service object creation dialog
 *
 */
public class CreateNetworkServiceDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(CreateNetworkServiceDialog.class);
   
	private LabeledText nameField;
   private LabeledText aliasField;
	private Combo serviceTypeField;
	private LabeledText portField;
	private LabeledText requestField;
	private LabeledText responseField;
	private Button checkCreateDci;
	
	private String name;
   private String alias;
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
		newShell.setText(i18n.tr("Create Network Service Object"));
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

		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		serviceTypeField = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Service type"), gd);
		serviceTypeField.add(i18n.tr("User-defined"));
		serviceTypeField.add(i18n.tr("SSH"));
		serviceTypeField.add(i18n.tr("POP3"));
		serviceTypeField.add(i18n.tr("SMTP"));
		serviceTypeField.add(i18n.tr("FTP"));
		serviceTypeField.add(i18n.tr("HTTP"));
		serviceTypeField.add(i18n.tr("HTTPS"));
		serviceTypeField.add(i18n.tr("Telnet"));
		serviceTypeField.select(NetworkService.CUSTOM);
		
		portField = new LabeledText(dialogArea, SWT.NONE);
		portField.setLabel(i18n.tr("Port"));
		portField.setText("0"); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		portField.setLayoutData(gd);
		
		requestField = new LabeledText(dialogArea, SWT.NONE);
		requestField.setLabel(i18n.tr("Request"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		requestField.setLayoutData(gd);
		
		responseField = new LabeledText(dialogArea, SWT.NONE);
		responseField.setLabel(i18n.tr("Response"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		responseField.setLayoutData(gd);

      PreferenceStore settings = PreferenceStore.getInstance();
		checkCreateDci = new Button(dialogArea, SWT.CHECK);
		checkCreateDci.setText(i18n.tr("&Create service status DCI at parent node"));
		checkCreateDci.setSelection(settings.getAsBoolean("CreateNetworkServiceDialog.checkCreateDci", false)); //$NON-NLS-1$
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
			MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please provide non-empty object name"));
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
			MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter valid port number (1 .. 65535)"));
			return;
		}
      alias = aliasField.getText().trim();
		request = requestField.getText();
		response = responseField.getText();
		createDci = checkCreateDci.getSelection();

      PreferenceStore settings = PreferenceStore.getInstance();
		settings.set("CreateNetworkServiceDialog.checkCreateDci", createDci); //$NON-NLS-1$
		
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
    * @return the alias
    */
   public String getAlias()
   {
      return alias;
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
