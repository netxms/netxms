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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.NetworkService;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Network Service" property page for network service objects
 */
public class NetworkServicePolling extends PropertyPage
{
	private NetworkService object;
	private Combo serviceType;
	private LabeledText port;
	private LabeledText request;
	private LabeledText response;
	private ObjectSelector pollerNode;
	private Spinner pollCount;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (NetworkService)getElement().getAdapter(NetworkService.class);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		serviceType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Service type", gd);
		serviceType.add("User-defined");
		serviceType.add("SSH");
		serviceType.add("POP3");
		serviceType.add("SMTP");
		serviceType.add("FTP");
		serviceType.add("HTTP");
		serviceType.add("HTTPS");
		serviceType.add("Telnet");
		serviceType.select(object.getServiceType());
		
		port = new LabeledText(dialogArea, SWT.NONE);
		port.setLabel("Port");
		port.setText(Integer.toString(object.getPort()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		port.setLayoutData(gd);
		
		request = new LabeledText(dialogArea, SWT.NONE);
		request.setLabel("Request");
		request.setText(object.getRequest());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		request.setLayoutData(gd);
		
		response = new LabeledText(dialogArea, SWT.NONE);
		response.setLabel("Response");
		response.setText(object.getResponse());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		response.setLayoutData(gd);
		
		pollerNode = new ObjectSelector(dialogArea, SWT.NONE, true);
		pollerNode.setLabel("Poller node");
		pollerNode.setEmptySelectionName("<default>");
		pollerNode.setObjectClass(AbstractNode.class);
		pollerNode.setObjectId(object.getPollerNode());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		pollerNode.setLayoutData(gd);
      
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = false;
      pollCount = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, "Required poll count", 0, 1000, gd);
      pollCount.setSelection(object.getPollCount());
		
		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		
		try
		{
			int ipPort = Integer.parseInt(port.getText());
			if ((ipPort < 1) || (ipPort > 65535))
				throw new NumberFormatException();
			md.setIpPort(ipPort);
		}
		catch(NumberFormatException e)
		{
			MessageDialogHelper.openWarning(getShell(), "Warning", "Please enter valid port number (1 .. 65535)");
			return false;
		}
		
		md.setRequiredPolls(pollCount.getSelection());
		md.setServiceType(serviceType.getSelectionIndex());
		md.setRequest(request.getText());
		md.setResponse(response.getText());
		md.setPollerNode(pollerNode.getObjectId());
		
		if (isApply)
			setValid(false);
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Update network service object", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot update network service object";
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							NetworkServicePolling.this.setValid(true);
						}
					});
				}
			}
		}.start();
		return true;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		return applyChanges(false);
	}
}
