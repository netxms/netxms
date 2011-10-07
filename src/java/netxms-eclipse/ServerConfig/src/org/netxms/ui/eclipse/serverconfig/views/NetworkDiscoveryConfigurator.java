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
package org.netxms.ui.eclipse.serverconfig.views;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.serverconfig.widgets.ScriptSelector;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Configurator for network discovery
 */
public class NetworkDiscoveryConfigurator extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.NetworkDiscoveryConfigurator";
	
	private FormToolkit toolkit;
	private ScrolledForm form;
	private Button radioDiscoveryOff;
	private Button radioDiscoveryPassive;
	private Button radioDiscoveryActive;
	private LabeledText defaultSnmpCommunity;
	private Button radioFilterOff;
	private Button radioFilterCustom;
	private Button radioFilterAuto;
	private ScriptSelector filterScript;
	private Button checkAgentOnly;
	private Button checkSnmpOnly;
	private Button checkRangeOnly;
	private TableViewer filterAddressList;
	private TableViewer activeDiscoveryAddressList;
	private TableViewer snmpCommunityList;
	private TableViewer snmpUsmCredList;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		toolkit = new FormToolkit(getSite().getShell().getDisplay());
		form = toolkit.createScrolledForm(parent);
		form.setText("Network Discovery Configuration");

		TableWrapLayout layout = new TableWrapLayout();
		layout.numColumns = 2;
		form.getBody().setLayout(layout);
		
		createGeneralSection();
		createFilterSection();
		createActiveDiscoverySection();
		createSubnetFilterSection();
		createSnmpCommunitySection();
		createSnmpUsmCredSection();
	}
	
	/**
	 * Create "General" section
	 */
	private void createGeneralSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("General");
		section.setDescription("General network discovery settings");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		radioDiscoveryOff = toolkit.createButton(clientArea, "&Disabled", SWT.RADIO);
		radioDiscoveryPassive = toolkit.createButton(clientArea, "&Passive only (using ARP and routing information)", SWT.RADIO);
		radioDiscoveryActive = toolkit.createButton(clientArea, "&Active and passive", SWT.RADIO);
		
		defaultSnmpCommunity = new LabeledText(clientArea, SWT.NONE);
		toolkit.adapt(defaultSnmpCommunity);
		defaultSnmpCommunity.setLabel("Default SNMP community string");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalIndent = 10;
		defaultSnmpCommunity.setLayoutData(gd);
	}
	
	/**
	 * Create "Filter" section
	 */
	private void createFilterSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("Filter");
		section.setDescription("Discovery filter");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		clientArea.setLayout(layout);
		section.setClient(clientArea);

		radioFilterOff = toolkit.createButton(clientArea, "&No filtering", SWT.RADIO);
		radioFilterCustom = toolkit.createButton(clientArea, "&Custom script)", SWT.RADIO);
		filterScript = new ScriptSelector(toolkit, clientArea, "");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalIndent = 20;
		filterScript.setLayoutData(gd);
		radioFilterAuto = toolkit.createButton(clientArea, "A&utomatically generated script with following rules", SWT.RADIO);
		
		checkAgentOnly = toolkit.createButton(clientArea, "Accept node if it has &NetXMS agent", SWT.CHECK);
		gd = new GridData();
		gd.horizontalIndent = 20;
		checkAgentOnly.setLayoutData(gd);
		
		checkSnmpOnly = toolkit.createButton(clientArea, "Accept node if it has &SNMP agent", SWT.CHECK);
		gd = new GridData();
		gd.horizontalIndent = 20;
		checkSnmpOnly.setLayoutData(gd);
		
		checkRangeOnly = toolkit.createButton(clientArea, "Accept node if it is within given &range or subnet", SWT.CHECK);
		gd = new GridData();
		gd.horizontalIndent = 20;
		checkRangeOnly.setLayoutData(gd);
	}
	
	/**
	 * Create "Active Discovery Targets" section
	 */
	private void createActiveDiscoverySection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("Active Discovery Targets");
		section.setDescription("Subnets and address ranges to be scanned during active discovery");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		activeDiscoveryAddressList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		toolkit.adapt(activeDiscoveryAddressList.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 100;
		activeDiscoveryAddressList.getTable().setLayoutData(gd);
	}
	
	/**
	 * Create "Address Filters" section
	 */
	private void createSubnetFilterSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("Address Filters");
		section.setDescription("Subnets and address ranges for \"match address\" filter");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		filterAddressList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		toolkit.adapt(filterAddressList.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 100;
		filterAddressList.getTable().setLayoutData(gd);
	}
	
	/**
	 * Create "SNMP Communities" section
	 */
	private void createSnmpCommunitySection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("SNMP Communities");
		section.setDescription("SNMP community strings used in the network");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		snmpCommunityList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		toolkit.adapt(snmpCommunityList.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 100;
		snmpCommunityList.getTable().setLayoutData(gd);
	}
	
	/**
	 * Create "Address Filters" section
	 */
	private void createSnmpUsmCredSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("SNMP USM Credentials");
		section.setDescription("SNMP USM credentials used in the network");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		snmpUsmCredList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		toolkit.adapt(snmpUsmCredList.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 100;
		snmpUsmCredList.getTable().setLayoutData(gd);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		form.setFocus();
	}
}
