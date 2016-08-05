/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Zone;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Messages;
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
	private LabeledText sshLoginField;
   private LabeledText sshPasswordField;
	private Button checkUnmanaged;
	private Button checkDisableAgent;
	private Button checkDisableSNMP;
	private Button checkDisablePing;
	private Button checkCreateAnother;
	private ObjectSelector agentProxySelector;
	private ObjectSelector snmpProxySelector;
   private ObjectSelector icmpProxySelector;
   private ObjectSelector sshProxySelector;
	private ObjectSelector zoneSelector;
	
	private String objectName;
	private String hostName;
	private int creationFlags = 0;
	private long agentProxy = 0;
	private long snmpProxy = 0;
   private long icmpProxy = 0;
   private long sshProxy = 0;
	private long zoneId = 0;
	private int agentPort = 4700;
	private int snmpPort = 161;
	private String sshLogin = "";
	private String sshPassword; 
	private boolean showAgain = false;
	
	/**
	 * @param parentShell
	 */
	public CreateNodeDialog(Shell parentShell, CreateNodeDialog prev)
	{
		super(parentShell);
		session = (NXCSession)ConsoleSharedData.getSession();
		if (prev != null)
		{
         creationFlags = prev.creationFlags;
         agentProxy = prev.agentProxy;
         snmpProxy = prev.snmpProxy;
         icmpProxy = prev.snmpProxy;
         sshProxy = prev.snmpProxy;
		   zoneId = prev.zoneId;
		   agentPort = prev.agentPort;
		   snmpPort = prev.snmpPort;
		   sshLogin = prev.sshLogin;
		   sshPassword = prev.sshPassword;
         showAgain = prev.showAgain;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().CreateNodeDialog_Title);
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
		layout.makeColumnsEqualWidth = true;
		dialogArea.setLayout(layout);
		
		objectNameField = new LabeledText(dialogArea, SWT.NONE);
		objectNameField.setLabel(Messages.get().CreateNodeDialog_Name);
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
		hostNameField.setLabel(Messages.get().CreateNodeDialog_PrimaryHostName);
		hostNameField.getTextControl().setTextLimit(255);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		hostNameField.setLayoutData(gd);
		
		agentPortField = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, Messages.get().CreateNodeDialog_AgentPort, 1, 65535, WidgetHelper.DEFAULT_LAYOUT_DATA);
		agentPortField.setSelection(agentPort);
		
		snmpPortField = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, Messages.get().CreateNodeDialog_SNMPPort, 1, 65535, WidgetHelper.DEFAULT_LAYOUT_DATA);
		snmpPortField.setSelection(snmpPort);
		
		sshLoginField = new LabeledText(dialogArea, SWT.NONE);
		sshLoginField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
		sshLoginField.setLabel("SSH Login");
		sshLoginField.setText(sshLogin);
		
      sshPasswordField = new LabeledText(dialogArea, SWT.NONE);
      sshPasswordField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      sshPasswordField.setLabel("SSH Password");
      sshPasswordField.setText(sshPassword);
      
		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText(Messages.get().CreateNodeDialog_Options);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		optionsGroup.setLayoutData(gd);
		optionsGroup.setLayout(new RowLayout(SWT.VERTICAL));
		
		checkUnmanaged = new Button(optionsGroup, SWT.CHECK);
		checkUnmanaged.setText(Messages.get().CreateNodeDialog_CreateUnmanaged);
		checkUnmanaged.setSelection((creationFlags & NXCObjectCreationData.CF_CREATE_UNMANAGED) != 0);

		checkDisableAgent = new Button(optionsGroup, SWT.CHECK);
		checkDisableAgent.setText(Messages.get().CreateNodeDialog_DisableAgent);
		checkDisableAgent.setSelection((creationFlags & NXCObjectCreationData.CF_DISABLE_NXCP) != 0);
		
		checkDisableSNMP = new Button(optionsGroup, SWT.CHECK);
		checkDisableSNMP.setText(Messages.get().CreateNodeDialog_DisableSNMP);
		checkDisableSNMP.setSelection((creationFlags & NXCObjectCreationData.CF_DISABLE_SNMP) != 0);
		
		checkDisablePing = new Button(optionsGroup, SWT.CHECK);
		checkDisablePing.setText(Messages.get().CreateNodeDialog_DisableICMP);
		checkDisablePing.setSelection((creationFlags & NXCObjectCreationData.CF_DISABLE_ICMP) != 0);
		
		agentProxySelector = new ObjectSelector(dialogArea, SWT.NONE, true);
		agentProxySelector.setLabel(Messages.get().CreateNodeDialog_AgentProxy);
		agentProxySelector.setObjectClass(Node.class);
		agentProxySelector.setObjectId(agentProxy);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		agentProxySelector.setLayoutData(gd);
		
		snmpProxySelector = new ObjectSelector(dialogArea, SWT.NONE, true);
		snmpProxySelector.setLabel(Messages.get().CreateNodeDialog_SNMPProxy);
		snmpProxySelector.setObjectClass(Node.class);
		snmpProxySelector.setObjectId(snmpProxy);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		snmpProxySelector.setLayoutData(gd);
		
      icmpProxySelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      icmpProxySelector.setLabel("Proxy for ICMP");
      icmpProxySelector.setObjectClass(Node.class);
      icmpProxySelector.setObjectId(icmpProxy);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      icmpProxySelector.setLayoutData(gd);
      
      sshProxySelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      sshProxySelector.setLabel("Proxy for SSH");
      sshProxySelector.setEmptySelectionName("<default>");
      sshProxySelector.setObjectClass(Node.class);
      sshProxySelector.setObjectId(sshProxy);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      sshProxySelector.setLayoutData(gd);
      
		if (session.isZoningEnabled())
		{
			zoneSelector = new ObjectSelector(dialogArea, SWT.NONE, false);
			zoneSelector.setLabel(Messages.get().CreateNodeDialog_Zone);
			zoneSelector.setObjectClass(Zone.class);
			Zone zone = ConsoleSharedData.getSession().findZone(zoneId);
			zoneSelector.setObjectId((zone != null) ? zone.getObjectId() : 0);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			gd.horizontalSpan = 2;
			zoneSelector.setLayoutData(gd);
		}
		
		checkCreateAnother = new Button(dialogArea, SWT.CHECK);
		checkCreateAnother.setText(Messages.get().CreateNodeDialog_ShowAgain);
		checkCreateAnother.setSelection(showAgain);
		
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
		if (!hostName.matches("^(([A-Za-z0-9\\-]+\\.)*[A-Za-z0-9\\-]+|[A-Fa-f0-9:]+)$")) //$NON-NLS-1$
		{
			MessageDialogHelper.openWarning(getShell(), Messages.get().CreateNodeDialog_Warning, 
			      String.format(Messages.get().CreateNodeDialog_WarningInvalidHostname, hostName));
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
		
		sshLogin = sshLoginField.getText().trim();
		sshPassword = sshPasswordField.getText();
		
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
		
		showAgain = checkCreateAnother.getSelection();
		super.okPressed();
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
    * @return the icmpProxy
    */
   public long getIcmpProxy()
   {
      return icmpProxy;
   }

   /**
    * @return the sshProxy
    */
   public long getSshProxy()
   {
      return sshProxy;
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

   /**
    * @return the sshLogin
    */
   public String getSshLogin()
   {
      return sshLogin;
   }

   /**
    * @return the sshPassword
    */
   public String getSshPassword()
   {
      return sshPassword;
   }

   /**
    * @return the showAgain
    */
   public boolean isShowAgain()
   {
      return showAgain;
   }
}
