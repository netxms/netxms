/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console.dialogs;

import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.Arrays;
import java.util.HashSet;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StackLayout;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Monitor;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.forms.widgets.Form;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.certificate.manager.CertificateManager;
import org.netxms.certificate.manager.CertificateManagerProvider;
import org.netxms.certificate.manager.CertificateManagerRequestListener;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.BrandingManager;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Login dialog
 */
public class LoginDialog extends Dialog implements SelectionListener, CertificateManagerRequestListener
{
	private FormToolkit toolkit;
	private ImageDescriptor loginImage;
	private Combo comboServer;
	private LabeledText textLogin;
	private LabeledText textPassword;
	private Combo comboCert;
	private String password;
	private Color labelColor;

	/**
	 * @param parentShell
	 */
	public LoginDialog(Shell parentShell)
	{
		super(parentShell);

		loginImage = BrandingManager.getInstance().getLoginTitleImage();
		if (loginImage == null)
			loginImage = Activator.getImageDescriptor("icons/login.png"); //$NON-NLS-1$
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		String customTitle = BrandingManager.getInstance().getLoginTitle();
		newShell.setText((customTitle != null) ? customTitle : Messages.LoginDialog_title); //$NON-NLS-1$

		// Center dialog on screen
		// We don't have main window at this moment, so use
		// monitor data to determine right position
		Monitor[] ma = newShell.getDisplay().getMonitors();
		if (ma != null)
		{
			newShell.setLocation((ma[0].getClientArea().width - newShell.getSize().x) / 2,
					(ma[0].getClientArea().height - newShell.getSize().y) / 2);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		toolkit = new FormToolkit(parent.getDisplay());
		Form dialogArea = toolkit.createForm(parent);
		dialogArea.setLayoutData(new GridData(GridData.FILL_BOTH));
		applyDialogFont(dialogArea);

		GridLayout dialogLayout = new GridLayout();
		dialogLayout.numColumns = 2;
		dialogLayout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		dialogLayout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		dialogLayout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		dialogArea.getBody().setLayout(dialogLayout);

		RGB customColor = BrandingManager.getInstance().getLoginTitleColor();
		labelColor = (customColor != null) ? new Color(dialogArea.getDisplay(), customColor) : new Color(dialogArea.getDisplay(), 36,
				66, 90);
		dialogArea.addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				labelColor.dispose();
			}
		});

		// Login image
		Label label = new Label(dialogArea.getBody(), SWT.NONE);
		label.setImage(loginImage.createImage());
		label.addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent event)
			{
				((Label)event.widget).getImage().dispose();
			}
		});
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.CENTER;
		gd.verticalAlignment = SWT.CENTER;
		gd.grabExcessVerticalSpace = true;
		label.setLayoutData(gd);

		Composite fields = toolkit.createComposite(dialogArea.getBody());
		fields.setBackgroundMode(SWT.INHERIT_DEFAULT);
		GridLayout fieldsLayout = new GridLayout();
		fieldsLayout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		fieldsLayout.marginHeight = 0;
		fieldsLayout.marginWidth = 0;
		fields.setLayout(fieldsLayout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		gd.grabExcessVerticalSpace = true;
		fields.setLayoutData(gd);

		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		comboServer = WidgetHelper.createLabeledCombo(fields, SWT.DROP_DOWN, Messages.LoginDialog_server, gd, toolkit);

		textLogin = new LabeledText(fields, SWT.NONE, SWT.SINGLE | SWT.BORDER, toolkit);
		textLogin.setLabel(Messages.LoginDialog_login);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = WidgetHelper.getTextWidth(textLogin, "M") * 24;
		textLogin.setLayoutData(gd);

		// textPassword = new LabeledText(fields, SWT.NONE, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD, toolkit);
		// textPassword.setLabel(Messages.LoginDialog_password);
		// gd = new GridData();
		// gd.horizontalAlignment = SWT.FILL;
		// gd.grabExcessHorizontalSpace = true;
		// textPassword.setLayoutData(gd);

		attachAuthenticationFields(settings, fields);

		// Read field data
		String[] items = settings.getArray("Connect.ServerHistory"); //$NON-NLS-1$
		if (items != null)
			comboServer.setItems(items);
		String text = settings.get("Connect.Server"); //$NON-NLS-1$
		if (text != null)
			comboServer.setText(text);

		text = settings.get("Connect.Login"); //$NON-NLS-1$
		if (text != null)
			textLogin.setText(text);

		// Set initial focus
		if (comboServer.getText().isEmpty())
			comboServer.setFocus();
		else if (textLogin.getText().isEmpty())
			textLogin.setFocus();
		else
			textPassword.setFocus();

		return dialogArea;
	}

	private void attachAuthenticationFields(IDialogSettings settings, Composite fields)
	{
		// Display display = Display.getCurrent();
		//
		// final Composite authComposite = new Composite(group, SWT.NONE);
		// authComposite.setBackground(display.getSystemColor(SWT.COLOR_WHITE));
		// final GridLayout authCompositeLayout = new GridLayout(1, true);
		// authComposite.setLayout(authCompositeLayout);
		// authComposite.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_FILL | GridData.GRAB_HORIZONTAL));

		// final Composite radios = new Composite(authComposite, SWT.NONE);
		final Composite radios = new Composite(fields, SWT.NONE);
		final RowLayout radiosLayout = new RowLayout(SWT.HORIZONTAL);
		radiosLayout.justify = true;
		radiosLayout.pack = false;

		radios.setLayout(radiosLayout);
		radios.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_FILL | GridData.GRAB_HORIZONTAL));

		final Button passwdButton = new Button(radios, SWT.RADIO);
		passwdButton.setText(Messages.LoginDialog_password);
		passwdButton.setSelection(true);

		final Button certButton = new Button(radios, SWT.RADIO);
		certButton.setText("Certificate");
		certButton.addSelectionListener(this);
		// certButton.setEnabled(false);

		attachAuthMethodComposite(fields, passwdButton, certButton);
	}

	private void attachAuthMethodComposite(final Composite fields, final Button passwdButton, final Button certButton)
	{
		final Composite authMethod = new Composite(fields, SWT.NONE);
		final StackLayout authMethodLayout = new StackLayout();

		authMethod.setLayout(authMethodLayout);
		authMethod.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_FILL | GridData.GRAB_HORIZONTAL));

		final LabeledText passwdField = attachPasswdField(authMethod);
		final Composite certField = attachCertField(authMethod);

		authMethodLayout.topControl = passwdButton.getSelection() ? passwdField : certField;

		Listener radioButtonListener = new Listener() {
			@Override
			public void handleEvent(Event event)
			{
				StackLayout layout = (StackLayout)authMethod.getLayout();
				layout.topControl = passwdButton.getSelection() ? passwdField : certField;
				authMethod.layout();
			}
		};

		passwdButton.addListener(SWT.Selection, radioButtonListener);
		certButton.addListener(SWT.Selection, radioButtonListener);
	}

	private LabeledText attachPasswdField(final Composite authMethod)
	{
		// final Composite passwdField = new Composite(authMethod, SWT.NONE);
		// passwdField.setLayout(new GridLayout(2, false));
		// passwdField.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_FILL | GridData.GRAB_HORIZONTAL));

		// final Label label = new Label(passwdField, SWT.NONE);
		//		label.setText(Messages.LoginDialog_password + ":"); //$NON-NLS-1$
		// textPassword = new LabeledText(passwdField, SWT.SINGLE | SWT.PASSWORD | SWT.BORDER);
		// textPassword.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_FILL | GridData.GRAB_HORIZONTAL));

		textPassword = new LabeledText(authMethod, SWT.NONE, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD, toolkit);
		textPassword.setLabel(Messages.LoginDialog_password);
		textPassword.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_FILL | GridData.GRAB_HORIZONTAL));
		// gd = new GridData();
		// gd.horizontalAlignment = SWT.FILL;
		// gd.grabExcessHorizontalSpace = true;
		// textPassword.setLayoutData(gd);

		return textPassword;
	}

	private Composite attachCertField(Composite authMethod)
	{
		final Composite certField = new Composite(authMethod, SWT.NONE);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = GridData.CENTER;
		layout.horizontalSpacing = 0;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;

		certField.setLayout(layout);
		certField.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_FILL | GridData.GRAB_HORIZONTAL));

		final Label label = new Label(certField, SWT.NONE);
		label.setText("Certificate");

		comboCert = new Combo(certField, SWT.DROP_DOWN | SWT.READ_ONLY);
		comboCert.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_FILL | GridData.GRAB_HORIZONTAL));

		// GridData gd = new GridData(GridData.HORIZONTAL_ALIGN_FILL | GridData.GRAB_HORIZONTAL);
		// final Combo comboCert = WidgetHelper.createLabeledCombo(authMethod, (SWT.DROP_DOWN | SWT.READ_ONLY), "Certificate", gd,
		// toolkit);
		// comboCert.setItems(new String[] { "A", "B", "C" });

		return certField;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		HashSet<String> items = new HashSet<String>();
		items.addAll(Arrays.asList(comboServer.getItems()));
		items.add(comboServer.getText());

		settings.put("Connect.Server", comboServer.getText()); //$NON-NLS-1$
		settings.put("Connect.ServerHistory", items.toArray(new String[items.size()])); //$NON-NLS-1$
		settings.put("Connect.Login", textLogin.getText()); //$NON-NLS-1$
		//      settings.put("Connect.Encrypt", checkBoxEncrypt.getSelection()); //$NON-NLS-1$

		password = textPassword.getText();

		super.okPressed();
	}

	/**
	 * @return the password
	 */
	public String getPassword()
	{
		return password;
	}

	@Override
	public void widgetSelected(SelectionEvent e)
	{
		Object source = e.getSource();

		if (source.toString().equals("Button {Certificate}"))
		{
			Button certButton = (Button) source;
			
			if (!certButton.getSelection()) return;
			
			fillCertCombo();
		}
	}

	private void fillCertCombo()
	{
		if(comboCert.getItemCount() != 0) return;
		
		CertificateManager certMgr = CertificateManagerProvider.provideCertificateManager(this);
		Certificate[] certs = certMgr.getCerts();
		String[] subjectStrings = new String[certs.length];
		
		for(int i = 0; i < certs.length; i++)
		{
			X509Certificate x509 = (X509Certificate) certs[i];
			String subject = x509.getSubjectDN().toString();
			
			String name = subject.split("\\s*,\\s*")[0].split("=")[1];
			
			subjectStrings[i] = name;
		}
		
		comboCert.setItems(subjectStrings);
	}

	@Override
	public void widgetDefaultSelected(SelectionEvent e) {}

	@Override
	public String keyStoreEntryPasswordRequested()
	{
		// TODO Auto-generated method stub
		return "test1337";
	}

	@Override
	public String keyStorePasswordRequested()
	{
		// TODO Auto-generated method stub
		return "test1337";
	}

	@Override
	public String keyStoreLocationRequested()
	{
		// TODO Auto-generated method stub
		return "/home/valentine/Documents/Subversion/netxms/src/java/certificate-manager/src/test/resource/keystore.p12";
	}
}
