/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console.preferencepages;

import java.net.Authenticator;
import java.net.PasswordAuthentication;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * HTTP proxy preferences
 */
public class HttpProxyPrefs extends PreferencePage implements IWorkbenchPreferencePage
{
	private Button checkUseProxy;
	private LabeledText editProxyServer;
	private LabeledText editProxyPort;
	private LabeledText editExclusions;
	private Button checkRequireAuth;
	private LabeledText editLogin;
	private LabeledText editPassword;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchPreferencePage#init(org.eclipse.ui.IWorkbench)
	 */
	@Override
	public void init(IWorkbench workbench)
	{
		setPreferenceStore(Activator.getDefault().getPreferenceStore());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		checkUseProxy = new Button(dialogArea, SWT.CHECK);
		checkUseProxy.setText(Messages.HttpProxyPrefs_UserProxyMessage); //$NON-NLS-1$
		checkUseProxy.setSelection(getPreferenceStore().getBoolean("HTTP_PROXY_ENABLED")); //$NON-NLS-1$
		GridData gd = new GridData();
		gd.horizontalSpan = 2;
		checkUseProxy.setLayoutData(gd);
		checkUseProxy.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				boolean enabled = checkUseProxy.getSelection();
				editProxyServer.setEnabled(enabled);
				editProxyPort.setEnabled(enabled);
				editExclusions.setEnabled(enabled);
				checkRequireAuth.setEnabled(enabled);
				editLogin.setEnabled(enabled && checkRequireAuth.getEnabled());
				editPassword.setEnabled(enabled && checkRequireAuth.getEnabled());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		editProxyServer = new LabeledText(dialogArea, SWT.NONE);
		editProxyServer.setLabel(Messages.HttpProxyPrefs_ProxyServer); //$NON-NLS-1$
		editProxyServer.setText(getPreferenceStore().getString("HTTP_PROXY_SERVER")); //$NON-NLS-1$
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		editProxyServer.setLayoutData(gd);

		editProxyPort = new LabeledText(dialogArea, SWT.NONE);
		editProxyPort.setLabel(Messages.HttpProxyPrefs_Port); //$NON-NLS-1$
		editProxyPort.setText(getPreferenceStore().getString("HTTP_PROXY_PORT")); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.widthHint = 100;
		editProxyPort.setLayoutData(gd);
		
		editExclusions = new LabeledText(dialogArea, SWT.NONE);
		editExclusions.setLabel(Messages.HttpProxyPrefs_ExcludedAddresses); //$NON-NLS-1$
		editExclusions.setText(getPreferenceStore().getString("HTTP_PROXY_EXCLUSIONS").replaceAll("\\|", ",")); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalSpan = 2;
		editExclusions.setLayoutData(gd);

		editProxyServer.setEnabled(checkUseProxy.getSelection());
		editProxyPort.setEnabled(checkUseProxy.getSelection());
		editExclusions.setEnabled(checkUseProxy.getSelection());
		
		checkRequireAuth = new Button(dialogArea, SWT.CHECK);
		checkRequireAuth.setText(Messages.HttpProxyPrefs_ProxyRequireAuth); //$NON-NLS-1$
		checkRequireAuth.setSelection(getPreferenceStore().getBoolean("HTTP_PROXY_AUTH")); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalSpan = 2;
		gd.verticalIndent = WidgetHelper.DIALOG_SPACING;	// additional spacing before auth block
		checkRequireAuth.setLayoutData(gd);
		checkRequireAuth.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				boolean enabled = checkUseProxy.getSelection() && checkRequireAuth.getSelection();
				editLogin.setEnabled(enabled);
				editPassword.setEnabled(enabled);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});

		editLogin = new LabeledText(dialogArea, SWT.NONE);
		editLogin.setLabel(Messages.HttpProxyPrefs_Login); //$NON-NLS-1$
		editLogin.setText(getPreferenceStore().getString("HTTP_PROXY_LOGIN")); //$NON-NLS-1$
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalSpan = 2;
		editLogin.setLayoutData(gd);

		editPassword = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.PASSWORD);
		editPassword.setLabel(Messages.HttpProxyPrefs_Password); //$NON-NLS-1$
		editPassword.setText(getPreferenceStore().getString("HTTP_PROXY_PASSWORD")); //$NON-NLS-1$
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalSpan = 2;
		editPassword.setLayoutData(gd);

		boolean enabled = checkUseProxy.getSelection() && checkRequireAuth.getSelection();
		editLogin.setEnabled(enabled);
		editPassword.setEnabled(enabled);

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		
		checkUseProxy.setSelection(getPreferenceStore().getDefaultBoolean("HTTP_PROXY_ENABLED")); //$NON-NLS-1$
		editProxyServer.setText(getPreferenceStore().getDefaultString("HTTP_PROXY_SERVER")); //$NON-NLS-1$
		editProxyPort.setText(getPreferenceStore().getDefaultString("HTTP_PROXY_PORT")); //$NON-NLS-1$
		editExclusions.setText(getPreferenceStore().getDefaultString("HTTP_PROXY_EXCLUSIONS").replaceAll("\\|", ",")); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
		checkRequireAuth.setSelection(getPreferenceStore().getDefaultBoolean("HTTP_PROXY_AUTH")); //$NON-NLS-1$
		editLogin.setText(getPreferenceStore().getDefaultString("HTTP_PROXY_LOGIN")); //$NON-NLS-1$
		editPassword.setText(getPreferenceStore().getDefaultString("HTTP_PROXY_PASSWORD")); //$NON-NLS-1$

		editProxyServer.setEnabled(checkUseProxy.getSelection());
		editProxyPort.setEnabled(checkUseProxy.getSelection());
		editExclusions.setEnabled(checkUseProxy.getSelection());
		checkRequireAuth.setEnabled(checkUseProxy.getSelection());
		editLogin.setEnabled(checkUseProxy.getSelection() && checkRequireAuth.getSelection());
		editPassword.setEnabled(checkUseProxy.getSelection() && checkRequireAuth.getSelection());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		final IPreferenceStore ps = getPreferenceStore();
		
		ps.setValue("HTTP_PROXY_ENABLED", checkUseProxy.getSelection()); //$NON-NLS-1$
		ps.setValue("HTTP_PROXY_SERVER", editProxyServer.getText()); //$NON-NLS-1$
		ps.setValue("HTTP_PROXY_PORT", editProxyPort.getText()); //$NON-NLS-1$
		ps.setValue("HTTP_PROXY_EXCLUSIONS", editExclusions.getText().replaceAll("\\,", "|")); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
		ps.setValue("HTTP_PROXY_AUTH", checkRequireAuth.getSelection()); //$NON-NLS-1$
		ps.setValue("HTTP_PROXY_LOGIN", editLogin.getText()); //$NON-NLS-1$
		ps.setValue("HTTP_PROXY_PASSWORD", editPassword.getText()); //$NON-NLS-1$
		
		if (checkUseProxy.getSelection())
		{
			System.setProperty("http.proxyHost", ps.getString("HTTP_PROXY_SERVER")); //$NON-NLS-1$ //$NON-NLS-2$
			System.setProperty("http.proxyPort", ps.getString("HTTP_PROXY_PORT")); //$NON-NLS-1$ //$NON-NLS-2$
			System.setProperty("http.noProxyHosts", ps.getString("HTTP_PROXY_EXCLUSIONS")); //$NON-NLS-1$ //$NON-NLS-2$
			
			if (checkRequireAuth.getSelection())
			{
				Authenticator.setDefault(new Authenticator() {
					@Override
					protected PasswordAuthentication getPasswordAuthentication()
					{
						return new PasswordAuthentication(ps.getString("HTTP_PROXY_LOGIN"), ps.getString("HTTP_PROXY_PASSWORD").toCharArray()); //$NON-NLS-1$ //$NON-NLS-2$
					}
				});
			}
			else
			{
				Authenticator.setDefault(null);
			}
		}
		else
		{
			System.clearProperty("http.proxyHost"); //$NON-NLS-1$
			System.clearProperty("http.proxyPort"); //$NON-NLS-1$
			System.clearProperty("http.noProxyHosts"); //$NON-NLS-1$
			Authenticator.setDefault(null);
		}
		
		return super.performOk();
	}
}
