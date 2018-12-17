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
package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventObject;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.widgets.LogParserEditor;
import org.netxms.ui.eclipse.eventmanager.widgets.EventSelector;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.CGroup;
import org.netxms.ui.eclipse.widgets.DashboardComposite;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Editor widget for single log parser rule
 */
public class LogParserRuleEditor extends DashboardComposite
{
	private static final Color CONDITION_BORDER_COLOR = new Color(Display.getCurrent(), 198,214,172);
	private static final Color ACTION_BORDER_COLOR = new Color(Display.getCurrent(), 186,176,201);
	private static final Color TITLE_COLOR = new Color(Display.getCurrent(), 0, 0, 0);
	
	private FormToolkit toolkit;
	private LogParserRule rule;
	private LogParserEditor editor;
	private LabeledText name;
	private LabeledText regexp;
	private Button checkboxInvert;
   private Button checkboxReset;
   private Spinner repeatCount;
   private Spinner timeRange;
   private Combo timeUnits;
	private LabeledText severity;
	private LabeledText facility;
	private LabeledText tag;
	private LabeledText activeContext;
	private LabeledText description;
	private LabeledText agentAction;
	private EventSelector event;
	private LabeledText context;
	private Combo contextAction;
	private Combo contextResetMode;
	private Button checkboxBreak;

