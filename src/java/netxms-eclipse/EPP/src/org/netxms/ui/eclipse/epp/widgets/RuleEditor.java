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
package org.netxms.ui.eclipse.epp.widgets;

import java.util.Iterator;
import java.util.Map.Entry;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.osgi.util.NLS;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.internal.IWorkbenchHelpContextIds;
import org.eclipse.ui.internal.WorkbenchMessages;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.eclipse.ui.internal.dialogs.PropertyPageContributorManager;
import org.eclipse.ui.internal.dialogs.PropertyPageManager;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.situations.Situation;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.epp.Messages;
import org.netxms.ui.eclipse.epp.SituationCache;
import org.netxms.ui.eclipse.epp.views.EventProcessingPolicyEditor;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.CGroup;
import org.netxms.ui.eclipse.widgets.helpers.DashboardElementButton;

/**
 * Rule editor widget
 */
@SuppressWarnings("restriction")
public class RuleEditor extends Composite
{
	private static final int INDENT = 20;
	
	private EventProcessingPolicyRule rule;
	private int ruleNumber;
	private EventProcessingPolicyEditor editor;
	private NXCSession session;
	private WorkbenchLabelProvider labelProvider;
	private boolean collapsed = true;
	private boolean verticalLayout = false;
	private Composite leftPanel;
	private Label ruleNumberLabel;
	private Composite header;
	private Label headerLabel;
	private Composite mainArea;
	private CGroup condition;
	private CGroup action;
	private Label expandButton;
	private Label editButton;
	private boolean modified = false;
	private boolean selected = false;
	private MouseListener ruleMouseListener;
	
	/**
	 * @param parent
	 * @param rule
	 */
	public RuleEditor(Composite parent, EventProcessingPolicyRule rule, int ruleNumber, EventProcessingPolicyEditor editor)
	{
		super(parent, SWT.NONE);
		this.rule = rule;
		this.ruleNumber = ruleNumber;
		this.editor = editor;
		
		session = (NXCSession)ConsoleSharedData.getSession();
		labelProvider = new WorkbenchLabelProvider();
		
		setBackground(SharedColors.getColor(SharedColors.RULE_EDITOR_RULE_BORDER, getDisplay()));

		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.marginHeight = 1;
		layout.marginWidth = 0;
		layout.horizontalSpacing = 1;
		layout.verticalSpacing = 1;
		setLayout(layout);
		
		ruleMouseListener = new MouseListener() {
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
			}

			@Override
			public void mouseDown(MouseEvent e)
			{
				switch(e.button)
				{
					case 1:
						processRuleMouseEvent(e);
						break;
					default:
						if (!selected)
							RuleEditor.this.editor.setSelection(RuleEditor.this);
						break;
				}
			}

			@Override
			public void mouseUp(MouseEvent e)
			{
			}
		};
		
		createLeftPanel();
		createHeader();
		createMainArea();
		
		createPopupMenu(new Control[] { leftPanel, ruleNumberLabel, header, headerLabel });
		
		condition = new CGroup(mainArea, Messages.get().RuleEditor_Filter) {
			@Override
			protected Control createClientArea(Composite parent)
			{
				setBorderColor(SharedColors.getColor(SharedColors.RULE_EDITOR_CONDITION_BORDER, getDisplay()));
				setTitleColor(SharedColors.getColor(SharedColors.RULE_EDITOR_TITLE_TEXT, getDisplay()));
				return createConditionControl(parent, RuleEditor.this.rule);
			}
		};
		configureLayout(condition);
		final Action editRuleCondition = new Action() {
			@Override
			public void run()
			{
				editRule("org.netxms.ui.eclipse.epp.propertypages.RuleCondition#0"); //$NON-NLS-1$
			}
		};
		condition.addButton(new DashboardElementButton(Messages.get().RuleEditor_EditCondition, editor.getImageEdit(), editRuleCondition));
		condition.setDoubleClickAction(editRuleCondition);

