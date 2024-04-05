/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.propertypages;

import java.net.InetAddress;
import java.net.UnknownHostException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.base.InetAddressEx;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkService;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Network Service" property page for network service objects
 */
public class NetworkServicePolling extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(NetworkServicePolling.class);

   private NetworkService service;
	private Combo serviceType;
	private LabeledText port;
	private LabeledText request;
	private LabeledText response;
	private LabeledText ipAddress;
	private ObjectSelector pollerNode;
	private Spinner pollCount;
	
   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public NetworkServicePolling(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(NetworkServicePolling.class).tr("Polling"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "networkServicePolling";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 10;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof NetworkService);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
      service = (NetworkService)object;

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		serviceType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Service type"), gd);
		serviceType.add(i18n.tr("User-defined"));
		serviceType.add(i18n.tr("SSH"));
		serviceType.add(i18n.tr("POP3"));
		serviceType.add(i18n.tr("SMTP"));
		serviceType.add(i18n.tr("FTP"));
		serviceType.add(i18n.tr("HTTP"));
		serviceType.add(i18n.tr("HTTPS"));
		serviceType.add(i18n.tr("Telnet"));
      serviceType.select(service.getServiceType());
		
		port = new LabeledText(dialogArea, SWT.NONE);
		port.setLabel(i18n.tr("Port"));
      port.setText(Integer.toString(service.getPort()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		port.setLayoutData(gd);
		
		ipAddress = new LabeledText(dialogArea, SWT.NONE);
		ipAddress.setLabel("IP Address");
      if (service.getIpAddress().isValidAddress())
         ipAddress.setText(service.getIpAddress().getHostAddress());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      ipAddress.setLayoutData(gd);
		
		request = new LabeledText(dialogArea, SWT.NONE);
		request.setLabel(i18n.tr("Request"));
      request.setText(service.getRequest());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		request.setLayoutData(gd);
		
		response = new LabeledText(dialogArea, SWT.NONE);
		response.setLabel(i18n.tr("Response"));
      response.setText(service.getResponse());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		response.setLayoutData(gd);
		
		pollerNode = new ObjectSelector(dialogArea, SWT.NONE, true);
		pollerNode.setLabel(i18n.tr("Poller node"));
		pollerNode.setEmptySelectionName(i18n.tr("<default>"));
		pollerNode.setObjectClass(AbstractNode.class);
      pollerNode.setObjectId(service.getPollerNode());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		pollerNode.setLayoutData(gd);
      
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = false;
      pollCount = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, i18n.tr("Required poll count"), 0, 1000, gd);
      pollCount.setSelection(service.getPollCount());
		
		return dialogArea;
	}

	/**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
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
			MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter valid port number (1 .. 65535)"));
			return false;
		}

		md.setRequiredPolls(pollCount.getSelection());
		md.setServiceType(serviceType.getSelectionIndex());
		String addr = ipAddress.getText().trim();
		if (!addr.isEmpty())
		{
   		try
         {
         	md.setIpAddress(new InetAddressEx(InetAddress.getByName(addr)));
         }
         catch(UnknownHostException e)
         {
         	MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Address validation error"));
         	return false;
         }
		}
		else
		{
		   md.setIpAddress(new InetAddressEx());
		}
		md.setRequest(request.getText());
		md.setResponse(response.getText());
		md.setPollerNode(pollerNode.getObjectId());
		
		if (isApply)
			setValid(false);
		
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating network service object"), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot update network service object");
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
}
