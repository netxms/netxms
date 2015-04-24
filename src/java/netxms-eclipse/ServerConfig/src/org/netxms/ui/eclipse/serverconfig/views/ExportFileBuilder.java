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
package org.netxms.ui.eclipse.serverconfig.views;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.Script;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Template;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.epp.dialogs.RuleSelectionDialog;
import org.netxms.ui.eclipse.eventmanager.dialogs.EventSelectionDialog;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.dialogs.SelectScriptDialog;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.serverconfig.dialogs.ObjectToolSelectionDialog;
import org.netxms.ui.eclipse.serverconfig.dialogs.SelectSnmpTrapDialog;
import org.netxms.ui.eclipse.serverconfig.dialogs.helpers.TrapListLabelProvider;
import org.netxms.ui.eclipse.serverconfig.views.helpers.RuleComparator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.RuleLabelProvider;
import org.netxms.ui.eclipse.serverconfig.views.helpers.ScriptComparator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.ScriptLabelProvider;
import org.netxms.ui.eclipse.serverconfig.views.helpers.SummaryTablesComparator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.SummaryTablesLabelProvider;
import org.netxms.ui.eclipse.serverconfig.views.helpers.ToolComparator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.ToolLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ObjectLabelComparator;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Export file builder
 */
public class ExportFileBuilder extends ViewPart implements ISaveablePart
{
	public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.ExportFileBuilder"; //$NON-NLS-1$

	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	private FormToolkit toolkit;
	private ScrolledForm form;
	private LabeledText description;
	private TableViewer templateViewer;
	private TableViewer eventViewer;
	private TableViewer trapViewer;
	private TableViewer ruleViewer;
   private TableViewer scriptViewer;
   private TableViewer toolsViewer;
   private TableViewer summaryTableViewer;
	private Action actionSave;
	private Map<Long, EventTemplate> events = new HashMap<Long, EventTemplate>();
	private Map<Long, Template> templates = new HashMap<Long, Template>();
	private Map<Long, SnmpTrap> traps = new HashMap<Long, SnmpTrap>();
	private Map<UUID, EventProcessingPolicyRule> rules = new HashMap<UUID, EventProcessingPolicyRule>();
	private Map<Long, Script> scripts = new HashMap<Long, Script>();
	private Map<Long, ObjectTool> tools = new HashMap<Long, ObjectTool>();
   private Map<Integer, DciSummaryTableDescriptor> summaryTables = new HashMap<Integer, DciSummaryTableDescriptor>();
	private boolean modified = false;
	private List<SnmpTrap> snmpTrapCache = null;
	private List<EventProcessingPolicyRule> rulesCache = null;
	private String exportFileName = null;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		// Initiate loading of required plugins if they was not loaded yet
		try
		{
			Platform.getAdapterManager().loadAdapter(new EventTemplate(0), "org.eclipse.ui.model.IWorkbenchAdapter"); //$NON-NLS-1$
			Platform.getAdapterManager().loadAdapter(session.getTopLevelObjects()[0], "org.eclipse.ui.model.IWorkbenchAdapter"); //$NON-NLS-1$
		}
		catch(Exception e)
		{
		}
		
		toolkit = new FormToolkit(getSite().getShell().getDisplay());
		form = toolkit.createScrolledForm(parent);
		form.setText(Messages.get().ExportFileBuilder_FormTitle);

		TableWrapLayout layout = new TableWrapLayout();
		layout.numColumns = 3;
		form.getBody().setLayout(layout);
		
		createFileSection();
		createTemplateSection();
		createEventSection();
		createTrapSection();
		createRuleSection();
		createScriptSection();
		createToolsSection();
      createSummaryTablesSection();
		
		form.reflow(true);
		
