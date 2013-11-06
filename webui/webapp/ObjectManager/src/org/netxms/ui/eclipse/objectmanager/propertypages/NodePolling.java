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
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Polling" property page for nodes
 */
public class NodePolling extends PropertyPage
{
	private AbstractNode object;
	private ObjectSelector pollerNode;
	private Button radioDefault;
	private Button radioEnable;
	private Button radioDisable;
	private List<Button> flagButtons = new ArrayList<Button>();
	private List<Integer> flagValues = new ArrayList<Integer>();
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (AbstractNode)getElement().getAdapter(AbstractNode.class);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      /* poller node */
      Group servicePollGroup = new Group(dialogArea, SWT.NONE);
      servicePollGroup.setText("Network service polling");
		layout = new GridLayout();
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.numColumns = 2;
		servicePollGroup.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		servicePollGroup.setLayoutData(gd);
		
		pollerNode = new ObjectSelector(servicePollGroup, SWT.NONE, true);
		pollerNode.setLabel("Poller node");
		pollerNode.setObjectClass(AbstractNode.class);
		pollerNode.setEmptySelectionName("<server>");
		pollerNode.setObjectId(object.getPollerNodeId());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		pollerNode.setLayoutData(gd);
		
		Label label = new Label(servicePollGroup, SWT.WRAP);
		label.setText("All network services of this node will be polled from poller node specified here, if not overrided by network service settings.");
		gd = new GridData();
		gd.widthHint = 250;
		label.setLayoutData(gd);

		/* options */
		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText("Options");
		layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		optionsGroup.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		optionsGroup.setLayoutData(gd);
		
		addFlag(optionsGroup, AbstractNode.NF_DISABLE_NXCP, "Disable usage of NetXMS &agent for all polls");
		addFlag(optionsGroup, AbstractNode.NF_DISABLE_SNMP, "Disable usage of &SNMP for all polls");
		addFlag(optionsGroup, AbstractNode.NF_DISABLE_ICMP, "Disable usage of &ICMP pings for status polling");
		addFlag(optionsGroup, AbstractNode.NF_DISABLE_STATUS_POLL, "Disable s&tatus polling");
		addFlag(optionsGroup, AbstractNode.NF_DISABLE_CONF_POLL, "Disable &configuration polling");
		addFlag(optionsGroup, AbstractNode.NF_DISABLE_ROUTE_POLL, "Disable &routing table polling");
		addFlag(optionsGroup, AbstractNode.NF_DISABLE_TOPOLOGY_POLL, "Disable &topology polling");
		addFlag(optionsGroup, AbstractNode.NF_DISABLE_DISCOVERY_POLL, "Disable network &discovery polling");
		addFlag(optionsGroup, AbstractNode.NF_DISABLE_DATA_COLLECT, "Disable data c&ollection");
		
		/* use ifXTable */
		Group ifXTableGroup = new Group(dialogArea, SWT.NONE);
		ifXTableGroup.setText("Use ifXTable for interface polling");
		layout = new GridLayout();
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.numColumns = 3;
		layout.makeColumnsEqualWidth = true;
		ifXTableGroup.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		ifXTableGroup.setLayoutData(gd);
		
		radioDefault = new Button(ifXTableGroup, SWT.RADIO);
		radioDefault.setText("De&fault");
		radioDefault.setSelection(object.getIfXTablePolicy() == AbstractNode.IFXTABLE_DEFAULT);

		radioEnable = new Button(ifXTableGroup, SWT.RADIO);
		radioEnable.setText("&Enable");
		radioEnable.setSelection(object.getIfXTablePolicy() == AbstractNode.IFXTABLE_ENABLED);

		radioDisable = new Button(ifXTableGroup, SWT.RADIO);
		radioDisable.setText("&Disable");
		radioDisable.setSelection(object.getIfXTablePolicy() == AbstractNode.IFXTABLE_DISABLED);

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
		button.setSelection((object.getFlags() & value) != 0);
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
		int flags = object.getFlags();
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
	 * Collect ifXTabe usage policy from radio buttons
	 * 
	 * @return
	 */
	private int collectIfXTablePolicy()
	{
		if (radioEnable.getSelection())
			return AbstractNode.IFXTABLE_ENABLED;
		if (radioDisable.getSelection())
			return AbstractNode.IFXTABLE_DISABLED;
		return AbstractNode.IFXTABLE_DEFAULT;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		
		md.setPollerNode(pollerNode.getObjectId());
		md.setObjectFlags(collectNodeFlags());
		md.setIfXTablePolicy(collectIfXTablePolicy());
		
		if (isApply)
			setValid(false);
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Update node polling settings", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot update node polling settings";
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
							NodePolling.this.setValid(true);
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
