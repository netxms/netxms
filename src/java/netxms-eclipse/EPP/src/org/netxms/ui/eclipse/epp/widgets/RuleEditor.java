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
package org.netxms.ui.eclipse.epp.widgets;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.window.Window;
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
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.epp.dialogs.EditRuleActionsDlg;
import org.netxms.ui.eclipse.epp.views.EventProcessingPolicyEditor;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.DashboardElement;
import org.netxms.ui.eclipse.widgets.helpers.DashboardElementButton;

/**
 * Rule overview widget
 *
 */
public class RuleEditor extends Composite
{
	private static final Color BACKGROUND_COLOR = new Color(Display.getDefault(), 255, 255, 255);
	private static final Color TITLE_BACKGROUND_COLOR = new Color(Display.getDefault(), 225, 233, 241);
	private static final Color RULE_BORDER_COLOR = new Color(Display.getDefault(), 153, 180, 209);
	private static final Color CONDITION_BORDER_COLOR = new Color(Display.getDefault(), 198,214,172);
	private static final Color ACTION_BORDER_COLOR = new Color(Display.getDefault(), 186,176,201);
	private static final Color TITLE_COLOR = new Color(Display.getDefault(), 0, 0, 0);
	private static final int INDENT = 20;
	
	private EventProcessingPolicyRule rule;
	private int ruleNumber;
	private EventProcessingPolicyEditor editor;
	private NXCSession session;
	private WorkbenchLabelProvider labelProvider;
	private boolean collapsed = true;
	private boolean verticalLayout = false;
	private Composite leftPanel;
	private Composite mainArea;
	private DashboardElement condition;
	private DashboardElement action;
	private Label expandButton;
	
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
		
		setBackground(RULE_BORDER_COLOR);

		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.marginHeight = 1;
		layout.marginWidth = 0;
		layout.horizontalSpacing = 1;
		layout.verticalSpacing = 1;
		setLayout(layout);
		
		createLeftPanel();
		createHeader();
		createMainArea();
		
		condition = new DashboardElement(mainArea, "Filter") {
			@Override
			protected Control createClientArea(Composite parent)
			{
				setBorderColor(CONDITION_BORDER_COLOR);
				setTitleColor(TITLE_COLOR);
				return createConditionControl(parent, RuleEditor.this.rule);
			}
		};
		configureLayout(condition);

