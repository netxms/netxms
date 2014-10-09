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
import org.eclipse.swt.widgets.Group;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Communication" property page for nodes
 */
public class Communication extends PropertyPage
{
	private AbstractNode node;
	private LabeledText primaryName;
	private LabeledText agentPort;
	private LabeledText agentSharedSecret;
	private Combo agentAuthMethod;
	private Button agentForceEncryption;
   private Button agentIsRemote;
	private ObjectSelector agentProxy;
	private Combo snmpVersion;
	private LabeledText snmpPort;
	private Combo snmpAuth;
	private Combo snmpPriv;
	private ObjectSelector snmpProxy;
	private LabeledText snmpAuthName;
	private LabeledText snmpAuthPassword;
	private LabeledText snmpPrivPassword;
	private ObjectSelector icmpProxy;
	private boolean primaryNameChanged = false;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		node = (AbstractNode)getElement().getAdapter(AbstractNode.class);

		Composite dialogArea = new Composite(parent, SWT.NONE);
		GridLayout dialogLayout = new GridLayout();
		dialogLayout.marginWidth = 0;
		dialogLayout.marginHeight = 0;
		dialogArea.setLayout(dialogLayout);
		
		// General
		Group generalGroup = new Group(dialogArea, SWT.NONE);
		generalGroup.setText(Messages.get().Communication_GroupGeneral);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		generalGroup.setLayoutData(gd);
		GridLayout generalGroupLayout = new GridLayout();
		generalGroup.setLayout(generalGroupLayout);
		
		primaryName = new LabeledText(generalGroup, SWT.NONE);
		primaryName.setLabel(Messages.get().Communication_PrimaryHostName);
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

      agentIsRemote = new Button(generalGroup, SWT.CHECK);
      agentIsRemote.setText("This is address of remote management node");
      agentIsRemote.setSelection((node.getFlags() & AbstractNode.NF_REMOTE_AGENT) != 0);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      agentIsRemote.setLayoutData(gd);
      		
		// Agent
		Group agentGroup = new Group(dialogArea, SWT.NONE);
		agentGroup.setText(Messages.get().Communication_GroupAgent);
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
		agentPort.setLabel(Messages.get().Communication_TCPPort);
		agentPort.setText(Integer.toString(node.getAgentPort()));
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		agentPort.setLayoutData(fd);
		
		agentProxy = new ObjectSelector(agentGroup, SWT.NONE, true);
		agentProxy.setLabel(Messages.get().Communication_Proxy);
		agentProxy.setObjectId(node.getAgentProxyId());
		fd = new FormData();
		fd.left = new FormAttachment(agentPort, 0, SWT.RIGHT);
		fd.right = new FormAttachment(100, 0);
		fd.top = new FormAttachment(0, 0);
		agentProxy.setLayoutData(fd);