		activateContext();
		createActions();
		contributeToActionBars();
	}
	
   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.serverconfig.context.ExportFileBuilder"); //$NON-NLS-1$
      }
   }
	
	/**
	 * Create "File" section
	 */
	private void createFileSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		section.setText(Messages.get().ExportFileBuilder_SectionFile);
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
		
		description = new LabeledText(clientArea, SWT.NONE, SWT.MULTI | SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL);
		toolkit.adapt(description);
		description.setLabel(Messages.get().ExportFileBuilder_Description);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 200;
		description.setLayoutData(gd);
	}
	
	/**
	 * Create "Templates" section
	 */
	private void createTemplateSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		section.setText(Messages.get().ExportFileBuilder_SectionTemplates);
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
		linkAdd.setText(Messages.get().ExportFileBuilder_Add);
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
		linkRemove.setText(Messages.get().ExportFileBuilder_Remove);
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkRemove.setLayoutData(gd);
		linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
			   removeObjects(templateViewer, templates);
			}
		});
	}

	/**
	 * Create "Events" section
	 */
	private void createEventSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		section.setText(Messages.get().ExportFileBuilder_SectionEvents);
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
		linkAdd.setText(Messages.get().ExportFileBuilder_Add);
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
		linkRemove.setText(Messages.get().ExportFileBuilder_Remove);
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkRemove.setLayoutData(gd);
		linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
		      removeObjects(eventViewer, events);
			}
		});
	}

	/**
	 * Create "SNMP Traps" section
	 */
	private void createTrapSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		section.setText(Messages.get().ExportFileBuilder_SectionTraps);
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
		linkAdd.setText(Messages.get().ExportFileBuilder_Add);
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
					new ConsoleJob(Messages.get().ExportFileBuilder_TrapsLoadJobName, ExportFileBuilder.this, Activator.PLUGIN_ID, null) {
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
							return Messages.get().ExportFileBuilder_TrapsLoadJobError;
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
		linkRemove.setText(Messages.get().ExportFileBuilder_Remove);
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkRemove.setLayoutData(gd);
		linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
		      removeObjects(trapViewer, traps);
			}
		});
	}

	/**
	 * Create "Rules" section
	 */
	private void createRuleSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		section.setText(Messages.get().ExportFileBuilder_SectionRules);
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
		linkAdd.setText(Messages.get().ExportFileBuilder_Add);
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
					new ConsoleJob(Messages.get().ExportFileBuilder_EPPLoadJobName, ExportFileBuilder.this, Activator.PLUGIN_ID, null) {
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
							return Messages.get().ExportFileBuilder_EPPLoadJobError;
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
		linkRemove.setText(Messages.get().ExportFileBuilder_Remove);
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkRemove.setLayoutData(gd);
		linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
		      removeObjects(ruleViewer, rules);
			}
		});
	}

   /**
    * Create "Scripts" section
    */
   private void createScriptSection()
   {
      Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
      section.setText("Scripts");
      TableWrapData td = new TableWrapData();
      td.align = TableWrapData.FILL;
      td.grabHorizontal = true;
      section.setLayoutData(td);
      
      Composite clientArea = toolkit.createComposite(section);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      section.setClient(clientArea);
      
      scriptViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
      toolkit.adapt(ruleViewer.getTable());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 200;
      gd.verticalSpan = 2;
      scriptViewer.getTable().setLayoutData(gd);
      scriptViewer.setContentProvider(new ArrayContentProvider());
      scriptViewer.setLabelProvider(new ScriptLabelProvider());
      scriptViewer.setComparator(new ScriptComparator());
      scriptViewer.getTable().setSortDirection(SWT.UP);

      final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(Messages.get().ExportFileBuilder_Add);
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addScript();
         }
      });
      
      final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(Messages.get().ExportFileBuilder_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkRemove.setLayoutData(gd);
      linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            removeObjects(scriptViewer, scripts);
         }
      });
   }

   /**
    * Create "Object Tools" section
    */
   private void createToolsSection()
   {
      Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
      section.setText("Object Tools");
      TableWrapData td = new TableWrapData();
      td.align = TableWrapData.FILL;
      td.grabHorizontal = true;
      section.setLayoutData(td);
      
      Composite clientArea = toolkit.createComposite(section);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      section.setClient(clientArea);
      
      toolsViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
      toolkit.adapt(ruleViewer.getTable());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 200;
      gd.verticalSpan = 2;
      toolsViewer.getTable().setLayoutData(gd);
      toolsViewer.setContentProvider(new ArrayContentProvider());
      toolsViewer.setLabelProvider(new ToolLabelProvider());
      toolsViewer.setComparator(new ToolComparator());
      toolsViewer.getTable().setSortDirection(SWT.UP);

      final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(Messages.get().ExportFileBuilder_Add);
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addObjectTools();
         }
      });
      
      final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(Messages.get().ExportFileBuilder_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkRemove.setLayoutData(gd);
      linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            removeObjects(toolsViewer, tools);
         }
      });
   }

   /**
    * Create "Summary Tables" section
    */
   private void createSummaryTablesSection()
   {
      Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
      section.setText("DCI Summary Tables");
      TableWrapData td = new TableWrapData();
      td.align = TableWrapData.FILL;
      td.grabHorizontal = true;
      section.setLayoutData(td);
      
      Composite clientArea = toolkit.createComposite(section);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      section.setClient(clientArea);
      
      summaryTableViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
      toolkit.adapt(ruleViewer.getTable());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 200;
      gd.verticalSpan = 2;
      summaryTableViewer.getTable().setLayoutData(gd);
      summaryTableViewer.setContentProvider(new ArrayContentProvider());
      summaryTableViewer.setLabelProvider(new SummaryTablesLabelProvider());
      summaryTableViewer.setComparator(new SummaryTablesComparator());
      summaryTableViewer.getTable().setSortDirection(SWT.UP);

      final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(Messages.get().ExportFileBuilder_Add);
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
      linkRemove.setText(Messages.get().ExportFileBuilder_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkRemove.setLayoutData(gd);
      linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            removeObjects(summaryTableViewer, summaryTables);
         }
      });
   }

	/**
	 * Create actions
	 */
	private void createActions()
	{
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      
		actionSave = new Action(Messages.get().ExportFileBuilder_Save, SharedIcons.SAVE) {
			@Override
			public void run()
			{
				save();
			}
		};
		actionSave.setActionDefinitionId("org.netxms.ui.eclipse.serverconfig.commands.save_exported_config"); //$NON-NLS-1$
      handlerService.activateHandler(actionSave.getActionDefinitionId(), new ActionHandler(actionSave));
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
		final long[] eventList = new long[events.size()];
		int i = 0;
		for(EventTemplate t : events.values())
			eventList[i++] = t.getCode();
		
		final long[] templateList = new long[templates.size()];
		i = 0;
		for(Template t : templates.values())
			templateList[i++] = t.getObjectId();
		
		final long[] trapList = new long[traps.size()];
		i = 0;
		for(SnmpTrap t : traps.values())
			trapList[i++] = t.getId();
		
		final UUID[] ruleList = new UUID[rules.size()];
		i = 0;
		for(EventProcessingPolicyRule r : rules.values())
			ruleList[i++] = r.getGuid();
		
      final long[] scriptList = new long[scripts.size()];
      i = 0;
      for(Script s : scripts.values())
         scriptList[i++] = s.getId();
      
      final long[] toolList = new long[tools.size()];
      i = 0;
      for(ObjectTool t : tools.values())
         toolList[i++] = t.getId();
      
      final long[] summaryTableList = new long[summaryTables.size()];
      i = 0;
      for(DciSummaryTableDescriptor t : summaryTables.values())
         summaryTableList[i++] = t.getId();
      
		final String descriptionText = description.getText();
		
		new ConsoleJob(Messages.get().ExportFileBuilder_ExportJobName, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final String xml = session.exportConfiguration(descriptionText, eventList, trapList, templateList, ruleList, scriptList, toolList, summaryTableList);
				runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  FileDialog dlg = new FileDialog(getSite().getShell(), SWT.SAVE);
                  dlg.setFilterExtensions(new String[] { "*.xml", "*.*" }); //$NON-NLS-1$ //$NON-NLS-2$
                  dlg.setFilterNames(new String[] { Messages.get().ConfigurationImportDialog_FileTypeXML, Messages.get().ConfigurationImportDialog_FileTypeAll });
                  dlg.setOverwrite(true);
                  dlg.setFileName(exportFileName);
                  final String fileName = dlg.open();
                  if (fileName != null)
                  {
                     exportFileName = fileName;
                     new ConsoleJob("Save exported configuration", ExportFileBuilder.this, Activator.PLUGIN_ID, null) {
                        @Override
                        protected void runInternal(IProgressMonitor monitor) throws Exception
                        {
                           OutputStreamWriter out = new OutputStreamWriter(new FileOutputStream(fileName), "UTF-8"); //$NON-NLS-1$
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
                           return "Cannot save exported configuration to file";
                        }
                     }.start();
                  }
               }
            });
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().ExportFileBuilder_ExportJobError;
			}
		}.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
	 */
	@Override
	public void doSave(IProgressMonitor monitor)
	{
	   getSite().getShell().getDisplay().syncExec(new Runnable() {
         @Override
         public void run()
         {
            save();
         }
      });
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
	 * @param o
	 * @return
	 */
	private static Object keyFromObject(Object o)
	{
	   if (o instanceof Template)
	      return ((Template)o).getObjectId();
	   if (o instanceof EventTemplate)
	      return ((EventTemplate)o).getCode();
	   if (o instanceof SnmpTrap)
	      return ((SnmpTrap)o).getId();
	   if (o instanceof Script)
	      return ((Script)o).getId();
	   if (o instanceof ObjectTool)
	      return ((ObjectTool)o).getId();
	   if (o instanceof EventProcessingPolicyRule)
	      return ((EventProcessingPolicyRule)o).getGuid();
	   if (o instanceof DciSummaryTableDescriptor)
	      return ((DciSummaryTableDescriptor)o).getId();
	   return 0L;
	}
	
	/**
	 * Remove objects from list
	 * 
	 * @param viewer
	 * @param objects
	 */
	private void removeObjects(TableViewer viewer, Map<?, ?> objects)
	{
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
            objects.remove(keyFromObject(o));
         viewer.setInput(objects.values().toArray());
         setModified();
      }
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
				events.put(t.getCode(), t);
			eventViewer.setInput(events.values().toArray());
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
				templates.put(((Template)o).getObjectId(), (Template)o);
				idList.add(o.getObjectId());
			}
			templateViewer.setInput(templates.values().toArray());
			setModified();
			new ConsoleJob(Messages.get().ExportFileBuilder_ResolveJobName, this, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					final Set<Long> eventCodes = new HashSet<Long>();
					final Map<Long, Script> scriptList = new HashMap<Long, Script>();
					for(Long id : idList)
					{
						long[] e = session.getDataCollectionEvents(id);
						for(long c : e)
						{
							if (c >= 100000)
								eventCodes.add(c);
						}
						
						for(Script s : session.getDataCollectionScripts(id))
						   scriptList.put(s.getId(), s);
					}
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
						   for(EventTemplate e : session.findMultipleEventTemplates(eventCodes.toArray(new Long[eventCodes.size()])))
						   {
						      events.put(e.getCode(), e);
						   }
							eventViewer.setInput(events.values().toArray());
							
							scripts.putAll(scriptList);
							scriptViewer.setInput(scripts.values().toArray());
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
				traps.put(t.getId(), t);
				if (t.getEventCode() >= 100000)
				{
					eventCodes.add((long)t.getEventCode());
				}
			}
			trapViewer.setInput(traps.values().toArray());
			setModified();
			if (eventCodes.size() > 0)
			{
				for(EventTemplate e : session.findMultipleEventTemplates(eventCodes.toArray(new Long[eventCodes.size()])))
				{
				   events.put(e.getCode(), e);
				}
				eventViewer.setInput(events.values().toArray());
			};
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
				rules.put(r.getGuid(), r);
				for(Long e : r.getEvents())
				{
					if (e >= 100000)
					{
						eventCodes.add(e);
					}
				}
			}
			ruleViewer.setInput(rules.values().toArray());
			setModified();
			if (eventCodes.size() > 0)
			{
				for(EventTemplate e : session.findMultipleEventTemplates(eventCodes.toArray(new Long[eventCodes.size()])))
				{
				   events.put(e.getCode(), e);
				}
				eventViewer.setInput(events.values().toArray());
			};
		}
	}
   
   /**
    * Add script to list
    */
   private void addScript()
   {
      SelectScriptDialog dlg = new SelectScriptDialog(getSite().getShell());
      if (dlg.open() == Window.OK)
      {
         Script s = dlg.getScript();
         scripts.put(s.getId(), s);
         scriptViewer.setInput(scripts.values().toArray());
         setModified();
      }
   }
   
   /**
    * Add oject tools to list
    */
   private void addObjectTools()
   {
      ObjectToolSelectionDialog dlg = new ObjectToolSelectionDialog(getSite().getShell());
      if (dlg.open() == Window.OK)
      {
         for(ObjectTool t : dlg.getSelection())
            tools.put(t.getId(), t);
         toolsViewer.setInput(tools.values().toArray());
         setModified();
      }
   }
}
