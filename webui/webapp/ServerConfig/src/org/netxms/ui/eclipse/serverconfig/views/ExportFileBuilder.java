/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
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
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Template;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.ui.eclipse.epp.dialogs.RuleSelectionDialog;
import org.netxms.ui.eclipse.eventmanager.dialogs.EventSelectionDialog;
import org.netxms.ui.eclipse.filemanager.widgets.LocalFileSelector;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.dialogs.SelectSnmpTrapDialog;
import org.netxms.ui.eclipse.serverconfig.dialogs.helpers.TrapListLabelProvider;
import org.netxms.ui.eclipse.serverconfig.views.helpers.RuleComparator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.RuleLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.ObjectLabelComparator;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Export file builder
 */
public class ExportFileBuilder extends ViewPart implements ISaveablePart
{
	public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.ExportFileBuilder";

	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	private FormToolkit toolkit;
	private ScrolledForm form;
	private LocalFileSelector exportFile;
	private LabeledText description;
	private TableViewer templateViewer;
	private TableViewer eventViewer;
	private TableViewer trapViewer;
	private TableViewer ruleViewer;
	private Action actionSave;
	private Set<EventTemplate> events = new HashSet<EventTemplate>();
	private Set<Template> templates = new HashSet<Template>();
	private Set<SnmpTrap> traps = new HashSet<SnmpTrap>();
	private Set<EventProcessingPolicyRule> rules = new HashSet<EventProcessingPolicyRule>();
	private boolean modified = false;
	private List<SnmpTrap> snmpTrapCache = null;
	private List<EventProcessingPolicyRule> rulesCache = null;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		// Initiate loading of required plugins if they was not loaded yet
		try
		{
			Platform.getAdapterManager().loadAdapter(new EventTemplate(0), "org.eclipse.ui.model.IWorkbenchAdapter");
			Platform.getAdapterManager().loadAdapter(session.getTopLevelObjects()[0], "org.eclipse.ui.model.IWorkbenchAdapter");
		}
		catch(Exception e)
		{
		}
		
		toolkit = new FormToolkit(getSite().getShell().getDisplay());
		form = toolkit.createScrolledForm(parent);
		form.setText("Export Configuration");

		TableWrapLayout layout = new TableWrapLayout();
		layout.numColumns = 2;
		form.getBody().setLayout(layout);
		
		createFileSection();
		createTemplateSection();
		createEventSection();
		createTrapSection();
		createRuleSection();
		
		form.reflow(true);
		
