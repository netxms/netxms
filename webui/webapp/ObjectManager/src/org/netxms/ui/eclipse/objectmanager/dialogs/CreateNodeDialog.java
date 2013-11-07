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

import java.net.Inet4Address;
import java.net.InetAddress;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Zone;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for creating new node
 */
public class CreateNodeDialog extends Dialog
{
	private NXCSession session;
	
	private LabeledText objectNameField;
	private LabeledText hostNameField;
	private Spinner agentPortField;
	private Spinner snmpPortField;
	private Button checkUnmanaged;
	private Button checkDisableAgent;
	private Button checkDisableSNMP;
	private Button checkDisablePing;
	private ObjectSelector agentProxySelector;
	private ObjectSelector snmpProxySelector;
	private ObjectSelector zoneSelector;
	
	private String objectName;
	private String hostName;
	private int creationFlags;
	private long agentProxy;
	private long snmpProxy;
	private long zoneId;
	private int agentPort;
	private int snmpPort;
	
	/**
	 * @param parentShell
	 */
	public CreateNodeDialog(Shell parentShell)
	{
		super(parentShell);
		session = (NXCSession)ConsoleSharedData.getSession();
		zoneId = 0;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Create Node Object");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		objectNameField = new LabeledText(dialogArea, SWT.NONE);
		objectNameField.setLabel("Name");
		objectNameField.getTextControl().setTextLimit(255);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		gd.horizontalSpan = 2;
		objectNameField.setLayoutData(gd);
		
		final Composite ipAddrGroup = new Composite(dialogArea, SWT.NONE);
		layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 2;
		ipAddrGroup.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		ipAddrGroup.setLayoutData(gd);
		
		hostNameField = new LabeledText(ipAddrGroup, SWT.NONE);
		hostNameField.setLabel("Primary host name or IP address");
		hostNameField.getTextControl().setTextLimit(255);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		hostNameField.setLayoutData(gd);
		
		final Button resolve = new Button(ipAddrGroup, SWT.PUSH);
		resolve.setText("&Resolve");
		gd = new GridData();
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		gd.verticalAlignment = SWT.BOTTOM;
		resolve.setLayoutData(gd);
		resolve.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				resolveName();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		agentPortField = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, "NetXMS agent port", 1, 65535, WidgetHelper.DEFAULT_LAYOUT_DATA);
		agentPortField.setSelection(4700);
		
		snmpPortField = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, "SNMP agent port", 1, 65535, WidgetHelper.DEFAULT_LAYOUT_DATA);
		snmpPortField.setSelection(161);
		
		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText("Options");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		optionsGroup.setLayoutData(gd);
		optionsGroup.setLayout(new RowLayout(SWT.VERTICAL));
		
		checkUnmanaged = new Button(optionsGroup, SWT.CHECK);
		checkUnmanaged.setText("Create as &unmanaged object");

		checkDisableAgent = new Button(optionsGroup, SWT.CHECK);
		checkDisableAgent.setText("Disable usage of NetXMS &agent for all polls");
		
		checkDisableSNMP = new Button(optionsGroup, SWT.CHECK);
		checkDisableSNMP.setText("Disable usage of &SNMP for all polls");
		
		checkDisablePing = new Button(optionsGroup, SWT.CHECK);
		checkDisablePing.setText("Disable usage of &ICMP ping for all polls");
		
		agentProxySelector = new ObjectSelector(dialogArea, SWT.NONE, true);
		agentProxySelector.setLabel("Proxy for NetXMS agents");
		agentProxySelector.setObjectClass(Node.class);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		agentProxySelector.setLayoutData(gd);
		
		snmpProxySelector = new ObjectSelector(dialogArea, SWT.NONE, true);
		snmpProxySelector.setLabel("Proxy for SNMP");
		snmpProxySelector.setObjectClass(Node.class);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		snmpProxySelector.setLayoutData(gd);
		
		if (session.isZoningEnabled())
		{
			zoneSelector = new ObjectSelector(dialogArea, SWT.NONE, false);
			zoneSelector.setLabel("Zone");
			zoneSelector.setObjectClass(Zone.class);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			gd.horizontalSpan = 2;
			zoneSelector.setLayoutData(gd);
		}
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		// Validate primary name
		hostName = hostNameField.getText().trim();
		if (hostName.isEmpty())
			hostName = objectNameField.getText().trim();
		if (!hostName.matches("^([A-Za-z0-9\\-]+\\.)*[A-Za-z0-9\\-]+$"))
		{
			MessageDialogHelper.openWarning(getShell(), "Warning", "String \"" + hostName + "\" is not a valid host name or IP address. Please enter valid host name or IP address as primary host name");
			return;
		}
		
		objectName = objectNameField.getText().trim();
		if (objectName.isEmpty())
			objectName = hostName;
		
		creationFlags = 0;
		if (checkUnmanaged.getSelection())
			creationFlags |= NXCObjectCreationData.CF_CREATE_UNMANAGED;
		if (checkDisableAgent.getSelection())
			creationFlags |= NXCObjectCreationData.CF_DISABLE_NXCP;
		if (checkDisablePing.getSelection())
			creationFlags |= NXCObjectCreationData.CF_DISABLE_ICMP;
		if (checkDisableSNMP.getSelection())
			creationFlags |= NXCObjectCreationData.CF_DISABLE_SNMP;
		
		agentPort = agentPortField.getSelection();
		snmpPort = snmpPortField.getSelection();
		
		agentProxy = agentProxySelector.getObjectId();
		snmpProxy = snmpProxySelector.getObjectId();
		if (session.isZoningEnabled())
		{
			long zoneObjectId = zoneSelector.getObjectId();
			AbstractObject object = session.findObjectById(zoneObjectId);
			if ((object != null) && (object instanceof Zone))
			{
				zoneId = ((Zone)object).getZoneId();
			}
		}
		
		super.okPressed();
	}
	
	/**
	 * Resolve entered name to IP address
	 */
	private void resolveName()
	{
		final String name = objectNameField.getText();
		new ConsoleJob("Resolve host name", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final InetAddress addr = Inet4Address.getByName(name);
				PlatformUI.getWorkbench().getDisplay().asyncExec(new Runnable() {
					@Override
					public void run()
					{
						hostNameField.setText(addr.getHostAddress());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot resolve host name " + name + " to IP address";
			}
		}.start();
	}

	/**
	 * @return the name
	 */
	public String getObjectName()
	{
		return objectName;
	}

	/**
	 * @return the hostName
	 */
	public String getHostName()
	{
		return hostName;
	}

	/**
	 * @return the creationFlags
	 */
	public int getCreationFlags()
	{
		return creationFlags;
	}

	/**
	 * @return the agentProxy
	 */
	public long getAgentProxy()
	{
		return agentProxy;
	}

	/**
	 * @return the snmpProxy
	 */
	public long getSnmpProxy()
	{
		return snmpProxy;
	}

	/**
	 * @return the zoneId
	 */
	public long getZoneId()
	{
		return zoneId;
	}

	/**
	 * @return the agentPort
	 */
	public int getAgentPort()
	{
		return agentPort;
	}

	/**
	 * @return the snmpPort
	 */
	public int getSnmpPort()
	{
		return snmpPort;
	}
}