		action = new DashboardElement(mainArea, "Action") {
			@Override
			protected Control createClientArea(Composite parent)
			{
				setBorderColor(ACTION_BORDER_COLOR);
				setTitleColor(TITLE_COLOR);
				return createActionControl(parent, RuleEditor.this.rule);
			}
		};
		configureLayout(action);
		final Action editRuleAction = new Action() {
			@Override
			public void run()
			{
				editActions();
			}
		};
		action.addButton(new DashboardElementButton("Edit actions", editor.getImageEdit(), editRuleAction));
		action.setDoubleClickAction(editRuleAction);
	}
	
	/**
	 * Create main area which contains condition and action elements
	 */
	private void createMainArea()
	{
		mainArea = new Composite(this, SWT.NONE);
		mainArea.setBackground(BACKGROUND_COLOR);
		
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
		leftPanel.setBackground(TITLE_BACKGROUND_COLOR);
		
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
		
		Label label = new Label(leftPanel, SWT.NONE);
		label.setText(Integer.toString(ruleNumber));
		label.setFont(editor.getBoldFont());
		label.setBackground(TITLE_BACKGROUND_COLOR);
		label.setForeground(TITLE_COLOR);
		label.setAlignment(SWT.CENTER);
		
		gd = new GridData();
		gd.horizontalAlignment = SWT.CENTER;
		gd.verticalAlignment = SWT.CENTER;
		gd.grabExcessVerticalSpace = true;
		gd.widthHint = 30;
		label.setLayoutData(gd);
	}
	
	/**
	 * Create rule header
	 */
	private void createHeader()
	{
		Composite header = new Composite(this, SWT.NONE);
		header.setBackground(TITLE_BACKGROUND_COLOR);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		header.setLayout(layout);
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		header.setLayoutData(gd);
		
		Label label = new Label(header, SWT.NONE);
		label.setText(rule.getComments());
		label.setBackground(TITLE_BACKGROUND_COLOR);
		label.setForeground(TITLE_COLOR);
		label.setFont(editor.getNormalFont());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		label.setLayoutData(gd);
		label.addMouseListener(new MouseListener() {
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
				setCollapsed(!isCollapsed(), true);
			}

			@Override
			public void mouseDown(MouseEvent e)
			{
			}

			@Override
			public void mouseUp(MouseEvent e)
			{
			}
		});
		
		expandButton = new Label(header, SWT.NONE);
		expandButton.setBackground(TITLE_BACKGROUND_COLOR);
		expandButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
		expandButton.setImage(collapsed ? editor.getImageExpand() : editor.getImageCollapse());
		expandButton.addMouseListener(new MouseListener() {
			private boolean doAction = false;
			
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
				doAction = false;
			}

			@Override
			public void mouseDown(MouseEvent e)
			{
				doAction = true;
			}

			@Override
			public void mouseUp(MouseEvent e)
			{
				if (doAction)
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
	 * Create condition summary control
	 * 
	 * @param parent
	 * @param rule
	 * @return
	 */
	private Control createConditionControl(Composite parent, final EventProcessingPolicyRule rule)
	{
		Composite clientArea = new Composite(parent, SWT.NONE);
		
		clientArea.setBackground(BACKGROUND_COLOR);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = 0;
		clientArea.setLayout(layout);
		
		boolean needAnd = false;
		createLabel(clientArea, 0, true, "IF");
		
		/* events */
		if (rule.getEvents().size() > 0)
		{
			addConditionGroupLabel(clientArea, "event code is one of the following:", needAnd);
			
			for(Long code : rule.getEvents())
			{
				CLabel clabel = createCLabel(clientArea, 2, false);
				
				EventTemplate event = session.findEventTemplateByCode(code);
				if (event != null)
				{
					clabel.setText(event.getName());
					clabel.setImage(StatusDisplayInfo.getStatusImage(event.getSeverity()));
				}
				else
				{
					clabel.setText("<" + code.toString() + ">");
					clabel.setImage(StatusDisplayInfo.getStatusImage(Severity.UNKNOWN));
				}
			}
			needAnd = true;
		}
		
		/* source */
		if (rule.getSources().size() > 0)
		{
			addConditionGroupLabel(clientArea, "source object is one of the following:", needAnd);
			
			for(Long id : rule.getSources())
			{
				CLabel clabel = createCLabel(clientArea, 2, false);
				
				GenericObject object = session.findObjectById(id);
				if (object != null)
				{
					clabel.setText(object.getObjectName());
					clabel.setImage(labelProvider.getImage(object));
				}
				else
				{
					clabel.setText("[" + id.toString() + "]");
				}
			}
			needAnd = true;
		}
		
		/* severity */
		if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_ANY) != EventProcessingPolicyRule.SEVERITY_ANY)
		{
			addConditionGroupLabel(clientArea, "event severity is one of the following:", needAnd);
			
			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_NORMAL) != 0)
				addSeverityLabel(clientArea, Severity.NORMAL);

			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_WARNING) != 0)
				addSeverityLabel(clientArea, Severity.WARNING);

			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_MINOR) != 0)
				addSeverityLabel(clientArea, Severity.MINOR);

			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_MAJOR) != 0)
				addSeverityLabel(clientArea, Severity.MAJOR);

			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_CRITICAL) != 0)
				addSeverityLabel(clientArea, Severity.CRITICAL);
			
			needAnd = true;
		}
		
		/* script */
		if ((rule.getScript() != null) && !rule.getScript().isEmpty())
		{
			addConditionGroupLabel(clientArea, "the following script returns true:", needAnd);
			
			ScriptEditor scriptEditor = new ScriptEditor(clientArea, SWT.BORDER);
			GridData gd = new GridData();
			gd.horizontalIndent = INDENT * 2;
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			scriptEditor.setLayoutData(gd);
			scriptEditor.setText(rule.getScript());
			scriptEditor.getTextWidget().setEditable(false);
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
	private Label createLabel(Composite parent, int indent, boolean bold, String text)
	{
		Label label = new Label(parent, SWT.NONE);
		label.setBackground(BACKGROUND_COLOR);
		label.setFont(bold ? editor.getBoldFont() : editor.getNormalFont());
		label.setText(text);

		GridData gd = new GridData();
		gd.horizontalIndent = INDENT * indent;
		label.setLayoutData(gd);
		
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
		label.setBackground(BACKGROUND_COLOR);
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
	private void addConditionGroupLabel(Composite parent, String title, boolean needAnd)
	{
		if (needAnd)
			createLabel(parent, 0, true, "AND");
		createLabel(parent, 1, false, title);
	}
	
	/**
	 * Add severity label
	 * 
	 * @param parent parent composite
	 * @param severity severity code
	 */
	private void addSeverityLabel(Composite parent, int severity)
	{
		CLabel clabel = createCLabel(parent, 2, false);
		clabel.setText(StatusDisplayInfo.getStatusText(severity));
		clabel.setImage(StatusDisplayInfo.getStatusImage(severity));
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
		
		clientArea.setBackground(BACKGROUND_COLOR);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = 0;
		clientArea.setLayout(layout);
		
		/* alarm */
		if ((rule.getFlags() & EventProcessingPolicyRule.GENERATE_ALARM) != 0)
		{
			if (rule.getAlarmSeverity() < Severity.UNMANAGED)
			{
				addActionGroupLabel(clientArea, "Generate alarm", editor.getImageAlarm());
				
				CLabel clabel = createCLabel(clientArea, 1, false);
				clabel.setImage(StatusDisplayInfo.getStatusImage(rule.getAlarmSeverity()));
				clabel.setText(rule.getAlarmMessage());
				
				if ((rule.getAlarmKey() != null) && !rule.getAlarmKey().isEmpty())
				{
					createLabel(clientArea, 1, false, "with key \"" + rule.getAlarmKey() + "\"");
				}
			}
			else
			{
				addActionGroupLabel(clientArea, "Terminate alarms with key \"" + rule.getAlarmKey() + "\"", editor.getImageTerminate());
			}
		}
		
		/* actions */
		if (rule.getActions().size() > 0)
		{
			addActionGroupLabel(clientArea, "Execute the following predefined actions:", editor.getImageExecute());
			for(Long id : rule.getActions())
			{
				CLabel clabel = createCLabel(clientArea, 1, false);
				ServerAction action = editor.findActionById(id);
				if (action != null)
				{
					clabel.setText(action.getName());
					clabel.setImage(labelProvider.getImage(action));
				}
				else
				{
					clabel.setText("<" + id.toString() + ">");
				}
			}
		}
		
		/* flags */
		if ((rule.getFlags() & EventProcessingPolicyRule.STOP_PROCESSING) != 0)
		{
			addActionGroupLabel(clientArea, "Stop event processing", editor.getImageStop());
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
	private void addActionGroupLabel(Composite parent, String title, Image image)
	{
		CLabel label = createCLabel(parent, 0, true);
		label.setImage(image);
		label.setText(title);
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
	protected int getRuleNumber()
	{
		return ruleNumber;
	}

	/**
	 * @param ruleNumber the ruleNumber to set
	 */
	protected void setRuleNumber(int ruleNumber)
	{
		this.ruleNumber = ruleNumber;
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
		mainArea.setVisible(!collapsed);
		((GridData)mainArea.getLayoutData()).exclude = collapsed;
		((GridData)leftPanel.getLayoutData()).verticalSpan = collapsed ? 1 : 2;
		if (doLayout)
			editor.updateEditorAreaLayout();
	}
	
	/**
	 * Edit actions
	 */
	private void editActions()
	{
		EditRuleActionsDlg dlg = new EditRuleActionsDlg(getShell(), rule);
		if (dlg.open() == Window.OK)
		{
			
		}
	}
}