		agentForceEncryption = new Button(agentGroup, SWT.CHECK);
		agentForceEncryption.setText(Messages.get().Communication_ForceEncryption);
		agentForceEncryption.setSelection((node.getFlags() & AbstractNode.NF_FORCE_ENCRYPTION) != 0);
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(agentPort, 0, SWT.BOTTOM);
		agentForceEncryption.setLayoutData(fd);
		
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(agentForceEncryption, 0, SWT.BOTTOM);
		agentAuthMethod = WidgetHelper.createLabeledCombo(agentGroup, SWT.BORDER | SWT.READ_ONLY, Messages.get().Communication_AuthMethod, fd);
		agentAuthMethod.add(Messages.get().Communication_AuthNone);
		agentAuthMethod.add(Messages.get().Communication_AuthPlain);
		agentAuthMethod.add(Messages.get().Communication_AuthMD5);
		agentAuthMethod.add(Messages.get().Communication_AuthSHA1);
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
				agentSharedSecret.getTextControl().setEnabled(agentAuthMethod.getSelectionIndex() != AbstractNode.AGENT_AUTH_NONE);
			}
		});
		
		agentSharedSecret = new LabeledText(agentGroup, SWT.NONE);
		agentSharedSecret.setLabel(Messages.get().Communication_SharedSecret);
		agentSharedSecret.setText(node.getAgentSharedSecret());
		fd = new FormData();
		fd.left = new FormAttachment(agentAuthMethod.getParent(), 0, SWT.RIGHT);
		fd.right = new FormAttachment(100, 0);
		fd.top = new FormAttachment(agentForceEncryption, 0, SWT.BOTTOM);
		agentSharedSecret.setLayoutData(fd);
		agentSharedSecret.getTextControl().setEnabled(node.getAgentAuthMethod() != AbstractNode.AGENT_AUTH_NONE);
	
		// SNMP
		Group snmpGroup = new Group(dialogArea, SWT.NONE);
		snmpGroup.setText(Messages.get().Communication_GroupSNMP);
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
		snmpVersion = WidgetHelper.createLabeledCombo(snmpGroup, SWT.BORDER | SWT.READ_ONLY, Messages.get().Communication_Version, fd);
		snmpVersion.add("1"); //$NON-NLS-1$
		snmpVersion.add("2c"); //$NON-NLS-1$
		snmpVersion.add("3"); //$NON-NLS-1$
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
		snmpPort.setLabel(Messages.get().Communication_UDPPort);
		snmpPort.setText(Integer.toString(node.getSnmpPort()));
		
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(snmpVersion.getParent(), 0, SWT.BOTTOM);
		snmpAuth = WidgetHelper.createLabeledCombo(snmpGroup, SWT.BORDER | SWT.READ_ONLY, Messages.get().Communication_Authentication, fd);
		snmpAuth.add(Messages.get().Communication_AuthNone);
		snmpAuth.add(Messages.get().Communication_AuthMD5);
		snmpAuth.add(Messages.get().Communication_AuthSHA1);
		snmpAuth.select(node.getSnmpAuthMethod());
		snmpAuth.setEnabled(node.getSnmpVersion() == AbstractNode.SNMP_VERSION_3);
		
		fd = new FormData();
		fd.left = new FormAttachment(snmpAuth.getParent(), 0, SWT.RIGHT);
		fd.top = new FormAttachment(snmpVersion.getParent(), 0, SWT.BOTTOM);
		snmpPriv = WidgetHelper.createLabeledCombo(snmpGroup, SWT.BORDER | SWT.READ_ONLY, Messages.get().Communication_Encryption, fd);
		snmpPriv.add(Messages.get().Communication_EncNone);
		snmpPriv.add(Messages.get().Communication_EncDES);
		snmpPriv.add(Messages.get().Communication_EncAES);
		snmpPriv.select(node.getSnmpPrivMethod());
		snmpPriv.setEnabled(node.getSnmpVersion() == AbstractNode.SNMP_VERSION_3);
		
		snmpProxy = new ObjectSelector(snmpGroup, SWT.NONE, true);
		snmpProxy.setLabel(Messages.get().Communication_Proxy);
		snmpProxy.setObjectId(node.getSnmpProxyId());
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(snmpAuth.getParent(), 0, SWT.BOTTOM);
		fd.right = new FormAttachment(snmpPriv.getParent(), 0, SWT.RIGHT);
		snmpProxy.setLayoutData(fd);
		
		snmpAuthName = new LabeledText(snmpGroup, SWT.NONE);
		snmpAuthName.setLabel(node.getSnmpVersion() == AbstractNode.SNMP_VERSION_3 ? Messages.get().Communication_UserName : Messages.get().Communication_Community);
		snmpAuthName.setText(node.getSnmpAuthName());
		fd = new FormData();
		fd.left = new FormAttachment(snmpProxy, 0, SWT.RIGHT);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		snmpAuthName.setLayoutData(fd);
		
		snmpAuthPassword = new LabeledText(snmpGroup, SWT.NONE);
		snmpAuthPassword.setLabel(Messages.get().Communication_AuthPassword);
		snmpAuthPassword.setText(node.getSnmpAuthPassword());
		fd = new FormData();
		fd.left = new FormAttachment(snmpAuthName, 0, SWT.LEFT);
		fd.top = new FormAttachment(snmpAuth.getParent(), 0, SWT.TOP);
		fd.right = new FormAttachment(100, 0);
		snmpAuthPassword.setLayoutData(fd);
		snmpAuthPassword.getTextControl().setEnabled(node.getSnmpVersion() == AbstractNode.SNMP_VERSION_3);
		
		snmpPrivPassword = new LabeledText(snmpGroup, SWT.NONE);
		snmpPrivPassword.setLabel(Messages.get().Communication_EncPassword);
		snmpPrivPassword.setText(node.getSnmpPrivPassword());
		fd = new FormData();
		fd.left = new FormAttachment(snmpAuthName, 0, SWT.LEFT);
		fd.top = new FormAttachment(snmpProxy, 0, SWT.TOP);
		fd.right = new FormAttachment(100, 0);
		snmpPrivPassword.setLayoutData(fd);
		snmpPrivPassword.getTextControl().setEnabled(node.getSnmpVersion() == AbstractNode.SNMP_VERSION_3);

		fd = new FormData();
		fd.left = new FormAttachment(snmpVersion.getParent(), 0, SWT.RIGHT);
		fd.right = new FormAttachment(snmpAuthName, 0, SWT.LEFT);
		fd.top = new FormAttachment(0, 0);
		snmpPort.setLayoutData(fd);
		
      // SNMP
      Group icmpGroup = new Group(dialogArea, SWT.NONE);
      icmpGroup.setText("ICMP");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      icmpGroup.setLayoutData(gd);
      
      GridLayout icmpGroupLayout = new GridLayout();
      icmpGroup.setLayout(icmpGroupLayout);
      
      icmpProxy = new ObjectSelector(icmpGroup, SWT.NONE, true);
      icmpProxy.setLabel(Messages.get().Communication_Proxy);
      icmpProxy.setObjectId(node.getIcmpProxyId());
      icmpProxy.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
		
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
			case AbstractNode.SNMP_VERSION_1:
				return 0;
			case AbstractNode.SNMP_VERSION_2C:
				return 1;
			case AbstractNode.SNMP_VERSION_3:
				return 2;
		}
		return 0;
	}
	
	/**
	 * Convert selection index in SNMP version combo box to SNMP version
	 */
	private int snmpIndexToVersion(int index)
	{
		final int[] versions = { AbstractNode.SNMP_VERSION_1, AbstractNode.SNMP_VERSION_2C, AbstractNode.SNMP_VERSION_3 };
		return versions[index];
	}
	
	/**
	 * Handler for SNMP version change
	 */
	private void onSnmpVersionChange()
	{
		boolean isV3 = (snmpVersion.getSelectionIndex() == 2);
		snmpAuthName.setLabel(isV3 ? Messages.get().Communication_UserName : Messages.get().Communication_Community);
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
			if (!hostName.matches("^([A-Za-z0-9\\-]+\\.)*[A-Za-z0-9\\-]+$")) //$NON-NLS-1$
			{
				MessageDialogHelper.openWarning(getShell(), Messages.get().Communication_Warning, 
				      String.format(Messages.get().Communication_WarningInvalidHostname, hostName));
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
			MessageDialog.openWarning(getShell(), Messages.get().Communication_Warning, Messages.get().Communication_WarningInvalidAgentPort);
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
			MessageDialog.openWarning(getShell(), Messages.get().Communication_Warning, Messages.get().Communication_WarningInvalidSNMPPort);
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
		
		md.setIcmpProxy(icmpProxy.getObjectId());
		
		/* TODO: sync in some way with "Polling" page */
		int flags = node.getFlags();
		if (agentForceEncryption.getSelection())
			flags |= AbstractNode.NF_FORCE_ENCRYPTION;
		else
			flags &= ~AbstractNode.NF_FORCE_ENCRYPTION;
      if (agentIsRemote.getSelection())
         flags |= AbstractNode.NF_REMOTE_AGENT;
      else
         flags &= ~AbstractNode.NF_REMOTE_AGENT;
		md.setObjectFlags(flags);

		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(String.format(Messages.get().Communication_JobName, node.getObjectName()), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().Communication_JobError;
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
		
		agentPort.setText("4700"); //$NON-NLS-1$
		agentForceEncryption.setSelection(false);
		agentAuthMethod.select(0);
		agentProxy.setObjectId(0);
		agentSharedSecret.setText(""); //$NON-NLS-1$
		agentSharedSecret.getTextControl().setEnabled(false);
		
		snmpVersion.select(0);
		snmpAuth.select(0);
		snmpPriv.select(0);
		snmpAuthName.setText("public"); //$NON-NLS-1$
		snmpAuthPassword.setText(""); //$NON-NLS-1$
		snmpPrivPassword.setText(""); //$NON-NLS-1$
		snmpProxy.setObjectId(0);
		onSnmpVersionChange();
	}
}
