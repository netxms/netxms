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
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Group;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Communication" property page for nodes
 *
 */
public class Communication extends PropertyPage
{
	private Node node;
	private LabeledText primaryName;
	private LabeledText agentPort;
	private LabeledText agentSharedSecret;
	private Combo agentAuthMethod;
	private Button agentForceEncryption;
	private ObjectSelector agentProxy;
	private Combo snmpVersion;
	private LabeledText snmpPort;
	private Combo snmpAuth;
	private Combo snmpPriv;
	private ObjectSelector snmpProxy;
	private LabeledText snmpAuthName;
	private LabeledText snmpAuthPassword;
	private LabeledText snmpPrivPassword;
	private boolean primaryNameChanged = false;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		node = (Node)getElement().getAdapter(Node.class);

		Composite dialogArea = new Composite(parent, SWT.NONE);
		GridLayout dialogLayout = new GridLayout();
		dialogLayout.marginWidth = 0;
		dialogLayout.marginHeight = 0;
		dialogArea.setLayout(dialogLayout);
		
		// General
		Group generalGroup = new Group(dialogArea, SWT.NONE);
		generalGroup.setText("General");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		generalGroup.setLayoutData(gd);
		GridLayout generalGroupLayout = new GridLayout();
		generalGroup.setLayout(generalGroupLayout);
		