	/**
	 * @param parent
	 * @param style
	 */
	public LogParserRuleEditor(Composite parent, FormToolkit toolkit, LogParserRule rule, final LogParserEditor editor)
	{
		super(parent, SWT.BORDER);
		
		this.toolkit = toolkit;
		this.rule = rule;
		this.editor = editor;
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		setLayout(layout);
		
		final Composite controlBar = new Composite(this, SWT.NONE);
		controlBar.setLayout(new RowLayout(SWT.HORIZONTAL));
		GridData gd = new GridData();
		gd.horizontalSpan = 2;
		gd.horizontalAlignment = SWT.RIGHT;
		controlBar.setLayoutData(gd);
		fillControlBar(controlBar);
		
		name = new LabeledText(this, SWT.NONE);
		name.setLabel("Name");
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      name.setLayoutData(gd);
      name.setText((rule.getName() != null) ? rule.getName() : "");
      name.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            editor.fireModifyListeners();
         }
      });
		
		final CGroup condition = new CGroup(this, Messages.get().LogParserRuleEditor_Condition) {
			@Override
			protected Control createClientArea(Composite parent)
			{
				setBorderColor(CONDITION_BORDER_COLOR);
				setTitleColor(TITLE_COLOR);
				return createMatchingArea(parent);
			}
		};
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		condition.setLayoutData(gd);

		final CGroup action = new CGroup(this, Messages.get().LogParserRuleEditor_Action) {
			@Override
			protected Control createClientArea(Composite parent)
			{
				setBorderColor(ACTION_BORDER_COLOR);
				setTitleColor(TITLE_COLOR);
				return createActionArea(parent);
			}
		};
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		action.setLayoutData(gd);
	}
	
	/**
	 * Fill control bar
	 * 
	 * @param parent
	 */
	private void fillControlBar(Composite parent)
	{
		ImageHyperlink link = toolkit.createImageHyperlink(parent, SWT.NONE);
		link.setImage(SharedIcons.IMG_UP);
		link.setToolTipText(Messages.get().LogParserRuleEditor_MoveUp);
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				editor.moveRuleUp(rule);
			}
		});
		
		link = toolkit.createImageHyperlink(parent, SWT.NONE);
		link.setImage(SharedIcons.IMG_DOWN);
		link.setToolTipText(Messages.get().LogParserRuleEditor_MoveDown);
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				editor.moveRuleDown(rule);
			}
		});
		
		link = toolkit.createImageHyperlink(parent, SWT.NONE);
		link.setImage(SharedIcons.IMG_DELETE_OBJECT);
		link.setToolTipText(Messages.get().LogParserRuleEditor_DeleteRule);
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				editor.deleteRule(rule);
			}
		});
	}

	/**
	 * Create matching area
	 */
	private Composite createMatchingArea(Composite parent)
	{
		Composite area = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		area.setLayout(layout);
		
		final ModifyListener listener = new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				editor.fireModifyListeners();
			}
		};
		
		Composite matcher = new Composite(area, SWT.NONE);
      toolkit.adapt(matcher);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      matcher.setLayoutData(gd);
      
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginWidth = 0;
      matcher.setLayout(layout);
		
		regexp = new LabeledText(matcher, SWT.NONE);
		regexp.setLabel(Messages.get().LogParserRuleEditor_MatchingRegExp);
		regexp.setText(rule.getMatch().getMatch());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		regexp.setLayoutData(gd);
		regexp.getTextControl().addModifyListener(listener);
		
		checkboxInvert = toolkit.createButton(matcher, "Invert", SWT.CHECK);
		checkboxInvert.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editor.fireModifyListeners();
         }
      });          
		checkboxInvert.setSelection(rule.getMatch().getInvert());
		
      Composite matcherRepeatConf = new Composite(matcher, SWT.NONE);
      toolkit.adapt(matcherRepeatConf);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;      
      matcherRepeatConf.setLayoutData(gd);
      
      layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginWidth = 0;
      matcherRepeatConf.setLayout(layout);      

      final WidgetFactory factory = new WidgetFactory() {
         @Override
         public Control createControl(Composite parent, int style)
         {
            return new Spinner(parent, style);
         }
      };
      
      repeatCount = (Spinner)WidgetHelper.createLabeledControl(matcherRepeatConf, SWT.BORDER, factory, "Repeat count", WidgetHelper.DEFAULT_LAYOUT_DATA);
      toolkit.adapt(repeatCount);
      repeatCount.setMinimum(0);
      repeatCount.setSelection(rule.getMatch().getRepeatCount());
      repeatCount.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            editor.fireModifyListeners();
         }
      });
      
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      repeatCount.setLayoutData(gd); 
      Composite timeBackGroup = new Composite(matcherRepeatConf, SWT.NONE);
      toolkit.adapt(timeBackGroup);
      layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      timeBackGroup.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      timeBackGroup.setLayoutData(gd);
      
      timeRange = WidgetHelper.createLabeledSpinner(timeBackGroup, SWT.BORDER, "Repeat interval", 1, 10000, WidgetHelper.DEFAULT_LAYOUT_DATA);
      timeRange.setSelection(rule.getMatch().getTimeRagne()); 
      toolkit.adapt(timeRange);
      
      timeUnits = WidgetHelper.createLabeledCombo(timeBackGroup, SWT.READ_ONLY, "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      timeUnits.add("Seconds");
      timeUnits.add("Minutes");
      timeUnits.add("Hours");
      timeUnits.select(rule.getMatch().getTimeUnit()); 
      toolkit.adapt(timeUnits);
      //time range

      checkboxReset = toolkit.createButton(matcherRepeatConf, "Reset repeat count", SWT.CHECK);
      checkboxReset.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editor.fireModifyListeners();
         }
      });          
      checkboxReset.setSelection(rule.getMatch().getReset());

		severity = new LabeledText(area, SWT.NONE);
		toolkit.adapt(severity);
		severity.setLabel(editor.isSyslogParser() ? Messages.get().LogParserRuleEditor_Severity : "Level");
		severity.setText(rule.getSeverityOrLevel(editor.isSyslogParser())); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		severity.setLayoutData(gd);
		severity.getTextControl().addModifyListener(listener);
		
		facility = new LabeledText(area, SWT.NONE);
		toolkit.adapt(facility);
		facility.setLabel(editor.isSyslogParser() ? Messages.get().LogParserRuleEditor_Facility : "Id");
		facility.setText(rule.getFacilityOrId(editor.isSyslogParser())); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		facility.setLayoutData(gd);
		facility.getTextControl().addModifyListener(listener);

		tag = new LabeledText(area, SWT.NONE);
		toolkit.adapt(tag);
		tag.setLabel(editor.isSyslogParser() ? Messages.get().LogParserRuleEditor_SyslogTag : "Source");
		tag.setText(rule.getTagOrSource(editor.isSyslogParser())); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		tag.setLayoutData(gd);
		tag.getTextControl().addModifyListener(listener);		
		
		activeContext = new LabeledText(area, SWT.NONE);
		toolkit.adapt(activeContext);
		activeContext.setLabel(Messages.get().LogParserRuleEditor_ActiveContext);
		activeContext.setText((rule.getContext() != null) ? rule.getContext() : ""); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		activeContext.setLayoutData(gd);
		activeContext.getTextControl().addModifyListener(listener);
		
		description = new LabeledText(area, SWT.NONE);
      toolkit.adapt(description);
      description.setLabel("Description");
      description.setText((rule.getDescription() != null) ? rule.getDescription() : ""); //$NON-NLS-1$
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      description.setLayoutData(gd);
      description.getTextControl().addModifyListener(listener);
		
		updateWindowsEventLogFields();		
		return area;
	}
	
	/**
	 * Enables or disables Windows syslog fields depending on filename field
	 */
	public void updateWindowsEventLogFields()
	{
	   if(editor.isSyslogParser())
	      return;
	      
	   if(editor.isWindowsEventLogParser())
	   {
	      severity.setEnabled(true);
	      facility.setEnabled(true);
	      tag.setEnabled(true);
	   }
	   else
	   {
         severity.setEnabled(false);
         facility.setEnabled(false);
         tag.setEnabled(false);	      
	   }
	}
	
	/**
	 * Create action area
	 */
	private Composite createActionArea(Composite parent)
	{
		Composite area = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		area.setLayout(layout);
		
		long eventCode = 0;
		if (rule.getEvent() != null)
		{
			try
			{
				eventCode = Long.parseLong(rule.getEvent().getEvent());
			}
			catch(NumberFormatException e)
			{
				EventObject o = ((NXCSession)ConsoleSharedData.getSession()).findEventObjectByName(rule.getEvent().getEvent());
				if (o != null)
					eventCode = o.getCode();
			}
		}
		
		event = new EventSelector(area, SWT.NONE, true);
		toolkit.adapt(event);
		event.setLabel(Messages.get().LogParserRuleEditor_GenerateEvent);
		event.setEventCode(eventCode);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		event.setLayoutData(gd);
		event.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				editor.fireModifyListeners();
			}
		});
		
		final LogParserContext contextDefinition = rule.getContextDefinition();
		
		context = new LabeledText(area, SWT.NONE);
		toolkit.adapt(context);
		context.setLabel(Messages.get().LogParserRuleEditor_ChangeContext);
		context.setText((contextDefinition != null) ? contextDefinition.getData() : ""); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		context.setLayoutData(gd);
		context.getTextControl().addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				boolean contextSet = !context.getText().trim().isEmpty();
				contextAction.setEnabled(contextSet);
				contextResetMode.setEnabled(contextSet && (contextAction.getSelectionIndex() == 0));
				editor.fireModifyListeners();
			}
		});
		
		final Composite contextOptions = new Composite(area, SWT.NONE);
		layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		contextOptions.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		contextOptions.setLayoutData(gd);
		
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		contextAction = WidgetHelper.createLabeledCombo(contextOptions, SWT.READ_ONLY, Messages.get().LogParserRuleEditor_ContextAction, gd);
		toolkit.adapt(contextAction);
		contextAction.add(Messages.get().LogParserRuleEditor_CtxActionActivate);
		contextAction.add(Messages.get().LogParserRuleEditor_CtxActionClear);
		if (contextDefinition != null)
		{
			contextAction.select(contextDefinition.getAction());
		}
		else
		{
			contextAction.select(0);
			contextAction.setEnabled(false);
		}
		contextAction.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				contextResetMode.setEnabled(contextAction.getSelectionIndex() == 0);
				editor.fireModifyListeners();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		contextResetMode = WidgetHelper.createLabeledCombo(contextOptions, SWT.READ_ONLY, Messages.get().LogParserRuleEditor_ContextResetMode, gd);
		toolkit.adapt(contextResetMode);
		contextResetMode.add(Messages.get().LogParserRuleEditor_CtxModeAuto);
		contextResetMode.add(Messages.get().LogParserRuleEditor_CtxModeManual);
		if (contextDefinition != null)
		{
			contextResetMode.select(contextDefinition.getReset());
			contextResetMode.setEnabled(contextDefinition.getAction() == 0);
		}
		else
		{
			contextResetMode.select(0);
			contextResetMode.setEnabled(false);
		}
		contextResetMode.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				editor.fireModifyListeners();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		agentAction = new LabeledText(area, SWT.NONE);
		toolkit.adapt(agentAction);
		agentAction.setLabel("Execute action");
		agentAction.setText((rule.getAgentAction() != null) ? rule.getAgentAction().getAction() : ""); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		agentAction.setLayoutData(gd);
		
		checkboxBreak = toolkit.createButton(area, "Process all", SWT.CHECK);
		checkboxBreak.setText("Break");
		checkboxBreak.setSelection(rule.isBreakProcessing());
		
		return area;
	}
	
	/**
	 * Save data from controls into parser rule
	 */
	public void save()
	{
	   rule.setName(name.getText().trim());
		rule.setMatch(new LogParserMatch(regexp.getText(), checkboxInvert.getSelection(), intOrNull(repeatCount.getText()), 
		                                 Integer.parseInt(timeRange.getText()) *(timeUnits.getSelectionIndex() * 60), 
		                                 checkboxReset.getSelection()));
      rule.setFacilityOrId(intOrNull(facility.getText()));
      rule.setSeverityOrLevel(intOrNull(severity.getText()));
      rule.setTagOrSource(tag.getText());
		rule.setContext(activeContext.getText().trim().isEmpty() ? null : activeContext.getText());
		rule.setBreakProcessing(checkboxBreak.getSelection());
		rule.setDescription(description.getText());
		if (event.getEventCode() != 0)
		{
			rule.setEvent(new LogParserEvent(event.getEventName(), null));
		}
		else
		{
			rule.setEvent(null);
		}
		
		if (context.getText().trim().isEmpty())
		{
			rule.setContextDefinition(null);
		}
		else
		{
			LogParserContext ctx = new LogParserContext();
			ctx.setData(context.getText());
			ctx.setAction(contextAction.getSelectionIndex());
			ctx.setReset(contextResetMode.getSelectionIndex());
			rule.setContextDefinition(ctx);
		}
		
		if (!agentAction.getText().trim().isEmpty())
			rule.setAgentAction(agentAction.getText().trim());
	}
	
	/**
	 * Return integer object created from input string or null if input text string is empty or cannot be parsed
	 * 
	 * @param text
	 * @return
	 */
	private Integer intOrNull(String text)
	{
		if (text.trim().isEmpty())
			return null;
		try
		{
			return Integer.parseInt(text);
		}
		catch(NumberFormatException e)
		{
			return null;
		}
	}

   /**
    * @return
    */
   public boolean isSyslogParser()
   {
      return editor.isSyslogParser();
   }
}
