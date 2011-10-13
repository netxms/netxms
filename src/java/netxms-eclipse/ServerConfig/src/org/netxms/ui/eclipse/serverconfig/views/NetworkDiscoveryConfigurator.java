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

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.constants.NetworkDiscovery;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.DiscoveryConfig;
import org.netxms.ui.eclipse.serverconfig.widgets.ScriptSelector;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.StringComparator;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Configurator for network discovery
 */
public class NetworkDiscoveryConfigurator extends ViewPart implements ISaveablePart
{
	public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.NetworkDiscoveryConfigurator";

	private DiscoveryConfig config;
	private boolean modified = false;
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
	private Action actionSave;

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
		
		createActions();
		contributeToActionBars();
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionSave = new Action("&Save") {
			@Override
			public void run()
			{
				save();
			}
		};
		actionSave.setImageDescriptor(SharedIcons.SAVE);
	}
	
	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionSave);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionSave);
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
		
		final SelectionListener listener = new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				setModified();
				if (radioDiscoveryOff.getSelection())
				{
					config.setEnabled(false);
				}
				else
				{
					config.setEnabled(true);
					config.setActive(radioDiscoveryActive.getSelection());
				}
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		};
		
		radioDiscoveryOff = toolkit.createButton(clientArea, "&Disabled", SWT.RADIO);
		radioDiscoveryOff.addSelectionListener(listener);
		radioDiscoveryPassive = toolkit.createButton(clientArea, "&Passive only (using ARP and routing information)", SWT.RADIO);
		radioDiscoveryPassive.addSelectionListener(listener);
		radioDiscoveryActive = toolkit.createButton(clientArea, "&Active and passive", SWT.RADIO);
		radioDiscoveryActive.addSelectionListener(listener);
		
		defaultSnmpCommunity = new LabeledText(clientArea, SWT.NONE);
		toolkit.adapt(defaultSnmpCommunity);
		defaultSnmpCommunity.setLabel("Default SNMP community string");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalIndent = 10;
		defaultSnmpCommunity.setLayoutData(gd);
		defaultSnmpCommunity.getTextControl().addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				setModified();
				config.setDefaultCommunity(defaultSnmpCommunity.getText());
			}
		});
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

		final SelectionListener radioButtonListener = new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				updateElementEnablement();
				setModified();
				if (radioFilterOff.getSelection())
				{
					config.setFilter(NetworkDiscovery.FILTER_NONE);
				}
				else if (radioFilterAuto.getSelection())
				{
					config.setFilter(NetworkDiscovery.FILTER_AUTO);
				}
				else
				{
					config.setFilter(filterScript.getScriptName());
				}
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		};
		
		final SelectionListener checkBoxListener = new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				setModified();
				int flags = 0;
				if (checkAgentOnly.getSelection())
					flags |= NetworkDiscovery.FILTER_ALLOW_AGENT;
				if (checkSnmpOnly.getSelection())
					flags |= NetworkDiscovery.FILTER_ALLOW_SNMP;
				if (checkRangeOnly.getSelection())
					flags |= NetworkDiscovery.FILTER_LIMIT_BY_RANGE;
				config.setFilterFlags(flags);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		};
		
		radioFilterOff = toolkit.createButton(clientArea, "&No filtering", SWT.RADIO);
		radioFilterOff.addSelectionListener(radioButtonListener);
		radioFilterCustom = toolkit.createButton(clientArea, "&Custom script)", SWT.RADIO);
		radioFilterCustom.addSelectionListener(radioButtonListener);
		filterScript = new ScriptSelector(toolkit, clientArea, "");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalIndent = 20;
		filterScript.setLayoutData(gd);
		filterScript.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				if (radioFilterCustom.getSelection())
				{
					setModified();
					config.setFilter(filterScript.getScriptName());
				}
			}
		});
		
		radioFilterAuto = toolkit.createButton(clientArea, "A&utomatically generated script with following rules", SWT.RADIO);
		radioFilterAuto.addSelectionListener(radioButtonListener);
		
		checkAgentOnly = toolkit.createButton(clientArea, "Accept node if it has &NetXMS agent", SWT.CHECK);
		checkAgentOnly.addSelectionListener(checkBoxListener);
		gd = new GridData();
		gd.horizontalIndent = 20;
		checkAgentOnly.setLayoutData(gd);
		
		checkSnmpOnly = toolkit.createButton(clientArea, "Accept node if it has &SNMP agent", SWT.CHECK);
		checkSnmpOnly.addSelectionListener(checkBoxListener);
		gd = new GridData();
		gd.horizontalIndent = 20;
		checkSnmpOnly.setLayoutData(gd);
		
		checkRangeOnly = toolkit.createButton(clientArea, "Accept node if it is within given &range or subnet", SWT.CHECK);
		checkRangeOnly.addSelectionListener(checkBoxListener);
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
		layout.numColumns = 2;
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		activeDiscoveryAddressList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		toolkit.adapt(activeDiscoveryAddressList.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalSpan = 2;
		gd.heightHint = 100;
		activeDiscoveryAddressList.getTable().setLayoutData(gd);
		
		final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkAdd.setText("Add...");
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkAdd.setLayoutData(gd);
		linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
			}
		});
		
		final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkRemove.setText("Remove");
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkRemove.setLayoutData(gd);
		linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
			}
		});
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
		layout.numColumns = 2;
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		filterAddressList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		toolkit.adapt(filterAddressList.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalSpan = 2;
		gd.heightHint = 100;
		filterAddressList.getTable().setLayoutData(gd);
		
		final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkAdd.setText("Add...");
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkAdd.setLayoutData(gd);
		linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
			}
		});
		
		final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkRemove.setText("Remove");
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkRemove.setLayoutData(gd);
		linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
			}
		});

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
		layout.numColumns = 2;
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		snmpCommunityList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		toolkit.adapt(snmpCommunityList.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalSpan = 2;
		gd.heightHint = 100;
		snmpCommunityList.getTable().setLayoutData(gd);
		snmpCommunityList.getTable().setSortDirection(SWT.UP);
		snmpCommunityList.setContentProvider(new ArrayContentProvider());
		snmpCommunityList.setComparator(new StringComparator());
		
		final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkAdd.setText("Add...");
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkAdd.setLayoutData(gd);
		linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addCommunity();
			}
		});
		
		final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkRemove.setText("Remove");
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkRemove.setLayoutData(gd);
		linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				removeCommunity();
			}
		});
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
		layout.numColumns = 2;
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		snmpUsmCredList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		toolkit.adapt(snmpUsmCredList.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalSpan = 2;
		gd.heightHint = 100;
		snmpUsmCredList.getTable().setLayoutData(gd);

		final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkAdd.setText("Add...");
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkAdd.setLayoutData(gd);
		linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
			}
		});
		
		final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkRemove.setText("Remove");
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkRemove.setLayoutData(gd);
		linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		form.setFocus();
	}
	
	/**
	 * @param config
	 */
	public void setConfig(DiscoveryConfig config)
	{
		this.config = config;
		
		radioDiscoveryOff.setSelection(!config.isEnabled());
		radioDiscoveryPassive.setSelection(config.isEnabled() && !config.isActive());
		radioDiscoveryActive.setSelection(config.isEnabled() && config.isActive());
		
		defaultSnmpCommunity.setText(config.getDefaultCommunity());
		
		if (config.getFilter().equalsIgnoreCase(NetworkDiscovery.FILTER_NONE) || config.getFilter().isEmpty())
		{
			radioFilterOff.setSelection(true);
		}
		else if (config.getFilter().equalsIgnoreCase(NetworkDiscovery.FILTER_AUTO))
		{
			radioFilterAuto.setSelection(true);
		}
		else
		{
			radioFilterCustom.setSelection(true);
			filterScript.setScriptName(config.getFilter());
		}
		
		checkAgentOnly.setSelection((config.getFilterFlags() & NetworkDiscovery.FILTER_ALLOW_AGENT) != 0);
		checkSnmpOnly.setSelection((config.getFilterFlags() & NetworkDiscovery.FILTER_ALLOW_SNMP) != 0);
		checkRangeOnly.setSelection((config.getFilterFlags() & NetworkDiscovery.FILTER_LIMIT_BY_RANGE) != 0);
		
		snmpCommunityList.setInput(config.getCommunities().toArray());

		updateElementEnablement();
		modified = false;
		firePropertyChange(PROP_DIRTY);
	}
	
	/**
	 * Update enabled state of elements
	 */
	private void updateElementEnablement()
	{
		// Filter section
		if (radioFilterOff.getSelection())
		{
			filterScript.setEnabled(false);
			checkAgentOnly.setEnabled(false);
			checkSnmpOnly.setEnabled(false);
			checkRangeOnly.setEnabled(false);
		}
		else if (radioFilterAuto.getSelection())
		{
			filterScript.setEnabled(false);
			checkAgentOnly.setEnabled(true);
			checkSnmpOnly.setEnabled(true);
			checkRangeOnly.setEnabled(true);
		}
		if (radioFilterCustom.getSelection())
		{
			filterScript.setEnabled(true);
			checkAgentOnly.setEnabled(false);
			checkSnmpOnly.setEnabled(false);
			checkRangeOnly.setEnabled(false);
		}
	}
	
	/**
	 * Mark view as modified
	 */
	private void setModified()
	{
		if (!modified)
		{
			modified = true;
			firePropertyChange(PROP_DIRTY);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
	 */
	@Override
	public void doSave(IProgressMonitor monitor)
	{
		try
		{
			config.save();
		}
		catch(Exception e)
		{
			MessageDialog.openError(getSite().getShell(), "Error", "Cannot save network discovery configuration: " + e.getLocalizedMessage());
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#doSaveAs()
	 */
	@Override
	public void doSaveAs()
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#isDirty()
	 */
	@Override
	public boolean isDirty()
	{
		return modified;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
	 */
	@Override
	public boolean isSaveAsAllowed()
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#isSaveOnCloseNeeded()
	 */
	@Override
	public boolean isSaveOnCloseNeeded()
	{
		return modified;
	}
	
	/**
	 * Save settings
	 */
	private void save()
	{
		new ConsoleJob("Saving network discovery configuration", this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				config.save();
				getSite().getShell().getDisplay().asyncExec(new Runnable() {
					@Override
					public void run()
					{
						modified = false;
						firePropertyChange(PROP_DIRTY);
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Cannot save network discovery configuration";
			}
		}.start();
	}
	
	/**
	 * Add SNMP community to the list
	 */
	private void addCommunity()
	{
		InputDialog dlg = new InputDialog(getSite().getShell(), "Add SNMP Community", 
				"Please enter SNMP community string", "", null);
		if (dlg.open() == Window.OK)
		{
			String s = dlg.getValue();
			final List<String> list = config.getCommunities();
			if (!list.contains(s))
			{
				list.add(s);
				snmpCommunityList.setInput(list.toArray());
				setModified();
			}
		}
	}
	
	/**
	 * Remove selected SNMP communities
	 */
	private void removeCommunity()
	{
		final List<String> list = config.getCommunities();
		IStructuredSelection selection = (IStructuredSelection)snmpCommunityList.getSelection();
		if (selection.size() > 0)
		{
			for(Object o : selection.toList())
			{
				list.remove(o);
			}
			snmpCommunityList.setInput(list.toArray());
			setModified();
		}
	}
}
