/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.views;

import java.io.File;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.UUID;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.AgentPolicy;
import org.netxms.client.NXCSession;
import org.netxms.client.Script;
import org.netxms.client.ServerAction;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.client.datacollection.WebServiceDefinition;
import org.netxms.client.events.ActionExecutionConfiguration;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Template;
import org.netxms.client.objects.TemplateGroup;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.Section;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.actions.dialogs.ActionSelectionDialog;
import org.netxms.nxmc.modules.actions.views.helpers.ActionComparator;
import org.netxms.nxmc.modules.actions.views.helpers.ActionLabelProvider;
import org.netxms.nxmc.modules.assetmanagement.dialogs.SelectAssetAttributeDlg;
import org.netxms.nxmc.modules.datacollection.dialogs.SelectWebServiceDlg;
import org.netxms.nxmc.modules.events.dialogs.EventSelectionDialog;
import org.netxms.nxmc.modules.events.dialogs.RuleSelectionDialog;
import org.netxms.nxmc.modules.events.widgets.helpers.BaseEventTemplateLabelProvider;
import org.netxms.nxmc.modules.nxsl.dialogs.SelectScriptDialog;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.helpers.DecoratingObjectLabelProvider;
import org.netxms.nxmc.modules.serverconfig.dialogs.ObjectToolSelectionDialog;
import org.netxms.nxmc.modules.serverconfig.dialogs.SelectSnmpTrapDialog;
import org.netxms.nxmc.modules.serverconfig.dialogs.SummaryTableSelectionDialog;
import org.netxms.nxmc.modules.serverconfig.dialogs.helpers.TrapListLabelProvider;
import org.netxms.nxmc.modules.serverconfig.views.helpers.RuleComparator;
import org.netxms.nxmc.modules.serverconfig.views.helpers.RuleLabelProvider;
import org.netxms.nxmc.modules.serverconfig.views.helpers.ScriptComparator;
import org.netxms.nxmc.modules.serverconfig.views.helpers.ScriptLabelProvider;
import org.netxms.nxmc.modules.serverconfig.views.helpers.SummaryTablesComparator;
import org.netxms.nxmc.modules.serverconfig.views.helpers.SummaryTablesLabelProvider;
import org.netxms.nxmc.modules.serverconfig.views.helpers.ToolComparator;
import org.netxms.nxmc.modules.serverconfig.views.helpers.ToolLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.ElementLabelComparator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Export file builder
 */
