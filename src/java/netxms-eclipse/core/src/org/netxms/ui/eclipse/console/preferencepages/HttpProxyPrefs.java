/**
 * 
 */
package org.netxms.ui.eclipse.console.preferencepages;

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
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * HTTP proxy preferences
 *
 */
public class HttpProxyPrefs extends PreferencePage implements IWorkbenchPreferencePage
{
	private Button checkUseProxy;
	private LabeledText editProxyServer;
	private LabeledText editProxyPort;
	private LabeledText editExclusions;
	
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
		checkUseProxy.setText("&Use proxy server for HTTP connections");
		checkUseProxy.setSelection(getPreferenceStore().getBoolean("HTTP_PROXY_ENABLED"));
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
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		editProxyServer = new LabeledText(dialogArea, SWT.NONE);
		editProxyServer.setLabel("Proxy server");
		editProxyServer.setText(getPreferenceStore().getString("HTTP_PROXY_SERVER"));
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		editProxyServer.setLayoutData(gd);

		editProxyPort = new LabeledText(dialogArea, SWT.NONE);
		editProxyPort.setLabel("Port");
		editProxyPort.setText(getPreferenceStore().getString("HTTP_PROXY_PORT"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.widthHint = 100;
		editProxyPort.setLayoutData(gd);
		
		editExclusions = new LabeledText(dialogArea, SWT.NONE);
		editExclusions.setLabel("Excluded addresses");
		editExclusions.setText(getPreferenceStore().getString("HTTP_PROXY_EXCLUSIONS").replaceAll("\\|", ","));
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalSpan = 2;
		editExclusions.setLayoutData(gd);

		editProxyServer.setEnabled(checkUseProxy.getSelection());
		editProxyPort.setEnabled(checkUseProxy.getSelection());
		editExclusions.setEnabled(checkUseProxy.getSelection());
		
		return dialogArea;
	}

	/* (non-Javadoc)
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

		editProxyServer.setEnabled(checkUseProxy.getSelection());
		editProxyPort.setEnabled(checkUseProxy.getSelection());
		editExclusions.setEnabled(checkUseProxy.getSelection());
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
		ps.setValue("HTTP_PROXY_EXCLUSIONS", editExclusions.getText().replaceAll("\\,", "|")); //$NON-NLS-1$
		
		if (checkUseProxy.getSelection())
		{
			System.setProperty("http.proxyHost", ps.getString("HTTP_PROXY_SERVER"));
			System.setProperty("http.proxyPort", ps.getString("HTTP_PROXY_PORT"));
			System.setProperty("http.noProxyHosts", ps.getString("HTTP_PROXY_EXCLUSIONS"));
		}
		else
		{
			System.clearProperty("http.proxyHost");
			System.clearProperty("http.proxyPort");
			System.clearProperty("http.noProxyHosts");
		}
		
		return super.performOk();
	}
}