		action = new CGroup(mainArea, Messages.get().RuleEditor_Action) {
			@Override
			protected Control createClientArea(Composite parent)
			{
				setBorderColor(SharedColors.getColor(SharedColors.RULE_EDITOR_ACTION_BORDER, getDisplay()));
				setTitleColor(SharedColors.getColor(SharedColors.RULE_EDITOR_TITLE_TEXT, getDisplay()));
				return createActionControl(parent, RuleEditor.this.rule);
			}
		};
		configureLayout(action);
		final Action editRuleAction = new Action() {
			@Override
			public void run()
			{
				editRule("org.netxms.ui.eclipse.epp.propertypages.RuleAction#1"); //$NON-NLS-1$
			}
		};
		action.addButton(new DashboardElementButton(Messages.get().RuleEditor_EditActions, editor.getImageEdit(), editRuleAction));
		action.setDoubleClickAction(editRuleAction);
	}
	
	/**
	 * Create main area which contains condition and action elements
	 */
	private void createMainArea()
	{
		mainArea = new Composite(this, SWT.NONE);
		mainArea.setBackground(SharedColors.getColor(SharedColors.RULE_EDITOR_BACKGROUND, getDisplay()));
		
		GridLayout layout = new GridLayout();
		layout.numColumns = verticalLayout ? 1 : 2;
		layout.marginHeight = 4;
		layout.marginWidth = 4;
		layout.makeColumnsEqualWidth = true;
		mainArea.setLayout(layout);
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.exclude = collapsed;
		mainArea.setLayoutData(gd);
	}
	
	/**
	 * Create panel with rule number and collapse/expand button on the left
	 */
	private void createLeftPanel()
	{
		leftPanel = new Composite(this, SWT.NONE);
		leftPanel.setBackground(SharedColors.getColor(rule.isDisabled() ? SharedColors.RULE_EDITOR_DISABLED_TITLE_BACKGROUND : SharedColors.RULE_EDITOR_NORMAL_TITLE_BACKGROUND, getDisplay()));
		leftPanel.addMouseListener(ruleMouseListener);
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		leftPanel.setLayout(layout);

		GridData gd = new GridData();
		gd.verticalSpan = collapsed ? 1 : 2;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		leftPanel.setLayoutData(gd);		
		
		ruleNumberLabel = new Label(leftPanel, SWT.NONE);
		ruleNumberLabel.setText(Integer.toString(ruleNumber));
		ruleNumberLabel.setFont(editor.getBoldFont());
		ruleNumberLabel.setBackground(SharedColors.getColor(rule.isDisabled() ? SharedColors.RULE_EDITOR_DISABLED_TITLE_BACKGROUND : SharedColors.RULE_EDITOR_NORMAL_TITLE_BACKGROUND, getDisplay()));
		ruleNumberLabel.setForeground(SharedColors.getColor(SharedColors.RULE_EDITOR_TITLE_TEXT, getDisplay()));
		ruleNumberLabel.setAlignment(SWT.CENTER);
		ruleNumberLabel.addMouseListener(ruleMouseListener);
		
		gd = new GridData();
		gd.horizontalAlignment = SWT.CENTER;
		gd.verticalAlignment = SWT.CENTER;
		gd.grabExcessVerticalSpace = true;
		gd.widthHint = 30;
		ruleNumberLabel.setLayoutData(gd);
	}

	/**
	 * Create popup menu for given controls
	 * 
	 * @param controls
	 */
	private void createPopupMenu(final Control[] controls)
	{
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				editor.fillRuleContextMenu(mgr);
			}
		});

		for(Control c : controls)
		{
			Menu menu = menuMgr.createContextMenu(c);
			c.setMenu(menu);
		}
	}
		
	/**
	 * Create rule header
	 */
	private void createHeader()
	{
		header = new Composite(this, SWT.NONE);
		header.setBackground(SharedColors.getColor(rule.isDisabled() ? SharedColors.RULE_EDITOR_DISABLED_TITLE_BACKGROUND : SharedColors.RULE_EDITOR_NORMAL_TITLE_BACKGROUND, getDisplay()));
		header.addMouseListener(ruleMouseListener);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 3;
		header.setLayout(layout);
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		header.setLayoutData(gd);
		
		headerLabel = new Label(header, SWT.NONE);
		if (rule.isDisabled())
			headerLabel.setText(rule.getComments() + Messages.get().RuleEditor_DisabledSuffix);
		else
			headerLabel.setText(rule.getComments());
		headerLabel.setBackground(SharedColors.getColor(rule.isDisabled() ? SharedColors.RULE_EDITOR_DISABLED_TITLE_BACKGROUND : SharedColors.RULE_EDITOR_NORMAL_TITLE_BACKGROUND, getDisplay()));
		headerLabel.setForeground(SharedColors.getColor(SharedColors.RULE_EDITOR_TITLE_TEXT, getDisplay()));
		headerLabel.setFont(editor.getNormalFont());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		headerLabel.setLayoutData(gd);
		headerLabel.addMouseListener(new MouseListener() {
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
				if (e.button == 1)
					setCollapsed(!isCollapsed(), true);
			}

			@Override
			public void mouseDown(MouseEvent e)
			{
				ruleMouseListener.mouseDown(e);
			}

			@Override
			public void mouseUp(MouseEvent e)
			{
				ruleMouseListener.mouseUp(e);
			}
		});
		
		editButton = new Label(header, SWT.NONE);
		editButton.setBackground(SharedColors.getColor(rule.isDisabled() ? SharedColors.RULE_EDITOR_DISABLED_TITLE_BACKGROUND : SharedColors.RULE_EDITOR_NORMAL_TITLE_BACKGROUND, getDisplay()));
		editButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
		editButton.setImage(editor.getImageEdit());
		editButton.setToolTipText(Messages.get().RuleEditor_Tooltip_EditRule);
		editButton.addMouseListener(new MouseListener() {
			private boolean doAction = false;
			
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
				if (e.button == 1)
					doAction = false;
			}

			@Override
			public void mouseDown(MouseEvent e)
			{
				if (e.button == 1)
					doAction = true;
			}

			@Override
			public void mouseUp(MouseEvent e)
			{
				if ((e.button == 1) && doAction)
					editRule("org.netxms.ui.eclipse.epp.propertypages.RuleComments#2"); //$NON-NLS-1$
			}
		});
		
		expandButton = new Label(header, SWT.NONE);
		expandButton.setBackground(SharedColors.getColor(rule.isDisabled() ? SharedColors.RULE_EDITOR_DISABLED_TITLE_BACKGROUND : SharedColors.RULE_EDITOR_NORMAL_TITLE_BACKGROUND, getDisplay()));
		expandButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
		expandButton.setImage(collapsed ? editor.getImageExpand() : editor.getImageCollapse());
		expandButton.setToolTipText(collapsed ? Messages.get().RuleEditor_Tooltip_ExpandRule : Messages.get().RuleEditor_Tooltip_CollapseRule);
		expandButton.addMouseListener(new MouseListener() {
			private boolean doAction = false;
			
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
				if (e.button == 1)
					doAction = false;
			}

			@Override
			public void mouseDown(MouseEvent e)
			{
				if (e.button == 1)
					doAction = true;
			}

			@Override
			public void mouseUp(MouseEvent e)
			{
				if ((e.button == 1) && doAction)
					setCollapsed(!isCollapsed(), true);
			}
		});
	}
	
	/**
	 * Configure layout for child element
	 * 
	 * @param ctrl child control
	 */
	private void configureLayout(Control ctrl)
	{
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		ctrl.setLayoutData(gd);
	}
	
	/**
	 * Create mouse listener for elements
	 * 
	 * @param pageId property page ID to be opened on double click
	 * @return
	 */
	private MouseListener createMouseListener(final String pageId)
	{
		return new MouseListener() {
			@Override
			public void mouseUp(MouseEvent e)
			{
			}
			
			@Override
			public void mouseDown(MouseEvent e)
			{
			}
			
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
				editRule(pageId);
			}
		};
	}
	
	/**
	 * Create condition summary control
	 * 
	 * @param parent
	 * @param rule
	 * @return
	 */
	private Control createConditionControl(Composite parent, final EventProcessingPolicyRule rule)
	{
		Composite clientArea = new Composite(parent, SWT.NONE);
		
		clientArea.setBackground(SharedColors.getColor(SharedColors.RULE_EDITOR_BACKGROUND, getDisplay()));
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = 0;
		clientArea.setLayout(layout);
		
		boolean needAnd = false;
		if (((rule.getSources().size() > 0) && rule.isSourceInverted()) ||
			 ((rule.getSources().size() == 0) && (rule.getEvents().size() > 0) && rule.isEventsInverted()))
			createLabel(clientArea, 0, true, Messages.get().RuleEditor_IF_NOT, null);
		else
			createLabel(clientArea, 0, true, Messages.get().RuleEditor_IF, null);
		
		/* source */
		if (rule.getSources().size() > 0)
		{
			final MouseListener listener = createMouseListener("org.netxms.ui.eclipse.epp.propertypages.RuleSourceObjects#0"); //$NON-NLS-1$
			addConditionGroupLabel(clientArea, Messages.get().RuleEditor_SourceIs, needAnd, rule.isSourceInverted(), listener);
			
			for(Long id : rule.getSources())
			{
				CLabel clabel = createCLabel(clientArea, 2, false);
				clabel.addMouseListener(listener);
				
				AbstractObject object = session.findObjectById(id);
				if (object != null)
				{
					clabel.setText(object.getObjectName());
					clabel.setImage(labelProvider.getImage(object));
				}
				else
				{
					clabel.setText("[" + id.toString() + "]"); //$NON-NLS-1$ //$NON-NLS-2$
					clabel.setImage(SharedIcons.IMG_UNKNOWN_OBJECT);
				}
			}
			needAnd = true;
		}
		
		/* events */
		if (rule.getEvents().size() > 0)
		{
			final MouseListener listener = createMouseListener("org.netxms.ui.eclipse.epp.propertypages.RuleEvents#10"); //$NON-NLS-1$
			addConditionGroupLabel(clientArea, Messages.get().RuleEditor_EventIs, needAnd, rule.isEventsInverted(), listener);
			
			for(Long code : rule.getEvents())
			{
				CLabel clabel = createCLabel(clientArea, 2, false);
				clabel.addMouseListener(listener);
				
				EventTemplate event = session.findEventTemplateByCode(code);
				if (event != null)
				{
					clabel.setText(event.getName());
					clabel.setImage(StatusDisplayInfo.getStatusImage(event.getSeverity()));
				}
				else
				{
					clabel.setText("<" + code.toString() + ">"); //$NON-NLS-1$ //$NON-NLS-2$
					clabel.setImage(StatusDisplayInfo.getStatusImage(Severity.UNKNOWN));
				}
			}
			needAnd = true;
		}
		
		/* severity */
		if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_ANY) != EventProcessingPolicyRule.SEVERITY_ANY)
		{
			final MouseListener listener = createMouseListener("org.netxms.ui.eclipse.epp.propertypages.RuleSeverityFilter#20"); //$NON-NLS-1$
			addConditionGroupLabel(clientArea, Messages.get().RuleEditor_SeverityIs, needAnd, false, listener);
			
			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_NORMAL) != 0)
				addSeverityLabel(clientArea, Severity.NORMAL, listener);

			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_WARNING) != 0)
				addSeverityLabel(clientArea, Severity.WARNING, listener);

			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_MINOR) != 0)
				addSeverityLabel(clientArea, Severity.MINOR, listener);

			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_MAJOR) != 0)
				addSeverityLabel(clientArea, Severity.MAJOR, listener);

			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_CRITICAL) != 0)
				addSeverityLabel(clientArea, Severity.CRITICAL, listener);
			
			needAnd = true;
		}
		
		/* script */
		if ((rule.getScript() != null) && !rule.getScript().isEmpty())
		{
			final MouseListener listener = createMouseListener("org.netxms.ui.eclipse.epp.propertypages.RuleFilterScript#30"); //$NON-NLS-1$
			addConditionGroupLabel(clientArea, Messages.get().RuleEditor_ScriptIs, needAnd, false, listener);
			
			ScriptEditor scriptEditor = new ScriptEditor(clientArea, SWT.BORDER, SWT.NONE);
			GridData gd = new GridData();
			gd.horizontalIndent = INDENT * 2;
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			scriptEditor.setLayoutData(gd);
			scriptEditor.setText(rule.getScript());
			scriptEditor.getTextWidget().setEditable(false);
			scriptEditor.getTextWidget().addMouseListener(listener);
		}
		
		return clientArea;
	}
	
	/**
	 * Create label with given text and indent
	 * 
	 * @param parent
	 * @param indent
	 * @param bold
	 * @param text
	 * @return
	 */
	private Label createLabel(Composite parent, int indent, boolean bold, String text, MouseListener mouseListener)
	{
		Label label = new Label(parent, SWT.NONE);
		label.setBackground(SharedColors.getColor(SharedColors.RULE_EDITOR_BACKGROUND, getDisplay()));
		label.setFont(bold ? editor.getBoldFont() : editor.getNormalFont());
		label.setText(text);

		GridData gd = new GridData();
		gd.horizontalIndent = INDENT * indent;
		label.setLayoutData(gd);
		
		if (mouseListener != null)
			label.addMouseListener(mouseListener);
		
		return label;
	}

	/**
	 * Create CLable with given indent
	 * 
	 * @param parent
	 * @param indent
	 * @param bold
	 * @return
	 */
	private CLabel createCLabel(Composite parent, int indent, boolean bold)
	{
		CLabel label = new CLabel(parent, SWT.NONE);
		label.setBackground(SharedColors.getColor(SharedColors.RULE_EDITOR_BACKGROUND, getDisplay()));
		label.setFont(bold ? editor.getBoldFont() : editor.getNormalFont());

		GridData gd = new GridData();
		gd.horizontalIndent = INDENT * indent;
		label.setLayoutData(gd);
		
		return label;
	}
	
	/**
	 * Add condition group opening text
	 * 
	 * @param parent parent composite
	 * @param title group's title
	 * @param needAnd true if AND clause have to be added
	 */
	private void addConditionGroupLabel(Composite parent, String title, boolean needAnd, boolean inverted, MouseListener mouseListener)
	{
		if (needAnd)
			createLabel(parent, 0, true, inverted ? Messages.get().RuleEditor_AND_NOT : Messages.get().RuleEditor_AND, null);
		createLabel(parent, 1, false, title, mouseListener);
	}
	
	/**
	 * Add severity label
	 * 
	 * @param parent parent composite
	 * @param severity severity code
	 */
	private void addSeverityLabel(Composite parent, int severity, MouseListener mouseListener)
	{
		CLabel clabel = createCLabel(parent, 2, false);
		clabel.setText(StatusDisplayInfo.getStatusText(severity));
		clabel.setImage(StatusDisplayInfo.getStatusImage(severity));
		clabel.addMouseListener(mouseListener);
	}

	/**
	 * Create action summary control
	 * 
	 * @param parent
	 * @param rule
	 * @return
	 */
	private Control createActionControl(Composite parent, final EventProcessingPolicyRule rule)
	{
		Composite clientArea = new Composite(parent, SWT.NONE);
		
		clientArea.setBackground(SharedColors.getColor(SharedColors.RULE_EDITOR_BACKGROUND, getDisplay()));
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = 0;
		clientArea.setLayout(layout);
		
		/* alarm */
		if ((rule.getFlags() & EventProcessingPolicyRule.GENERATE_ALARM) != 0)
		{
			final MouseListener listener = createMouseListener("org.netxms.ui.eclipse.epp.propertypages.RuleAlarm#10"); //$NON-NLS-1$
			if (rule.getAlarmSeverity() < Severity.UNMANAGED)
			{
				addActionGroupLabel(clientArea, Messages.get().RuleEditor_GenerateAlarm, editor.getImageAlarm(), listener);
				
				CLabel clabel = createCLabel(clientArea, 1, false);
				clabel.setImage(StatusDisplayInfo.getStatusImage(rule.getAlarmSeverity()));
				clabel.setText(rule.getAlarmMessage());
				clabel.addMouseListener(listener);
				
				if ((rule.getAlarmKey() != null) && !rule.getAlarmKey().isEmpty())
				{
					createLabel(clientArea, 1, false, String.format(Messages.get().RuleEditor_WithKey, rule.getAlarmKey()), null);
				}
			}
			else if (rule.getAlarmSeverity() == Severity.UNMANAGED)
			{
				addActionGroupLabel(clientArea, Messages.get().RuleEditor_TerminateAlarms, editor.getImageTerminate(), listener);
				createLabel(clientArea, 1, false, String.format(Messages.get().RuleEditor_WithKey, rule.getAlarmKey()), listener);
				if ((rule.getFlags() & EventProcessingPolicyRule.TERMINATE_BY_REGEXP) != 0)
					createLabel(clientArea, 1, false, Messages.get().RuleEditor_UserRegexpForTerminate, listener);
			}
			else if (rule.getAlarmSeverity() == Severity.DISABLED)
			{
				addActionGroupLabel(clientArea, Messages.get().RuleEditor_ResolveAlarms, editor.getImageTerminate(), listener);
				createLabel(clientArea, 1, false, String.format(Messages.get().RuleEditor_WithKey, rule.getAlarmKey()), listener);
				if ((rule.getFlags() & EventProcessingPolicyRule.TERMINATE_BY_REGEXP) != 0)
					createLabel(clientArea, 1, false, Messages.get().RuleEditor_UseRegexpForResolve, listener);
			}
		}
		
		/* situation */
		if (rule.getSituationId() != 0)
		{
			final MouseListener listener = createMouseListener("org.netxms.ui.eclipse.epp.propertypages.RuleSituation#20"); //$NON-NLS-1$
			addActionGroupLabel(clientArea, Messages.get().RuleEditor_UpdateSituation, editor.getImageSituation(), listener);
			Situation s = SituationCache.findSituation(rule.getSituationId());
			createLabel(clientArea, 1, false, "\"" + ((s != null) ? s.getName() : Messages.get().RuleEditor_Unknown) + Messages.get().RuleEditor_Instance + rule.getSituationInstance() + "\"", listener); //$NON-NLS-1$ //$NON-NLS-2$
			createLabel(clientArea, 1, false, Messages.get().RuleEditor_Attributes, listener);
			for(Entry<String, String> e : rule.getSituationAttributes().entrySet())
			{
				createLabel(clientArea, 2, false, e.getKey() + " = \"" + e.getValue() + "\"", listener); //$NON-NLS-1$ //$NON-NLS-2$
			}
		}
		
		/* actions */
		if (rule.getActions().size() > 0)
		{
			final MouseListener listener = createMouseListener("org.netxms.ui.eclipse.epp.propertypages.RuleServerActions#30"); //$NON-NLS-1$
			addActionGroupLabel(clientArea, Messages.get().RuleEditor_ExecuteActions, editor.getImageExecute(), listener);
			for(Long id : rule.getActions())
			{
				CLabel clabel = createCLabel(clientArea, 1, false);
				clabel.addMouseListener(listener);
				ServerAction action = editor.findActionById(id);
				if (action != null)
				{
					clabel.setText(action.getName());
					clabel.setImage(labelProvider.getImage(action));
				}
				else
				{
					clabel.setText("<" + id.toString() + ">"); //$NON-NLS-1$ //$NON-NLS-2$
				}
			}
		}
		
		/* flags */
		if ((rule.getFlags() & EventProcessingPolicyRule.STOP_PROCESSING) != 0)
		{
			final MouseListener listener = createMouseListener("org.netxms.ui.eclipse.epp.propertypages.RuleAction#1"); //$NON-NLS-1$
			addActionGroupLabel(clientArea, Messages.get().RuleEditor_StopProcessing, editor.getImageStop(), listener);
		}
		
		return clientArea;
	}

	/**
	 * Add condition group opening text
	 * 
	 * @param parent parent composite
	 * @param title group's title
	 * @param needAnd true if AND clause have to be added
	 */
	private void addActionGroupLabel(Composite parent, String title, Image image, MouseListener mouseListener)
	{
		CLabel label = createCLabel(parent, 0, true);
		label.setImage(image);
		label.setText(title);
		label.addMouseListener(mouseListener);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Widget#dispose()
	 */
	@Override
	public void dispose()
	{
		labelProvider.dispose();
		super.dispose();
	}

	/**
	 * @return the ruleNumber
	 */
	public int getRuleNumber()
	{
		return ruleNumber;
	}

	/**
	 * @param ruleNumber the ruleNumber to set
	 */
	public void setRuleNumber(int ruleNumber)
	{
		this.ruleNumber = ruleNumber;
		ruleNumberLabel.setText(Integer.toString(ruleNumber));
		leftPanel.layout();
	}

	/**
	 * @param verticalLayout the verticalLayout to set
	 */
	public void setVerticalLayout(boolean verticalLayout, boolean doLayout)
	{
		this.verticalLayout = verticalLayout;
		GridLayout layout = (GridLayout)mainArea.getLayout();
		layout.numColumns = verticalLayout ? 1 : 2;
		if (doLayout)
			layout();
	}
	
	/**
	 * Check if widget is in collapsed state.
	 * 
	 * @return true if widget is in collapsed state
	 */
	public boolean isCollapsed()
	{
		return collapsed;
	}

	/**
	 * Set widget's to collapsed or expanded state
	 *  
	 * @param collapsed true to collapse widget, false to expand
	 * @param doLayout if set to true, view layout will be updated
	 */
	public void setCollapsed(boolean collapsed, boolean doLayout)
	{
		this.collapsed = collapsed;
		expandButton.setImage(collapsed ? editor.getImageExpand() : editor.getImageCollapse());
		expandButton.setToolTipText(collapsed ? Messages.get().RuleEditor_Tooltip_ExpandRule : Messages.get().RuleEditor_Tooltip_CollapseRule);
		mainArea.setVisible(!collapsed);
		((GridData)mainArea.getLayoutData()).exclude = collapsed;
		((GridData)leftPanel.getLayoutData()).verticalSpan = collapsed ? 1 : 2;
		if (doLayout)
			editor.updateEditorAreaLayout();
	}
	
	/**
	 * Create a new property dialog.
	 * 
	 * @param shell
	 *            the parent shell
	 * @param propertyPageId
	 *            the property page id
	 * @param element
	 *            the adaptable element
	 * @return the property dialog
	 */
	@SuppressWarnings("rawtypes")
	private static PropertyDialog createDialogOn(Shell shell, final String propertyPageId, Object element, String name)
	{
		PropertyPageManager pageManager = new PropertyPageManager();
		String title = "";//$NON-NLS-1$

		if (element == null)
		{
			return null;
		}
		
		// load pages for the selection
		// fill the manager with contributions from the matching contributors
		PropertyPageContributorManager.getManager().contribute(pageManager, element);
		// testing if there are pages in the manager
		Iterator pages = pageManager.getElements(PreferenceManager.PRE_ORDER).iterator();
		if (!pages.hasNext())
		{
			MessageDialogHelper.openInformation(shell, WorkbenchMessages.PropertyDialog_messageTitle,
					NLS.bind(WorkbenchMessages.PropertyDialog_noPropertyMessage, name));
			return null;
		}
		title = NLS.bind(WorkbenchMessages.PropertyDialog_propertyMessage, name);
		PropertyDialog propertyDialog = new PropertyDialog(shell, pageManager, new StructuredSelection(element)) {
			@Override
			protected TreeViewer createTreeViewer(Composite parent)
			{
				TreeViewer viewer = super.createTreeViewer(parent);
				viewer.expandAll();
				viewer.setAutoExpandLevel(TreeViewer.ALL_LEVELS);
				return viewer;
			}
		};

		if (propertyPageId != null)
		{
			propertyDialog.setSelectedNode(propertyPageId);
		}
		propertyDialog.create();

		propertyDialog.getShell().setText(title);
		PlatformUI.getWorkbench().getHelpSystem().setHelp(propertyDialog.getShell(), IWorkbenchHelpContextIds.PROPERTY_DIALOG);

		return propertyDialog;
	}

	/**
	 * Edit rule
	 */
	private void editRule(String pageId)
	{
		PropertyDialog dlg = createDialogOn(editor.getSite().getShell(), pageId, this, Messages.get().RuleEditor_Rule + ruleNumber);
		if (dlg != null)
		{
			modified = false;
			dlg.open();
			if (modified)
			{
				if (rule.isDisabled())
					headerLabel.setText(rule.getComments() + Messages.get().RuleEditor_DisabledSuffix);
				else
					headerLabel.setText(rule.getComments());
				
				updateBackground();
				
				condition.replaceClientArea();
				action.replaceClientArea();
				editor.updateEditorAreaLayout();
				editor.setModified(true);
			}
		}
	}

	/**
	 * @return the rule
	 */
	public EventProcessingPolicyRule getRule()
	{
		return rule;
	}

	/**
	 * @return the modified
	 */
	public boolean isModified()
	{
		return modified;
	}

	/**
	 * @param modified the modified to set
	 */
	public void setModified(boolean modified)
	{
		this.modified = modified;
	}

	/**
	 * @return the editor
	 */
	public EventProcessingPolicyEditor getEditorView()
	{
		return editor;
	}
	
	/**
	 * Process mouse click on rule number
	 * 
	 * @param e mouse event
	 */
	private void processRuleMouseEvent(MouseEvent e)
	{
		boolean ctrlPressed = (e.stateMask & SWT.CTRL) != 0;
		boolean shiftPressed = (e.stateMask & SWT.SHIFT) != 0;
		
		if (ctrlPressed)
		{
			editor.addToSelection(this, false);
		}
		else if (shiftPressed)
		{
			editor.addToSelection(this, true);
		}
		else
		{
			editor.setSelection(this);
		}
	}

	/**
	 * @return the selected
	 */
	public boolean isSelected()
	{
		return selected;
	}

	/**
	 * Set selection status
	 * 
	 * @param selected the selected to set
	 */
	public void setSelected(boolean selected)
	{
		this.selected = selected;
		final Color color = SharedColors.getColor(selected ? SharedColors.RULE_EDITOR_SELECTED_TITLE_BACKGROUND : (rule.isDisabled() ? SharedColors.RULE_EDITOR_DISABLED_TITLE_BACKGROUND : SharedColors.RULE_EDITOR_NORMAL_TITLE_BACKGROUND), getDisplay());
		
		leftPanel.setBackground(color);
		for(Control c : leftPanel.getChildren())
			c.setBackground(color);
		leftPanel.redraw();

		header.setBackground(color);
		headerLabel.setBackground(color);
		expandButton.setBackground(color);
		editButton.setBackground(color);
		header.redraw();
	}
	
	/**
	 * Enable or disable rule
	 * 
	 * @param enabled true to enable rule
	 */
	public void enableRule(boolean enabled)
	{
		if (enabled == !rule.isDisabled())
			return;
		
		if (enabled)
		{
			rule.setFlags(rule.getFlags() & ~EventProcessingPolicyRule.DISABLED);
			headerLabel.setText(rule.getComments());
		}
		else
		{
			rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.DISABLED);
			headerLabel.setText(rule.getComments() + Messages.get().RuleEditor_DisabledSuffix);
		}
		
		modified = true;
		updateBackground();
		editor.setModified(true);
	}
	
	/**
	 * Update background color after rule status change
	 */
	private void updateBackground()
	{
		final Color color = SharedColors.getColor(selected ? SharedColors.RULE_EDITOR_SELECTED_TITLE_BACKGROUND : (rule.isDisabled() ? SharedColors.RULE_EDITOR_DISABLED_TITLE_BACKGROUND : SharedColors.RULE_EDITOR_NORMAL_TITLE_BACKGROUND), getDisplay());
		leftPanel.setBackground(color);
		for(Control c : leftPanel.getChildren())
			c.setBackground(color);
		header.setBackground(color);
		headerLabel.setBackground(color);
		expandButton.setBackground(color);
		editButton.setBackground(color);
	}
}