		createActions();
		contributeToActionBars();
	}
	
	/**
	 * Create "File" section
	 */
	private void createFileSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		section.setText("Export File");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		td.colspan = 2;
		section.setLayoutData(td);
		
		Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		exportFile = new LocalFileSelector(clientArea, SWT.NONE, true);
		toolkit.adapt(exportFile);
		exportFile.setLabel("File name");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		exportFile.setLayoutData(gd);
		
		description = new LabeledText(clientArea, SWT.NONE);
		toolkit.adapt(description);
		description.setLabel("Description");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		description.setLayoutData(gd);
	}
	
	/**
	 * Create "Templates" section
	 */
	private void createTemplateSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		section.setText("Templates");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		templateViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
		toolkit.adapt(templateViewer.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 200;
		gd.verticalSpan = 2;
		templateViewer.getTable().setLayoutData(gd);
		templateViewer.setContentProvider(new ArrayContentProvider());
		templateViewer.setLabelProvider(new WorkbenchLabelProvider());
		templateViewer.setComparator(new ObjectLabelComparator((ILabelProvider)templateViewer.getLabelProvider()));
		templateViewer.getTable().setSortDirection(SWT.UP);

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
				addTemplates();
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
				removeTemplates();
			}
		});
	}

	/**
	 * Create "Events" section
	 */
	private void createEventSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		section.setText("Events");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		eventViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
		toolkit.adapt(eventViewer.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 200;
		gd.verticalSpan = 2;
		eventViewer.getTable().setLayoutData(gd);
		eventViewer.setContentProvider(new ArrayContentProvider());
		eventViewer.setLabelProvider(new WorkbenchLabelProvider());
		eventViewer.setComparator(new ObjectLabelComparator((ILabelProvider)eventViewer.getLabelProvider()));
		eventViewer.getTable().setSortDirection(SWT.UP);

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
				addEvents();
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
				removeEvents();
			}
		});
	}

	/**
	 * Create "SNMP Traps" section
	 */
	private void createTrapSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		section.setText("SNMP Traps");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		trapViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
		toolkit.adapt(trapViewer.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 200;
		gd.verticalSpan = 2;
		trapViewer.getTable().setLayoutData(gd);
		trapViewer.setContentProvider(new ArrayContentProvider());
		trapViewer.setLabelProvider(new TrapListLabelProvider());
		trapViewer.setComparator(new ObjectLabelComparator((ILabelProvider)eventViewer.getLabelProvider()));
		trapViewer.getTable().setSortDirection(SWT.UP);

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
				if (snmpTrapCache == null)
				{
					new ConsoleJob("Loading SNMP trap configuration", ExportFileBuilder.this, Activator.PLUGIN_ID, null) {
						@Override
						protected void runInternal(IProgressMonitor monitor) throws Exception
						{
							snmpTrapCache = session.getSnmpTrapsConfigurationSummary();
							runInUIThread(new Runnable() {
								@Override
								public void run()
								{
									addTraps();
								}
							});
						}

						@Override
						protected String getErrorMessage()
						{
							return "Cannot load SNMP trap configuration";
						}
					}.start();
				}
				else
				{
					addTraps();
				}
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
				removeTraps();
			}
		});
	}

	/**
	 * Create "Rules" section
	 */
	private void createRuleSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		section.setText("Rules");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		ruleViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
		toolkit.adapt(ruleViewer.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 200;
		gd.verticalSpan = 2;
		ruleViewer.getTable().setLayoutData(gd);
		ruleViewer.setContentProvider(new ArrayContentProvider());
		ruleViewer.setLabelProvider(new RuleLabelProvider());
		ruleViewer.setComparator(new RuleComparator());
		ruleViewer.getTable().setSortDirection(SWT.UP);

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
				if (rulesCache == null)
				{
					new ConsoleJob("Loading event processing policy", ExportFileBuilder.this, Activator.PLUGIN_ID, null) {
						@Override
						protected void runInternal(IProgressMonitor monitor) throws Exception
						{
							rulesCache = session.getEventProcessingPolicy().getRules();
							runInUIThread(new Runnable() {
								@Override
								public void run()
								{
									addRules();
								}
							});
						}

						@Override
						protected String getErrorMessage()
						{
							return "Cannot load event processing policy";
						}
					}.start();
				}
				else
				{
					addRules();
				}
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
				removeRules();
			}
		});
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

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		form.setFocus();
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

	/**
	 * Save settings
	 */
	private void save()
	{
		if (exportFile.getFile() == null)
		{
			MessageDialogHelper.openWarning(getSite().getShell(), "Warning", "Please enter valid file name to write exported configuration to!");
			return;
		}
		
		final long[] eventList = new long[events.size()];
		int i = 0;
		for(EventTemplate t : events)
			eventList[i++] = t.getCode();
		
		final long[] templateList = new long[templates.size()];
		i = 0;
		for(Template t : templates)
			templateList[i++] = t.getObjectId();
		
		final long[] trapList = new long[traps.size()];
		i = 0;
		for(SnmpTrap t : traps)
			trapList[i++] = t.getId();
		
		final UUID[] ruleList = new UUID[rules.size()];
		i = 0;
		for(EventProcessingPolicyRule r : rules)
			ruleList[i++] = r.getGuid();
		
		final String descriptionText = description.getText();
		
		new ConsoleJob("Exporting and saving configuration", this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				String xml = session.exportConfiguration(descriptionText, eventList, trapList, templateList, ruleList);
				OutputStreamWriter out = new OutputStreamWriter(new FileOutputStream(exportFile.getFile()), "UTF-8");
				try
				{
					out.write(xml);
				}
				finally
				{
					out.close();
				}
				runInUIThread(new Runnable() {
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
				return "Cannot export configuration";
			}
		}.start();
	}

	@Override
	public void doSave(IProgressMonitor monitor)
	{
		// TODO Auto-generated method stub
		
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
	 * Add events to list
	 */
	private void addEvents()
	{
		EventSelectionDialog dlg = new EventSelectionDialog(getSite().getShell());
		dlg.enableMultiSelection(true);
		if (dlg.open() == Window.OK)
		{
			for(EventTemplate t : dlg.getSelectedEvents())
				events.add(t);
			eventViewer.setInput(events.toArray());
			setModified();
		}
	}
	
	/**
	 * Remove events from list
	 */
	private void removeEvents()
	{
		IStructuredSelection selection = (IStructuredSelection)eventViewer.getSelection();
		if (selection.size() > 0)
		{
			for(Object o : selection.toList())
				events.remove(o);
			eventViewer.setInput(events.toArray());
			setModified();
		}
	}
	
	/**
	 * Add templates to list
	 */
	private void addTemplates()
	{
		final ObjectSelectionDialog dlg = new ObjectSelectionDialog(getSite().getShell(), null, ObjectSelectionDialog.createTemplateSelectionFilter());
		dlg.enableMultiSelection(true);
		if (dlg.open() == Window.OK)
		{
			final Set<Long> idList = new HashSet<Long>();
			for(AbstractObject o : dlg.getSelectedObjects(Template.class))
			{
				templates.add((Template)o);
				idList.add(o.getObjectId());
			}
			templateViewer.setInput(templates.toArray());
			setModified();
			new ConsoleJob("Resolving event dependencies", this, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					final Set<Long> eventCodes = new HashSet<Long>();
					for(Long id : idList)
					{
						long[] e = session.getDataCollectionEvents(id);
						for(long c : e)
						{
							if (c >= 100000)
								eventCodes.add(c);
						}
					}
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							events.addAll(session.findMultipleEventTemplates(eventCodes.toArray(new Long[eventCodes.size()])));
							eventViewer.setInput(events.toArray());
						}
					});
				}
				
				@Override
				protected String getErrorMessage()
				{
					return null;
				}
			}.start();
		}
	}
	
	/**
	 * Remove templates from list
	 */
	private void removeTemplates()
	{
		IStructuredSelection selection = (IStructuredSelection)templateViewer.getSelection();
		if (selection.size() > 0)
		{
			for(Object o : selection.toList())
				templates.remove(o);
			templateViewer.setInput(templates.toArray());
			setModified();
		}
	}
	
	/**
	 * Add traps to list
	 */
	private void addTraps()
	{
		SelectSnmpTrapDialog dlg = new SelectSnmpTrapDialog(getSite().getShell(), snmpTrapCache);
		if (dlg.open() == Window.OK)
		{
			final Set<Long> eventCodes = new HashSet<Long>();
			for(SnmpTrap t : dlg.getSelection())
			{
				traps.add(t);
				if (t.getEventCode() >= 100000)
				{
					eventCodes.add((long)t.getEventCode());
				}
			}
			trapViewer.setInput(traps.toArray());
			setModified();
			if (eventCodes.size() > 0)
			{
				events.addAll(session.findMultipleEventTemplates(eventCodes.toArray(new Long[eventCodes.size()])));
				eventViewer.setInput(events.toArray());
			};
		}
	}
	
	/**
	 * Remove traps from list
	 */
	private void removeTraps()
	{
		IStructuredSelection selection = (IStructuredSelection)trapViewer.getSelection();
		if (selection.size() > 0)
		{
			for(Object o : selection.toList())
				traps.remove(o);
			trapViewer.setInput(traps.toArray());
			setModified();
		}
	}

	/**
	 * Add rules to list
	 */
	private void addRules()
	{
		RuleSelectionDialog dlg = new RuleSelectionDialog(getSite().getShell(), rulesCache);
		if (dlg.open() == Window.OK)
		{
			final Set<Long> eventCodes = new HashSet<Long>();
			for(EventProcessingPolicyRule r : dlg.getSelectedRules())
			{
				rules.add(r);
				for(Long e : r.getEvents())
				{
					if (e >= 100000)
					{
						eventCodes.add(e);
					}
				}
			}
			ruleViewer.setInput(rules.toArray());
			setModified();
			if (eventCodes.size() > 0)
			{
				events.addAll(session.findMultipleEventTemplates(eventCodes.toArray(new Long[eventCodes.size()])));
				eventViewer.setInput(events.toArray());
			};
		}
	}
	
	/**
	 * Remove rules from list
	 */
	private void removeRules()
	{
		IStructuredSelection selection = (IStructuredSelection)ruleViewer.getSelection();
		if (selection.size() > 0)
		{
			for(Object o : selection.toList())
				rules.remove(o);
			ruleViewer.setInput(rules.toArray());
			setModified();
		}
	}
}