		primaryName = new LabeledText(generalGroup, SWT.NONE);
		primaryName.setLabel("Primary host name");
		primaryName.setText(node.getPrimaryName());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		primaryName.setLayoutData(gd);
		primaryName.getTextControl().addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				primaryNameChanged = true;
			}
		});
		
		// Agent
		Group agentGroup = new Group(dialogArea, SWT.NONE);
		agentGroup.setText("NetXMS Agent");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		agentGroup.setLayoutData(gd);
		
		FormLayout agentGroupLayout = new FormLayout();
		agentGroupLayout.marginWidth = WidgetHelper.OUTER_SPACING;
		agentGroupLayout.marginHeight = WidgetHelper.OUTER_SPACING;
		agentGroupLayout.spacing = WidgetHelper.OUTER_SPACING * 2;
		agentGroup.setLayout(agentGroupLayout);
		
		agentPort = new LabeledText(agentGroup, SWT.NONE);
		agentPort.setLabel("TCP port");
		agentPort.setText(Integer.toString(node.getAgentPort()));
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		agentPort.setLayoutData(fd);
		
		agentProxy = new ObjectSelector(agentGroup, SWT.NONE);
		agentProxy.setLabel("Proxy");
		agentProxy.setObjectId(node.getProxyNodeId());
		fd = new FormData();
		fd.left = new FormAttachment(agentPort, 0, SWT.RIGHT);
		fd.right = new FormAttachment(100, 0);
		fd.top = new FormAttachment(0, 0);
		agentProxy.setLayoutData(fd);

		agentForceEncryption = new Button(agentGroup, SWT.CHECK);
		agentForceEncryption.setText("Force encryption");
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(agentPort, 0, SWT.BOTTOM);
		agentForceEncryption.setLayoutData(fd);
		
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(agentForceEncryption, 0, SWT.BOTTOM);
		agentAuthMethod = WidgetHelper.createLabeledCombo(agentGroup, SWT.BORDER | SWT.READ_ONLY, "Authentication method", fd);
		agentAuthMethod.add("NONE");
		agentAuthMethod.add("PLAIN TEXT");
		agentAuthMethod.add("MD5");
		agentAuthMethod.add("SHA1");
		agentAuthMethod.select(node.getAgentAuthMethod());
		agentAuthMethod.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				agentSharedSecret.getTextControl().setEnabled(agentAuthMethod.getSelectionIndex() != Node.AGENT_AUTH_NONE);
			}
		});
		
		agentSharedSecret = new LabeledText(agentGroup, SWT.NONE);
		agentSharedSecret.setLabel("Shared secret");
		agentSharedSecret.setText(node.getAgentSharedSecret());
		fd = new FormData();
		fd.left = new FormAttachment(agentAuthMethod.getParent(), 0, SWT.RIGHT);
		fd.right = new FormAttachment(100, 0);
		fd.top = new FormAttachment(agentForceEncryption, 0, SWT.BOTTOM);
		agentSharedSecret.setLayoutData(fd);
		agentSharedSecret.getTextControl().setEnabled(node.getAgentAuthMethod() != Node.AGENT_AUTH_NONE);
	
		// SNMP
		Group snmpGroup = new Group(dialogArea, SWT.NONE);
		snmpGroup.setText("SNMP");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		snmpGroup.setLayoutData(gd);
		
		FormLayout snmpGroupLayout = new FormLayout();
		snmpGroupLayout.marginWidth = WidgetHelper.OUTER_SPACING;
		snmpGroupLayout.marginHeight = WidgetHelper.OUTER_SPACING;
		snmpGroupLayout.spacing = WidgetHelper.OUTER_SPACING * 2;
		snmpGroup.setLayout(snmpGroupLayout);
		
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		snmpVersion = WidgetHelper.createLabeledCombo(snmpGroup, SWT.BORDER | SWT.READ_ONLY, "Version", fd);
		snmpVersion.add("1");
		snmpVersion.add("2c");
		snmpVersion.add("3");
		snmpVersion.select(snmpVersionToIndex(node.getSnmpVersion()));
		snmpVersion.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				onSnmpVersionChange();
			}
		});
		
		snmpPort = new LabeledText(snmpGroup, SWT.NONE);
		snmpPort.setLabel("UDP Port");
		snmpPort.setText(Integer.toString(node.getSnmpPort()));
		
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(snmpVersion.getParent(), 0, SWT.BOTTOM);
		snmpAuth = WidgetHelper.createLabeledCombo(snmpGroup, SWT.BORDER | SWT.READ_ONLY, "Authentication", fd);
		snmpAuth.add("NONE");
		snmpAuth.add("MD5");
		snmpAuth.add("SHA1");
		snmpAuth.select(node.getSnmpAuthMethod());
		snmpAuth.setEnabled(node.getSnmpVersion() == Node.SNMP_VERSION_3);
		
		fd = new FormData();
		fd.left = new FormAttachment(snmpAuth.getParent(), 0, SWT.RIGHT);
		fd.top = new FormAttachment(snmpVersion.getParent(), 0, SWT.BOTTOM);
		snmpPriv = WidgetHelper.createLabeledCombo(snmpGroup, SWT.BORDER | SWT.READ_ONLY, "Encryption", fd);
		snmpPriv.add("NONE");
		snmpPriv.add("DES");
		snmpPriv.add("AES");
		snmpPriv.select(node.getSnmpPrivMethod());
		snmpPriv.setEnabled(node.getSnmpVersion() == Node.SNMP_VERSION_3);
		
		snmpProxy = new ObjectSelector(snmpGroup, SWT.NONE);
		snmpProxy.setLabel("Proxy");
		snmpProxy.setObjectId(node.getSnmpProxyId());
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(snmpAuth.getParent(), 0, SWT.BOTTOM);
		fd.right = new FormAttachment(snmpPriv.getParent(), 0, SWT.RIGHT);
		snmpProxy.setLayoutData(fd);
		
		snmpAuthName = new LabeledText(snmpGroup, SWT.NONE);
		snmpAuthName.setLabel(node.getSnmpVersion() == Node.SNMP_VERSION_3 ? "User name" : "Community string");
		snmpAuthName.setText(node.getSnmpAuthName());
		fd = new FormData();
		fd.left = new FormAttachment(snmpProxy, 0, SWT.RIGHT);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		snmpAuthName.setLayoutData(fd);
		
		snmpAuthPassword = new LabeledText(snmpGroup, SWT.NONE);
		snmpAuthPassword.setLabel("Authentication password");
		snmpAuthPassword.setText(node.getSnmpAuthPassword());
		fd = new FormData();
		fd.left = new FormAttachment(snmpAuthName, 0, SWT.LEFT);
		fd.top = new FormAttachment(snmpAuth.getParent(), 0, SWT.TOP);
		fd.right = new FormAttachment(100, 0);
		snmpAuthPassword.setLayoutData(fd);
		snmpAuthPassword.getTextControl().setEnabled(node.getSnmpVersion() == Node.SNMP_VERSION_3);
		
		snmpPrivPassword = new LabeledText(snmpGroup, SWT.NONE);
		snmpPrivPassword.setLabel("Encryption password");
		snmpPrivPassword.setText(node.getSnmpPrivPassword());
		fd = new FormData();
		fd.left = new FormAttachment(snmpAuthName, 0, SWT.LEFT);
		fd.top = new FormAttachment(snmpProxy, 0, SWT.TOP);
		fd.right = new FormAttachment(100, 0);
		snmpPrivPassword.setLayoutData(fd);
		snmpPrivPassword.getTextControl().setEnabled(node.getSnmpVersion() == Node.SNMP_VERSION_3);

		fd = new FormData();
		fd.left = new FormAttachment(snmpVersion.getParent(), 0, SWT.RIGHT);
		fd.right = new FormAttachment(snmpAuthName, 0, SWT.LEFT);
		fd.top = new FormAttachment(0, 0);
		snmpPort.setLayoutData(fd);
		
		return dialogArea;
	}

	/**
	 * Convert SNMP version to index in combo box
	 * 
	 * @param version SNMP version
	 * @return index in combo box
	 */
	private int snmpVersionToIndex(int version)
	{
		switch(version)
		{
			case Node.SNMP_VERSION_1:
				return 0;
			case Node.SNMP_VERSION_2C:
				return 1;
			case Node.SNMP_VERSION_3:
				return 2;
		}
		return 0;
	}
	
	/**
	 * Convert selection index in SNMP version combo box to SNMP version
	 */
	private int snmpIndexToVersion(int index)
	{
		final int[] versions = { Node.SNMP_VERSION_1, Node.SNMP_VERSION_2C, Node.SNMP_VERSION_3 };
		return versions[index];
	}
	
	/**
	 * Handler for SNMP version change
	 */
	private void onSnmpVersionChange()
	{
		boolean isV3 = (snmpVersion.getSelectionIndex() == 2);
		snmpAuthName.setLabel(isV3 ? "User name" : "Community string");
		snmpAuth.setEnabled(isV3);
		snmpPriv.setEnabled(isV3);
		snmpAuthPassword.getTextControl().setEnabled(isV3);
		snmpPrivPassword.getTextControl().setEnabled(isV3);
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		final NXCObjectModificationData md = new NXCObjectModificationData(node.getObjectId());
		
		if (primaryNameChanged)
		{
			// Validate primary name
			final String hostName = primaryName.getText().trim();
			if (!hostName.matches("^([A-Za-z0-9\\-]+\\.)*[A-Za-z0-9\\-]+$"))
			{
				MessageDialog.openWarning(getShell(), "Warning", "String \"" + hostName + "\" is not a valid host name or IP address. Please enter valid host name or IP address as primary host name");
				return false;
			}
			md.setPrimaryName(hostName);
		}
			
		if (isApply)
			setValid(false);
		
		try
		{
			md.setAgentPort(Integer.parseInt(agentPort.getText(), 10));
		}
		catch(NumberFormatException e)
		{
			MessageDialog.openWarning(getShell(), "Warning", "Please enter valid agent port number");
			if (isApply)
				setValid(true);
			return false;
		}
		md.setAgentProxy(agentProxy.getObjectId());
		md.setAgentAuthMethod(agentAuthMethod.getSelectionIndex());
		md.setAgentSecret(agentSharedSecret.getText());
		
		md.setSnmpVersion(snmpIndexToVersion(snmpVersion.getSelectionIndex()));
		try
		{
			md.setSnmpPort(Integer.parseInt(snmpPort.getText(), 10));
		}
		catch(NumberFormatException e)
		{
			MessageDialog.openWarning(getShell(), "Warning", "Please enter valid SNMP port number");
			if (isApply)
				setValid(true);
			return false;
		}
		md.setSnmpProxy(snmpProxy.getObjectId());
		md.setSnmpAuthMethod(snmpAuth.getSelectionIndex());
		md.setSnmpPrivMethod(snmpPriv.getSelectionIndex());
		md.setSnmpAuthName(snmpAuthName.getText());
		md.setSnmpAuthPassword(snmpAuthPassword.getText());
		md.setSnmpPrivPassword(snmpPrivPassword.getText());

		new ConsoleJob("Update communication settings for node " + node.getObjectName(), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				((NXCSession)ConsoleSharedData.getSession()).modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot update communication settings";
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					Display.getDefault().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							Communication.this.setValid(true);
						}
					});
				}
			}
		}.start();
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		return applyChanges(false);
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
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		
		agentPort.setText("4700");
		agentForceEncryption.setSelection(false);
		agentAuthMethod.select(0);
		agentProxy.setObjectId(0);
		agentSharedSecret.setText("");
		agentSharedSecret.getTextControl().setEnabled(false);
		
		snmpVersion.select(0);
		snmpAuth.select(0);
		snmpPriv.select(0);
		snmpAuthName.setText("public");
		snmpAuthPassword.setText("");
		snmpPrivPassword.setText("");
		snmpProxy.setObjectId(0);
		onSnmpVersionChange();
	}
}
