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
package org.netxms.ui.eclipse.serverconfig.widgets.helpers;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
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
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.eventmanager.widgets.EventSelector;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.serverconfig.widgets.LogParserEditor;
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
	private LabeledText regexp;
	private LabeledText severity;
	private LabeledText facility;
	private LabeledText tag;
	private LabeledText activeContext;
	private EventSelector event;
	private Spinner eventParamCount;
	private LabeledText context;
	private Combo contextAction;
	private Combo contextResetMode;

	/**
	 * @param parent
	 * @param style
	 */
	public LogParserRuleEditor(Composite parent, FormToolkit toolkit, LogParserRule rule, LogParserEditor editor)
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
		
		regexp = new LabeledText(area, SWT.NONE);
		toolkit.adapt(regexp);
		regexp.setLabel(Messages.get().LogParserRuleEditor_MatchingRegExp);
		regexp.setText(rule.getMatch());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		regexp.setLayoutData(gd);
		regexp.getTextControl().addModifyListener(listener);

		severity = new LabeledText(area, SWT.NONE);
		toolkit.adapt(severity);
		severity.setLabel(Messages.get().LogParserRuleEditor_Severity);
		severity.setText((rule.getSeverity() != null) ? Integer.toString(rule.getSeverity()) : ""); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		severity.setLayoutData(gd);
		severity.getTextControl().addModifyListener(listener);
		
		facility = new LabeledText(area, SWT.NONE);
		toolkit.adapt(facility);
		facility.setLabel(Messages.get().LogParserRuleEditor_Facility);
		facility.setText((rule.getFacility() != null) ? Integer.toString(rule.getFacility()) : ""); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		facility.setLayoutData(gd);
		facility.getTextControl().addModifyListener(listener);

		tag = new LabeledText(area, SWT.NONE);
		toolkit.adapt(tag);
		tag.setLabel(Messages.get().LogParserRuleEditor_SyslogTag);
		tag.setText((rule.getTag() != null) ? rule.getTag() : ""); //$NON-NLS-1$
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
		
		return area;
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
				EventTemplate t = ((NXCSession)ConsoleSharedData.getSession()).findEventTemplateByName(rule.getEvent().getEvent());
				if (t != null)
					eventCode = t.getCode();
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
				eventParamCount.setEnabled(event.getEventCode() != 0);
				editor.fireModifyListeners();
			}
		});
		
		final WidgetFactory factory = new WidgetFactory() {
			@Override
			public Control createControl(Composite parent, int style)
			{
				return new Spinner(parent, style);
			}
		};
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		eventParamCount = (Spinner)WidgetHelper.createLabeledControl(area, SWT.BORDER, factory, Messages.get().LogParserRuleEditor_Parameters, gd);
		toolkit.adapt(eventParamCount);
		eventParamCount.setMinimum(0);
		eventParamCount.setMaximum(32);
		if (rule.getEvent() != null)
		{
			eventParamCount.setSelection(rule.getEvent().getParameterCount());
		}
		else
		{
			eventParamCount.setSelection(0);
			eventParamCount.setEnabled(false);
		}
		eventParamCount.addModifyListener(new ModifyListener() {
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
		
		return area;
	}
	
	/**
	 * Save data from controls into parser rule
	 */
	public void save()
	{
		rule.setMatch(regexp.getText());
		rule.setFacility(intOrNull(facility.getText()));
		rule.setSeverity(intOrNull(severity.getText()));
		rule.setTag(tag.getText().trim().isEmpty() ? null : tag.getText());
		rule.setContext(activeContext.getText().trim().isEmpty() ? null : activeContext.getText());
		if (event.getEventCode() != 0)
		{
			rule.setEvent(new LogParserEvent(Long.toString(event.getEventCode()), eventParamCount.getSelection()));
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
}
