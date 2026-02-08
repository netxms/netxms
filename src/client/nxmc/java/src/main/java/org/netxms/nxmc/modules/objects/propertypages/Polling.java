/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BusinessServicePrototype;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.DashboardTemplate;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.NetworkMap;
import org.netxms.client.objects.Template;
import org.netxms.client.objects.interfaces.PollingTarget;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Polling" property page for nodes
 */
public class Polling extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(Polling.class);

   private PollingTarget pollingTarget;
	private ObjectSelector pollerNode;
	private Button radioIfXTableDefault;
	private Button radioIfXTableEnable;
	private Button radioIfXTableDisable;
	private Button radioAgentCacheDefault;
   private Button radioAgentCacheOn;
   private Button radioAgentCacheOff;
   private LabeledSpinner pollCount;
	private List<Button> flagButtons = new ArrayList<Button>();
	private List<Integer> flagValues = new ArrayList<Integer>();

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public Polling(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(Polling.class).tr("Polling"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "polling";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 20;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof PollingTarget) && !(object instanceof Container) && !(object instanceof Dashboard) && !(object instanceof DashboardTemplate)
            && !(object instanceof Template) && !(object instanceof BusinessServicePrototype) && !(object instanceof NetworkMap);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);

      pollingTarget = (PollingTarget)object;

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      GridData gd = new GridData();

      /* poller node */
      if (pollingTarget.canHavePollerNode())
      {
         Group servicePollGroup = new Group(dialogArea, SWT.NONE);
         servicePollGroup.setText(i18n.tr("Network service polling"));
   		layout = new GridLayout();
   		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
   		layout.numColumns = 2;
   		servicePollGroup.setLayout(layout);
   		gd = new GridData();
   		gd.horizontalAlignment = SWT.FILL;
   		gd.grabExcessHorizontalSpace = true;
   		servicePollGroup.setLayoutData(gd);
   		
   		pollerNode = new ObjectSelector(servicePollGroup, SWT.NONE, true);
   		pollerNode.setLabel(i18n.tr("Poller node"));
   		pollerNode.setObjectClass(AbstractNode.class);
   		pollerNode.setEmptySelectionName(i18n.tr("<server>"));
         pollerNode.setObjectId(pollingTarget.getPollerNodeId());
   		gd = new GridData();
   		gd.horizontalAlignment = SWT.FILL;
   		gd.grabExcessHorizontalSpace = true;
   		pollerNode.setLayoutData(gd);
   		
   		Label label = new Label(servicePollGroup, SWT.WRAP);
   		label.setText(i18n.tr("All network services of this node will be polled from poller node specified here, if not overriden by network service settings."));
   		gd = new GridData();
   		gd.widthHint = 250;
   		label.setLayoutData(gd);
      }

      /* poll count */
      if (object instanceof AbstractNode)
      {
         pollCount = new LabeledSpinner(dialogArea, SWT.NONE);
         pollCount.setLabel("Required poll count for status change");
         pollCount.setSelection(((AbstractNode)object).getRequredPollCount());
      }

		/* options */
		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText(i18n.tr("Options"));
		layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		optionsGroup.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		optionsGroup.setLayoutData(gd);

      if (pollingTarget.canHaveAgent())
      {
		   addFlag(optionsGroup, AbstractNode.NF_DISABLE_NXCP, i18n.tr("Disable usage of NetXMS &agent for all polls"));
      }
      if (pollingTarget.canHaveInterfaces())
		{
   		addFlag(optionsGroup, AbstractNode.NF_DISABLE_SNMP, i18n.tr("Disable usage of &SNMP for all polls"));
   		addFlag(optionsGroup, AbstractNode.NF_DISABLE_ICMP, i18n.tr("Disable usage of &ICMP pings for status polling"));
         addFlag(optionsGroup, AbstractNode.NF_DISABLE_SSH, i18n.tr("Disable SS&H usage for all polls"));
         addFlag(optionsGroup, AbstractNode.NF_DISABLE_VNC, i18n.tr("Disable &VNC detection"));
         addFlag(optionsGroup, AbstractNode.NF_DISABLE_SMCLP_PROPERTIES, i18n.tr("Disable reading of SM-CLP available properties metadata"));
		}
      if (pollingTarget.canUseEtherNetIP())
      {
         addFlag(optionsGroup, AbstractNode.NF_DISABLE_ETHERNET_IP, i18n.tr("Disable usage of &EtherNet/IP for all polls"));
      }
      if (pollingTarget.canUseModbus())
      {
         addFlag(optionsGroup, AbstractNode.NF_DISABLE_MODBUS_TCP, i18n.tr("Disable usage of &Modbus for all polls"));
      }
		addFlag(optionsGroup, AbstractNode.DCF_DISABLE_STATUS_POLL, i18n.tr("Disable s&tatus polling"));
      if (pollingTarget.canHaveInterfaces())
      {
         addFlag(optionsGroup, AbstractNode.NF_DISABLE_8021X_STATUS_POLL, "Disable &802.1x port state checking during status poll");
      }
		addFlag(optionsGroup, AbstractNode.DCF_DISABLE_CONF_POLL, i18n.tr("Disable &configuration polling"));
      if (pollingTarget.canHaveInterfaces())
      {
   		addFlag(optionsGroup, AbstractNode.NF_DISABLE_ROUTE_POLL, i18n.tr("Disable &routing table polling"));
         addFlag(optionsGroup, AbstractNode.NF_DISABLE_TOPOLOGY_POLL, i18n.tr("Disable to&pology polling"));
   		addFlag(optionsGroup, AbstractNode.NF_DISABLE_DISCOVERY_POLL, i18n.tr("Disable network &discovery polling"));
      }
      if (pollingTarget instanceof DataCollectionTarget)
      {
         addFlag(optionsGroup, AbstractNode.DCF_DISABLE_DATA_COLLECT, i18n.tr("Disable data c&ollection"));
      }
      if (pollingTarget.canHaveAgent())
      {
         addFlag(optionsGroup, AbstractNode.NF_DISABLE_PERF_COUNT, "Disable reading of &Windows performance counters metadata");
      }

		/* use ifXTable */
      if (pollingTarget.canHaveInterfaces())
		{
   		Group ifXTableGroup = new Group(dialogArea, SWT.NONE);
   		ifXTableGroup.setText(i18n.tr("Use ifXTable for interface polling"));
   		layout = new GridLayout();
   		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
   		layout.numColumns = 3;
   		layout.makeColumnsEqualWidth = true;
   		ifXTableGroup.setLayout(layout);
   		gd = new GridData();
   		gd.horizontalAlignment = SWT.FILL;
   		gd.grabExcessHorizontalSpace = true;
   		ifXTableGroup.setLayoutData(gd);
   		
   		radioIfXTableDefault = new Button(ifXTableGroup, SWT.RADIO);
   		radioIfXTableDefault.setText(i18n.tr("De&fault"));
         radioIfXTableDefault.setSelection(pollingTarget.getIfXTablePolicy() == AbstractNode.IFXTABLE_DEFAULT);
   
   		radioIfXTableEnable = new Button(ifXTableGroup, SWT.RADIO);
   		radioIfXTableEnable.setText(i18n.tr("&Enable"));
         radioIfXTableEnable.setSelection(pollingTarget.getIfXTablePolicy() == AbstractNode.IFXTABLE_ENABLED);
   
   		radioIfXTableDisable = new Button(ifXTableGroup, SWT.RADIO);
   		radioIfXTableDisable.setText(i18n.tr("&Disable"));
         radioIfXTableDisable.setSelection(pollingTarget.getIfXTablePolicy() == AbstractNode.IFXTABLE_DISABLED);
		}

      /* agent cache */      
      if (pollingTarget.canHaveAgent())
      {
         Group agentCacheGroup = new Group(dialogArea, SWT.NONE);
         agentCacheGroup.setText(i18n.tr("Agent cache mode"));
         layout = new GridLayout();
         layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
         layout.numColumns = 3;
         layout.makeColumnsEqualWidth = true;
         agentCacheGroup.setLayout(layout);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         agentCacheGroup.setLayoutData(gd);
      
         radioAgentCacheDefault = new Button(agentCacheGroup, SWT.RADIO);
         radioAgentCacheDefault.setText(i18n.tr("De&fault"));
         radioAgentCacheDefault.setSelection(pollingTarget.getAgentCacheMode() == AgentCacheMode.DEFAULT);
   
         radioAgentCacheOn = new Button(agentCacheGroup, SWT.RADIO);
         radioAgentCacheOn.setText(i18n.tr("On"));
         radioAgentCacheOn.setSelection(pollingTarget.getAgentCacheMode() == AgentCacheMode.ON);
   
         radioAgentCacheOff = new Button(agentCacheGroup, SWT.RADIO);
         radioAgentCacheOff.setText(i18n.tr("Off"));
         radioAgentCacheOff.setSelection(pollingTarget.getAgentCacheMode() == AgentCacheMode.OFF);
      }
      
      return dialogArea;
	}

	/**
	 * Add checkbox for flag
	 * 
	 * @param value
	 * @param name
	 */
	private void addFlag(Composite parent, int value, String name)
	{
		final Button button = new Button(parent, SWT.CHECK);
		button.setText(name);
      button.setSelection((pollingTarget.getFlags() & value) != 0);
		flagButtons.add(button);
		flagValues.add(value);
	}
	
	/**
	 * Collect new node flags from checkboxes
	 *  
	 * @return new node flags
	 */
	private int collectNodeFlags()
	{
      int flags = pollingTarget.getFlags();
		for(int i = 0; i < flagButtons.size(); i++)
		{
			if (flagButtons.get(i).getSelection())
			{
				flags |= flagValues.get(i);
			}
			else
			{
				flags &= ~flagValues.get(i);
			}
		}
		return flags;
	}

   /**
    * Collect mask for flags being modified
    * 
    * @return
    */
   private int collectNodeFlagsMask()
   {
      int mask = 0;
      for(int i = 0; i < flagButtons.size(); i++)
      {
         mask |= flagValues.get(i);
      }
      return mask;
   }
	
	/**
	 * Collect ifXTabe usage policy from radio buttons
	 * 
	 * @return
	 */
	private int collectIfXTablePolicy()
	{
		if (radioIfXTableEnable.getSelection())
			return AbstractNode.IFXTABLE_ENABLED;
		if (radioIfXTableDisable.getSelection())
			return AbstractNode.IFXTABLE_DISABLED;
		return AbstractNode.IFXTABLE_DEFAULT;
	}

   /**
    * Collect agent cache mode from radio buttons
    * 
    * @return
    */
   private AgentCacheMode collectAgentCacheMode()
   {
      if (radioAgentCacheOn.getSelection())
         return AgentCacheMode.ON;
      if (radioAgentCacheOff.getSelection())
         return AgentCacheMode.OFF;
      return AgentCacheMode.DEFAULT;
   }

	/**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
	protected boolean applyChanges(final boolean isApply)
	{
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      if (pollingTarget.canHavePollerNode())
		   md.setPollerNode(pollerNode.getObjectId());
		md.setObjectFlags(collectNodeFlags(), collectNodeFlagsMask());
      if (pollingTarget.canHaveInterfaces())
		   md.setIfXTablePolicy(collectIfXTablePolicy());
      if (pollingTarget.canHaveAgent())
		   md.setAgentCacheMode(collectAgentCacheMode());
      if (pollingTarget instanceof AbstractNode)
         md.setRequiredPolls(pollCount.getSelection());

		if (isApply)
			setValid(false);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Update polling configuration for object {0}", object.getObjectName()), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot update polling configuration for object {0}", object.getObjectName());
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
					runInUIThread(() -> Polling.this.setValid(true));
			}
		}.start();
		return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      super.performDefaults();
      pollerNode.setObjectId(0);
      radioIfXTableDefault.setSelection(true);
      radioIfXTableDisable.setSelection(false);
      radioIfXTableEnable.setSelection(false);
      for(Button b : flagButtons)
         b.setSelection(false);
   }
}