public class ExportFileBuilder extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(ExportFileBuilder.class);
   private static final Pattern SCRIPT_PATTERN = Pattern.compile("(?<!%)%\\[([a-zA-Z0-9:_]+)[./]?.*?\\]");

	private NXCSession session = Registry.getSession();
	private Composite content;
   private Text description;
   private TableViewer actionViewer;
   private TableViewer eventViewer;
   private TableViewer ruleViewer;
   private TableViewer scriptViewer;
   private TableViewer summaryTableViewer;
	private TableViewer templateViewer;
   private TableViewer toolsViewer;
	private TableViewer trapViewer;
   private TableViewer webServiceViewer;
   private TableViewer assetAttributeViewer;
	private Action actionExport;
	private Action actionClear;
   private Map<Long, ServerAction> actions = new HashMap<>();
   private Map<Integer, EventTemplate> events = new HashMap<>();
   private Map<UUID, EventProcessingPolicyRule> rules = new HashMap<>();
   private Map<Long, Script> scripts = new HashMap<Long, Script>();
   private Map<Integer, DciSummaryTableDescriptor> summaryTables = new HashMap<>();
	private Map<Long, Template> templates = new HashMap<Long, Template>();
   private Map<Long, ObjectTool> tools = new HashMap<Long, ObjectTool>();
	private Map<Long, SnmpTrap> traps = new HashMap<Long, SnmpTrap>();
   private Map<Integer, WebServiceDefinition> webServices = new HashMap<>();
   private Map<String, AssetAttribute> assetAttributes = new HashMap<>();
	private boolean modified = false;
	private List<SnmpTrap> snmpTrapCache = null;
	private List<EventProcessingPolicyRule> rulesCache = null;
   private List<ServerAction> actionsCache = null;

   /**
    * Create configuration export view
    */
	public ExportFileBuilder()
	{
      super(LocalizationHelper.getI18n(ExportFileBuilder.class).tr("Export Configuration"), ResourceManager.getImageDescriptor("icons/config-views/export.png"), "config.export", false);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
	{		
      ScrolledComposite scroller = new ScrolledComposite(parent, SWT.V_SCROLL);
      scroller.setExpandHorizontal(true);
      scroller.setExpandVertical(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            content.layout(true, true);
            scroller.setMinSize(content.computeSize(scroller.getSize().x, SWT.DEFAULT));
         }
      });

      content = new Composite(scroller, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = true;
      content.setLayout(layout);
      content.setBackground(content.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

      scroller.setContent(content);

		createDescriptionSection();		
		createTemplatesSection();
		createEventSection();
		createTrapSection();
		createRuleSection();
		createScriptSection();
		createToolsSection();
      createSummaryTablesSection();
      createActionsSection();
      createWebServiceSection();
      createAssetAttributesSection();
		createActions();
	}

	/**
    * Create "Description" section
    */
	private void createDescriptionSection()
	{
      Section section = new Section(content, i18n.tr("Description"), false);

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
      GridLayout layout = new GridLayout();
      layout.marginHeight = 2;
      layout.marginWidth = 2;
      clientArea.setLayout(layout);
      clientArea.setBackground(content.getBackground());

      description = new Text(clientArea, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
		gd = new GridData();
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
	private void createTemplatesSection()
	{
      Section section = new Section(content, i18n.tr("Templates"), false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
		GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
		clientArea.setLayout(layout);
		clientArea.setBackground(content.getBackground());

      templateViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 200;
		templateViewer.getTable().setLayoutData(gd);
		templateViewer.setContentProvider(new ArrayContentProvider());
		templateViewer.setLabelProvider(new DecoratingObjectLabelProvider());
		templateViewer.setComparator(new ElementLabelComparator((ILabelProvider)templateViewer.getLabelProvider()));
		templateViewer.getTable().setSortDirection(SWT.UP);

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
		linkAdd.setText(i18n.tr("Add..."));
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
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
		
      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
		linkRemove.setText(i18n.tr("Remove"));
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
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
      Section section = new Section(content, i18n.tr("Events"), false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
		GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
		clientArea.setLayout(layout);
      clientArea.setBackground(content.getBackground());

      eventViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 200;
		eventViewer.getTable().setLayoutData(gd);
		eventViewer.setContentProvider(new ArrayContentProvider());
		eventViewer.setLabelProvider(new BaseEventTemplateLabelProvider());
		eventViewer.setComparator(new ElementLabelComparator((ILabelProvider)eventViewer.getLabelProvider()));
		eventViewer.getTable().setSortDirection(SWT.UP);

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
		linkAdd.setText(i18n.tr("Add..."));
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
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
		
      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
		linkRemove.setText(i18n.tr("Remove"));
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
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
      Section section = new Section(content, i18n.tr("SNMP Traps"), false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
		GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
		clientArea.setLayout(layout);
      clientArea.setBackground(content.getBackground());
		
      trapViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 200;
		trapViewer.getTable().setLayoutData(gd);
		trapViewer.setContentProvider(new ArrayContentProvider());
		trapViewer.setLabelProvider(new TrapListLabelProvider());
		trapViewer.setComparator(new ElementLabelComparator((ILabelProvider)trapViewer.getLabelProvider()));
		trapViewer.getTable().setSortDirection(SWT.UP);

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
		linkAdd.setText(i18n.tr("Add..."));
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkAdd.setLayoutData(gd);
		linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				if (snmpTrapCache == null)
				{
					new Job(i18n.tr("Loading SNMP trap configuration"), ExportFileBuilder.this) {
						@Override
						protected void run(IProgressMonitor monitor) throws Exception
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
							return i18n.tr("Cannot load SNMP trap configuration");
						}
					}.start();
				}
				else
				{
					addTraps();
				}
			}
		});
		
      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
		linkRemove.setText(i18n.tr("Remove"));
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
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
      Section section = new Section(content, i18n.tr("Event Processing Rules"), false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
		GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
		clientArea.setLayout(layout);
      clientArea.setBackground(content.getBackground());
		
      ruleViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI);
		ruleViewer.getTable().setLinesVisible(true);
		ruleViewer.getTable().setHeaderVisible(true);

      TableColumn column = new TableColumn(ruleViewer.getTable(), SWT.LEFT);
      column.setText("Rule #");
      column.setWidth(60);

      column = new TableColumn(ruleViewer.getTable(), SWT.LEFT);
      column.setText("Rule Name");
      column.setWidth(250);

		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 200;
		ruleViewer.getTable().setLayoutData(gd);
		ruleViewer.setContentProvider(new ArrayContentProvider());
		ruleViewer.setLabelProvider(new RuleLabelProvider());
		ruleViewer.setComparator(new RuleComparator());
		ruleViewer.getTable().setSortDirection(SWT.UP);

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
		linkAdd.setText(i18n.tr("Add..."));
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkAdd.setLayoutData(gd);
		linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				if (rulesCache == null)
				{
					new Job(i18n.tr("Loading event processing policy"), ExportFileBuilder.this) {
						@Override
						protected void run(IProgressMonitor monitor) throws Exception
						{
							rulesCache = session.getEventProcessingPolicy().getRules();
                     actionsCache = session.getActions();
                     runInUIThread(() -> addRules());
						}

						@Override
						protected String getErrorMessage()
						{
							return i18n.tr("Cannot load event processing policy");
						}
					}.start();
				}
				else
				{
					addRules();
				}
			}
		});

      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
		linkRemove.setText(i18n.tr("Remove"));
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
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
      Section section = new Section(content, i18n.tr("Scripts"), false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
      clientArea.setLayout(layout);
      clientArea.setBackground(content.getBackground());
      
      scriptViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 200;
      scriptViewer.getTable().setLayoutData(gd);
      scriptViewer.setContentProvider(new ArrayContentProvider());
      scriptViewer.setLabelProvider(new ScriptLabelProvider());
      scriptViewer.setComparator(new ScriptComparator());
      scriptViewer.getTable().setSortDirection(SWT.UP);

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
      linkAdd.setText(i18n.tr("Add..."));
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addScripts();
         }
      });
      
      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
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
      Section section = new Section(content, i18n.tr("Object Tools"), false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
      clientArea.setLayout(layout);
      clientArea.setBackground(content.getBackground());
      
      toolsViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 200;
      toolsViewer.getTable().setLayoutData(gd);
      toolsViewer.setContentProvider(new ArrayContentProvider());
      toolsViewer.setLabelProvider(new ToolLabelProvider());
      toolsViewer.setComparator(new ToolComparator());
      toolsViewer.getTable().setSortDirection(SWT.UP);

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
      linkAdd.setText(i18n.tr("Add..."));
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
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
      
      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
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
      Section section = new Section(content, i18n.tr("Summary Tables"), false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
      clientArea.setLayout(layout);
      clientArea.setBackground(content.getBackground());
      
      summaryTableViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 200;
      summaryTableViewer.getTable().setLayoutData(gd);
      summaryTableViewer.setContentProvider(new ArrayContentProvider());
      summaryTableViewer.setLabelProvider(new SummaryTablesLabelProvider());
      summaryTableViewer.setComparator(new SummaryTablesComparator());
      summaryTableViewer.getTable().setSortDirection(SWT.UP);

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
      linkAdd.setText(i18n.tr("Add..."));
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addSummaryTables();
         }
      });
      
      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
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
    * Create "Actions" section
    */
   private void createActionsSection()
   {
      Section section = new Section(content, i18n.tr("Actions"), false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
      clientArea.setLayout(layout);
      clientArea.setBackground(content.getBackground());
      
      actionViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 200;
      actionViewer.getTable().setLayoutData(gd);
      actionViewer.setContentProvider(new ArrayContentProvider());
      actionViewer.setLabelProvider(new ActionLabelProvider());
      actionViewer.setComparator(new ActionComparator());
      actionViewer.getTable().setSortDirection(SWT.UP);

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
      linkAdd.setText(i18n.tr("Add..."));
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addActions();
         }
      });
      
      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkRemove.setLayoutData(gd);
      linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            removeObjects(actionViewer, actions);
         }
      });
   }
   
   /**
    * Create "Web Service Definitions" section
    */
   private void createWebServiceSection()
   {
      Section section = new Section(content, i18n.tr("Web Service Definitions"), false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
      clientArea.setLayout(layout);
      clientArea.setBackground(content.getBackground());
      
      webServiceViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 200;
      webServiceViewer.getTable().setLayoutData(gd);
      webServiceViewer.setContentProvider(new ArrayContentProvider());
      webServiceViewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((WebServiceDefinition)element).getName();
         }
      });
      webServiceViewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((WebServiceDefinition)e1).getName().compareToIgnoreCase(((WebServiceDefinition)e2).getName());
         }
      });
      webServiceViewer.getTable().setSortDirection(SWT.UP);

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
      linkAdd.setText(i18n.tr("Add..."));
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addWebServiceDefinitions();
         }
      });
      
      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkRemove.setLayoutData(gd);
      linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            IStructuredSelection selection = webServiceViewer.getStructuredSelection();
            if (selection.size() > 0)
            {
               removeObjects(webServiceViewer, webServices);
            }
         }
      });
   }
   
   /**
    * Create "Asset Attributes" section
    */
   private void createAssetAttributesSection()
   {
      Section section = new Section(content, i18n.tr("Asset Attributes"), false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      clientArea.setBackground(content.getBackground());

      assetAttributeViewer = new TableViewer(clientArea, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 200;
      gd.verticalSpan = 2;
      assetAttributeViewer.getTable().setLayoutData(gd);
      assetAttributeViewer.setContentProvider(new ArrayContentProvider());
      assetAttributeViewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((AssetAttribute)element).getEffectiveDisplayName();
         }
      });
      assetAttributeViewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((AssetAttribute)e1).getEffectiveDisplayName().compareToIgnoreCase(((AssetAttribute)e2).getEffectiveDisplayName());
         }
      });
      assetAttributeViewer.getTable().setSortDirection(SWT.UP);

      final ImageHyperlink linkAdd = new ImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(i18n.tr("Add..."));
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addAssetAttributes();
         }
      });

      final ImageHyperlink linkRemove = new ImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkRemove.setLayoutData(gd);
      linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            IStructuredSelection selection = (IStructuredSelection)assetAttributeViewer.getSelection();
            if (selection.size() > 0)
            {
               removeObjects(assetAttributeViewer, assetAttributes);
            }
         }
      });
   }

	/**
	 * Create actions
	 */
	private void createActions()
	{      
      actionExport = new Action(i18n.tr("&Export..."), SharedIcons.EXPORT) {
			@Override
			public void run()
			{
				save();
			}
		};		
      addKeyBinding("M1+E", actionExport);
      actionExport.setEnabled(false);

      actionClear = new Action(i18n.tr("&Clear"), SharedIcons.CLEAR) {
         @Override
         public void run()
         {
            clear();
         }
      };
      addKeyBinding("M1+L", actionClear);
	}

	/**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExport);
      manager.add(actionClear);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExport);
      manager.add(actionClear);
   }

	/**
	 * Do export operation and call completion handler when done
	 * 
	 * @param completionHandler
	 */
	private void doExport(final ExportCompletionHandler completionHandler)
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
      
      final long[] actionList = new long[actions.size()];
      i = 0;
      for(ServerAction a : actions.values())
         actionList[i++] = a.getId();
      
      final long[] webServiceList = new long[webServices.size()];
      i = 0;
      for(WebServiceDefinition a : webServices.values())
         webServiceList[i++] = a.getId();
      
      final String[] assetAttributesList = new String[assetAttributes.size()];
      i = 0;
      for(AssetAttribute a : assetAttributes.values())
         assetAttributesList[i++] = a.getName();

      final String descriptionText = description.getText();

      new Job(i18n.tr("Exporting and saving configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final File xml = session.exportConfiguration(descriptionText, eventList, trapList, templateList, ruleList, scriptList, toolList, summaryTableList, actionList, webServiceList,
                  assetAttributesList);
            runInUIThread(() -> completionHandler.exportCompleted(xml));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot export configuration");
         }
      }.start();
	}

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
	{
	   doExport(new ExportCompletionHandler() {
         @Override
         public void exportCompleted(final File xml)
         {
            WidgetHelper.saveTemporaryFile(ExportFileBuilder.this, "export.xml", new String[] { "*.xml", "*.*" }, new String[] { i18n.tr("XML files"), i18n.tr("All files") }, xml, "application/xml");
            modified = false;
         }
      });
	}

	/**
	 * @param o
	 * @return
	 */
	private static Object keyFromObject(Object o)
	{
      if (o instanceof DciSummaryTableDescriptor)
         return ((DciSummaryTableDescriptor)o).getId();
      if (o instanceof EventProcessingPolicyRule)
         return ((EventProcessingPolicyRule)o).getGuid();
      if (o instanceof EventTemplate)
         return ((EventTemplate)o).getCode();
      if (o instanceof ObjectTool)
         return ((ObjectTool)o).getId();
      if (o instanceof Script)
         return ((Script)o).getId();
      if (o instanceof ServerAction)
         return ((ServerAction)o).getId();
      if (o instanceof SnmpTrap)
         return ((SnmpTrap)o).getId();
	   if (o instanceof Template)
	      return ((Template)o).getObjectId();
      if (o instanceof WebServiceDefinition)
         return ((WebServiceDefinition)o).getId();
      if (o instanceof AssetAttribute)
         return ((AssetAttribute)o).getName();
      return null;
	}

	/**
	 * Remove objects from list
	 * 
	 * @param viewer
	 * @param objects
	 */
	private void removeObjects(TableViewer viewer, Map<?, ?> objects)
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
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
		EventSelectionDialog dlg = new EventSelectionDialog(getWindow().getShell());
		dlg.enableMultiSelection(true);
		if (dlg.open() == Window.OK)
		{
         Set<String> allMatches = new HashSet<>();
			for(EventTemplate t : dlg.getSelectedEvents())
			{
            events.put(t.getCode(), t);
            Matcher m = SCRIPT_PATTERN.matcher(t.getMessage());
            while (m.find()) 
            {
               allMatches.add(m.group(1));
            }
			}
			addReferencedScripts(allMatches);			
			eventViewer.setInput(events.values().toArray());
			setModified();
		}
	}
	
	/**
	 * Add referenced scripts
	 * 
	 * @param scriptNames list of script names
	 */
	void addReferencedScripts(Collection<String> scriptNames)
	{
      new Job(i18n.tr("Get script list"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<Script> scriptList = session.getScriptLibrary();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  for(String scriptName : scriptNames)
                  {
                     for(Script s : scriptList)
                     {
                        if (s.getName().equalsIgnoreCase(scriptName))
                        {
                           scripts.put(s.getId(), s);
                           break;
                        }
                     }
                  }
                  scriptViewer.setInput(scripts.values().toArray());
                  setModified();
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get script list from server");
         }
      }.start();	   
	}

	/**
	 * Add templates to list
	 */
	private void addTemplates()
	{
		final ObjectSelectionDialog dlg = new ObjectSelectionDialog(getWindow().getShell(), ObjectSelectionDialog.createTemplateSelectionFilter());
		dlg.enableMultiSelection(true);
		dlg.showFilterToolTip(false);
		if (dlg.open() == Window.OK)
		{
			final Set<Long> idList = new HashSet<Long>();
			for(AbstractObject o : dlg.getSelectedObjects())
			{
			   if (o instanceof TemplateGroup)
			   {
			      Set<AbstractObject> children = ((TemplateGroup)o).getAllChildren(AbstractObject.OBJECT_TEMPLATE);
			      for(AbstractObject child : children)
			      {
		            templates.put(((Template)child).getObjectId(), (Template)child);
		            idList.add(child.getObjectId());
			      }
			   }
			   else if (o instanceof Template)
			   {
   				templates.put(((Template)o).getObjectId(), (Template)o);
   				idList.add(o.getObjectId());
			   }
			}
			templateViewer.setInput(templates.values().toArray());
			setModified();
			new Job(i18n.tr("Resolving event dependencies"), this) {
				@Override
				protected void run(IProgressMonitor monitor) throws Exception
				{
               final Set<Integer> eventCodes = new HashSet<>();
               final Map<Long, Script> scriptList = new HashMap<>();
               Set<String> allMatches = new HashSet<>();
					for(Long id : idList)
					{
                  int[] e = session.getRelatedEvents(id);
                  for(int c : e)
						{
							if (c >= 100000)
								eventCodes.add(c);
						}

						for(Script s : session.getDataCollectionScripts(id)) 
						   scriptList.put(s.getId(), s);
						for (AgentPolicy entry : session.getAgentPolicyList(id).values())
						{
						   if (entry.getPolicyType().equals(AgentPolicy.AGENT_CONFIG) && 
						         (entry.getFlags() & AgentPolicy.EXPAND_MACRO) != 0)
						   {
	                     Matcher m = SCRIPT_PATTERN.matcher(entry.getContent());
	                     while (m.find()) 
	                     {
	                        allMatches.add(m.group(1));
	                     }
						   }
						}
					}
               runInUIThread(() -> {
                  for(EventTemplate e : session.findMultipleEventTemplates(eventCodes))
                  {
                     events.put(e.getCode(), e); 
                     Matcher m = SCRIPT_PATTERN.matcher(e.getMessage());
                     while (m.find()) 
                     {
                        allMatches.add(m.group(1));
                     }
                  }
                  addReferencedScripts(allMatches);
                  eventViewer.setInput(events.values().toArray());

                  scripts.putAll(scriptList);
                  scriptViewer.setInput(scripts.values().toArray());
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
		SelectSnmpTrapDialog dlg = new SelectSnmpTrapDialog(getWindow().getShell(), snmpTrapCache);
		if (dlg.open() == Window.OK)
		{
         final Set<Integer> eventCodes = new HashSet<>();
			for(SnmpTrap t : dlg.getSelection())
			{
				traps.put(t.getId(), t);
				if (t.getEventCode() >= 100000)
				{
               eventCodes.add(t.getEventCode());
				}
			}
			trapViewer.setInput(traps.values().toArray());
			setModified();
			if (eventCodes.size() > 0)
			{
	         Set<String> allMatches = new HashSet<>();
            for(EventTemplate t : session.findMultipleEventTemplates(eventCodes))
				{
				   events.put(t.getCode(), t);  
               Matcher m = SCRIPT_PATTERN.matcher(t.getMessage());
               while (m.find()) 
               {
                  allMatches.add(m.group(1));
               }
				}
            addReferencedScripts(allMatches);
				eventViewer.setInput(events.values().toArray());
			};
		}
	}

	/**
	 * Add rules to list
	 */
	private void addRules()
	{
		RuleSelectionDialog dlg = new RuleSelectionDialog(getWindow().getShell(), rulesCache);
		if (dlg.open() == Window.OK)
		{
         Set<String> allMatches = new HashSet<>();
         final Set<Integer> eventCodes = new HashSet<>();
         final Set<Long> actionCodes = new HashSet<>();
			for(EventProcessingPolicyRule r : dlg.getSelectedRules())
			{
            rules.put(r.getGuid(), r);
            for(Integer e : r.getEvents())
				{
					if (e >= 100000)
					{
						eventCodes.add(e);
					}
				}
            for(ActionExecutionConfiguration ac : r.getActions())
               actionCodes.add(ac.getActionId());
            
            Matcher m = SCRIPT_PATTERN.matcher(r.getAlarmKey());
            while (m.find()) 
            {
               allMatches.add(m.group(1));
            }
            m = SCRIPT_PATTERN.matcher(r.getAlarmMessage());
            while (m.find()) 
            {
               allMatches.add(m.group(1));
            }
            m = SCRIPT_PATTERN.matcher(r.getDowntimeTag());
            while (m.find()) 
            {
               allMatches.add(m.group(1));
            }
            for (Entry<String, String> entry : r.getPStorageSet().entrySet())
            {
               m = SCRIPT_PATTERN.matcher(entry.getKey());
               while (m.find()) 
               {
                  allMatches.add(m.group(1));
               }
               m = SCRIPT_PATTERN.matcher(entry.getValue());
               while (m.find()) 
               {
                  allMatches.add(m.group(1));
               }
            }
            for (String entry : r.getPStorageDelete())
            {
               m = SCRIPT_PATTERN.matcher(entry);
               while (m.find()) 
               {
                  allMatches.add(m.group(1));
               }
            }
            for (Entry<String, String> entry : r.getCustomAttributeStorageSet().entrySet())
            {
               m = SCRIPT_PATTERN.matcher(entry.getKey());
               while (m.find()) 
               {
                  allMatches.add(m.group(1));
               }
               m = SCRIPT_PATTERN.matcher(entry.getValue());
               while (m.find()) 
               {
                  allMatches.add(m.group(1));
               }
            }
            for (String entry : r.getCustomAttributeStorageDelete())
            {
               m = SCRIPT_PATTERN.matcher(entry);
               while (m.find()) 
               {
                  allMatches.add(m.group(1));
               }
            }
            for (ActionExecutionConfiguration entry : r.getActions())
            {
               m = SCRIPT_PATTERN.matcher(entry.getTimerKey());
               while (m.find()) 
               {
                  allMatches.add(m.group(1));
               }
               m = SCRIPT_PATTERN.matcher(entry.getBlockingTimerKey());
               while (m.find()) 
               {
                  allMatches.add(m.group(1));
               }
            }
            for (String entry : r.getTimerCancellations())
            {
               m = SCRIPT_PATTERN.matcher(entry);
               while (m.find()) 
               {
                  allMatches.add(m.group(1));
               }
            }
			}
			ruleViewer.setInput(rules.values().toArray());
			setModified();
			
         if (!eventCodes.isEmpty())
			{
            for(EventTemplate t : session.findMultipleEventTemplates(eventCodes))
            {
               events.put(t.getCode(), t);
               Matcher m = SCRIPT_PATTERN.matcher(t.getMessage());
               while (m.find()) 
               {
                  allMatches.add(m.group(1));
               }
            } 
				eventViewer.setInput(events.values().toArray());
         }
         if (!actionCodes.isEmpty())
         {
            for(Long actionId : actionCodes)
            {
               for(ServerAction action : actionsCache)
               {
                  if (action.getId() == actionId)
                  {
                     actions.put(actionId, action);
                     Matcher m = SCRIPT_PATTERN.matcher(action.getEmailSubject());
                     while (m.find()) 
                     {
                        allMatches.add(m.group(1));
                     }    
                     m = SCRIPT_PATTERN.matcher(action.getData());
                     while (m.find()) 
                     {
                        allMatches.add(m.group(1));
                     }  
                     m = SCRIPT_PATTERN.matcher(action.getRecipientAddress());
                     while (m.find()) 
                     {
                        allMatches.add(m.group(1));
                     }  
                     break;
                  }
               }
            }
            actionViewer.setInput(actions.values().toArray());
         }
         addReferencedScripts(allMatches);
		}
	}

   /**
    * Add script to list
    */
   private void addScripts()
   {
      SelectScriptDialog dlg = new SelectScriptDialog(getWindow().getShell());
      dlg.setMultiSelection(true);
      if (dlg.open() == Window.OK)
      {
         for(Script s : dlg.getSelection())
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
      ObjectToolSelectionDialog dlg = new ObjectToolSelectionDialog(getWindow().getShell());
      if (dlg.open() == Window.OK)
      {
         for(ObjectTool t : dlg.getSelection())
            tools.put(t.getId(), t);
         toolsViewer.setInput(tools.values().toArray());
         setModified();
      }
   }
   
   /**
    * Add oject tools to list
    */
   private void addSummaryTables()
   {
      SummaryTableSelectionDialog dlg = new SummaryTableSelectionDialog(getWindow().getShell());
      if (dlg.open() == Window.OK)
      {
         for(DciSummaryTableDescriptor t : dlg.getSelection())
            summaryTables.put(t.getId(), t);
         summaryTableViewer.setInput(summaryTables.values().toArray());
         setModified();
      }
   }
   
   /**
    * Add actions to list
    */
   private void addActions()
   {
      ActionSelectionDialog dlg = new ActionSelectionDialog(getWindow().getShell());
      if (dlg.open() == Window.OK)
      {
         Set<String> allMatches = new HashSet<>();
         for(ServerAction a : dlg.getSelection())
         {
            actions.put(a.getId(), a);
            Matcher m = SCRIPT_PATTERN.matcher(a.getEmailSubject());
            while (m.find()) 
            {
               allMatches.add(m.group(1));
            }    
            m = SCRIPT_PATTERN.matcher(a.getData());
            while (m.find()) 
            {
               allMatches.add(m.group(1));
            }  
            m = SCRIPT_PATTERN.matcher(a.getRecipientAddress());
            while (m.find()) 
            {
               allMatches.add(m.group(1));
            }         
         }
         addReferencedScripts(allMatches);
         actionViewer.setInput(actions.values().toArray());
         setModified();
      }
   }

   /**
    * Add web service definitions to list
    */
   private void addWebServiceDefinitions()
   {
      SelectWebServiceDlg dlg = new SelectWebServiceDlg(getWindow().getShell(), true);
      if (dlg.open() == Window.OK)
      {
         for(WebServiceDefinition service : dlg.getSelection())
            webServices.put(service.getId(), service);
         webServiceViewer.setInput(webServices.values().toArray());
         setModified();
      }
   }
   
   /**
    * Add asset attributes to list
    */
   private void addAssetAttributes()
   {
      SelectAssetAttributeDlg dlg = new SelectAssetAttributeDlg(getWindow().getShell());
      if (dlg.open() == Window.OK)
      {
         for(AssetAttribute service : dlg.getSelectedAttributes())
            assetAttributes.put(service.getName(), service);
         assetAttributeViewer.setInput(assetAttributes.values().toArray());
         setModified();
      }
   }

   /**
    * Clear configuration
    */
   private void clear()
   {
      description.setText("");

      actions.clear();
      events.clear();
      rules.clear();
      scripts.clear();
      summaryTables.clear();
      templates.clear();
      traps.clear();
      tools.clear();
      webServices.clear();
      assetAttributes.clear();

      actionViewer.setInput(new Object[0]);
      eventViewer.setInput(new Object[0]);
      ruleViewer.setInput(new Object[0]);
      scriptViewer.setInput(new Object[0]);
      summaryTableViewer.setInput(new Object[0]);
      templateViewer.setInput(new Object[0]);
      toolsViewer.setInput(new Object[0]);
      trapViewer.setInput(new Object[0]);
      webServiceViewer.setInput(new Object[0]);
      assetAttributeViewer.setInput(new Object[0]);

      modified = false;
      actionExport.setEnabled(false);
   }

   /**
    * Export completion handler
    */
   private interface ExportCompletionHandler
   {
      /**
       * Called when export is complete
       * 
       * @param xml resulting XML document
       */
      public void exportCompleted(final File xml);
   }

   /**
    * Mark view as modified
    */
   private void setModified()
   {
      modified = true;
      actionExport.setEnabled(true);
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return modified;
   }
}
