/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.base.preferencepages;

import java.net.Authenticator;
import java.net.PasswordAuthentication;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * HTTP proxy preferences
 */
public class HttpProxyPage extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(HttpProxyPage.class);
   
	private Button checkUseProxy;
	private LabeledText editProxyServer;
	private LabeledText editProxyPort;
	private LabeledText editExclusions;
	private Button checkRequireAuth;
	private LabeledText editLogin;
	private LabeledText editPassword;

   public HttpProxyPage()
   {
      super(LocalizationHelper.getI18n(HttpProxyPage.class).tr("HTTP Proxy"));
      setPreferenceStore(PreferenceStore.getInstance());
   }

   /**
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
		checkUseProxy.setText(i18n.tr("&Use proxy server for HTTP connections")); 
		checkUseProxy.setSelection(getPreferenceStore().getBoolean("HTTP_PROXY_ENABLED")); 
		GridData gd = new GridData();
		gd.horizontalSpan = 2;
		checkUseProxy.setLayoutData(gd);
      checkUseProxy.addSelectionListener(new SelectionAdapter() {
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
		});
		
		editProxyServer = new LabeledText(dialogArea, SWT.NONE);
		editProxyServer.setLabel(i18n.tr("Proxy server")); 
		editProxyServer.setText(getPreferenceStore().getString("HTTP_PROXY_SERVER")); 
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		editProxyServer.setLayoutData(gd);

		editProxyPort = new LabeledText(dialogArea, SWT.NONE);
		editProxyPort.setLabel(i18n.tr("Port")); 
		editProxyPort.setText(getPreferenceStore().getString("HTTP_PROXY_PORT")); 
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.widthHint = 100;
		editProxyPort.setLayoutData(gd);
		
		editExclusions = new LabeledText(dialogArea, SWT.NONE);
		editExclusions.setLabel(i18n.tr("Excluded addresses")); 
		editExclusions.setText(getPreferenceStore().getString("HTTP_PROXY_EXCLUSIONS").replaceAll("\\|", ","));   
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalSpan = 2;
		editExclusions.setLayoutData(gd);

		editProxyServer.setEnabled(checkUseProxy.getSelection());
		editProxyPort.setEnabled(checkUseProxy.getSelection());
		editExclusions.setEnabled(checkUseProxy.getSelection());
		
		checkRequireAuth = new Button(dialogArea, SWT.CHECK);
		checkRequireAuth.setText(i18n.tr("Proxy server requires &authentication")); 
		checkRequireAuth.setSelection(getPreferenceStore().getBoolean("HTTP_PROXY_AUTH")); 
		gd = new GridData();
		gd.horizontalSpan = 2;
		gd.verticalIndent = WidgetHelper.DIALOG_SPACING;	// additional spacing before auth block
		checkRequireAuth.setLayoutData(gd);
      checkRequireAuth.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				boolean enabled = checkUseProxy.getSelection() && checkRequireAuth.getSelection();
				editLogin.setEnabled(enabled);
				editPassword.setEnabled(enabled);
			}
		});

		editLogin = new LabeledText(dialogArea, SWT.NONE);
		editLogin.setLabel(i18n.tr("Login name")); 
		editLogin.setText(getPreferenceStore().getString("HTTP_PROXY_LOGIN")); 
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalSpan = 2;
		editLogin.setLayoutData(gd);

		editPassword = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.PASSWORD);
		editPassword.setLabel(i18n.tr("Password")); 
		editPassword.setText(getPreferenceStore().getString("HTTP_PROXY_PASSWORD")); 
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

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();

		checkUseProxy.setSelection(getPreferenceStore().getDefaultBoolean("HTTP_PROXY_ENABLED")); 
		editProxyServer.setText(getPreferenceStore().getDefaultString("HTTP_PROXY_SERVER")); 
		editProxyPort.setText(getPreferenceStore().getDefaultString("HTTP_PROXY_PORT")); 
		editExclusions.setText(getPreferenceStore().getDefaultString("HTTP_PROXY_EXCLUSIONS").replaceAll("\\|", ","));   
		checkRequireAuth.setSelection(getPreferenceStore().getDefaultBoolean("HTTP_PROXY_AUTH")); 
		editLogin.setText(getPreferenceStore().getDefaultString("HTTP_PROXY_LOGIN")); 
		editPassword.setText(getPreferenceStore().getDefaultString("HTTP_PROXY_PASSWORD")); 

		editProxyServer.setEnabled(checkUseProxy.getSelection());
		editProxyPort.setEnabled(checkUseProxy.getSelection());
		editExclusions.setEnabled(checkUseProxy.getSelection());
		checkRequireAuth.setEnabled(checkUseProxy.getSelection());
		editLogin.setEnabled(checkUseProxy.getSelection() && checkRequireAuth.getSelection());
		editPassword.setEnabled(checkUseProxy.getSelection() && checkRequireAuth.getSelection());
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
		final IPreferenceStore ps = getPreferenceStore();

		ps.setValue("HTTP_PROXY_ENABLED", checkUseProxy.getSelection()); 
		ps.setValue("HTTP_PROXY_SERVER", editProxyServer.getText()); 
		ps.setValue("HTTP_PROXY_PORT", editProxyPort.getText()); 
		ps.setValue("HTTP_PROXY_EXCLUSIONS", editExclusions.getText().replaceAll("\\,", "|"));   
		ps.setValue("HTTP_PROXY_AUTH", checkRequireAuth.getSelection()); 
		ps.setValue("HTTP_PROXY_LOGIN", editLogin.getText()); 
		ps.setValue("HTTP_PROXY_PASSWORD", editPassword.getText()); 

		if (checkUseProxy.getSelection())
		{
			System.setProperty("http.proxyHost", ps.getString("HTTP_PROXY_SERVER"));  
			System.setProperty("http.proxyPort", ps.getString("HTTP_PROXY_PORT"));  
			System.setProperty("http.noProxyHosts", ps.getString("HTTP_PROXY_EXCLUSIONS"));  
         System.setProperty("https.proxyHost", ps.getString("HTTP_PROXY_SERVER"));  
         System.setProperty("https.proxyPort", ps.getString("HTTP_PROXY_PORT"));  
         System.setProperty("https.noProxyHosts", ps.getString("HTTP_PROXY_EXCLUSIONS"));  

			if (checkRequireAuth.getSelection())
			{
				Authenticator.setDefault(new Authenticator() {
					@Override
					protected PasswordAuthentication getPasswordAuthentication()
					{
						return new PasswordAuthentication(ps.getString("HTTP_PROXY_LOGIN"), ps.getString("HTTP_PROXY_PASSWORD").toCharArray());  
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
			System.clearProperty("http.proxyHost"); 
			System.clearProperty("http.proxyPort"); 
			System.clearProperty("http.noProxyHosts"); 
         System.clearProperty("https.proxyHost"); 
         System.clearProperty("https.proxyPort"); 
         System.clearProperty("https.noProxyHosts"); 
			Authenticator.setDefault(null);
		}

      return true;
	}
}
