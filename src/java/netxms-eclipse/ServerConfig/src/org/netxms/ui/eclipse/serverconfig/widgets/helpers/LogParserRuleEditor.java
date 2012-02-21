/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.ui.eclipse.eventmanager.widgets.EventSelector;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.DashboardComposite;
import org.netxms.ui.eclipse.widgets.DashboardElement;
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
	private LabeledText regexp;
	private LabeledText severity;
	private LabeledText facility;
	private LabeledText tag;
	private EventSelector event;
	private Spinner eventParamCount;
	private LabeledText context;

	/**
	 * @param parent
	 * @param style
	 */
	public LogParserRuleEditor(Composite parent, FormToolkit toolkit, LogParserRule rule)
	{
		super(parent, SWT.BORDER);
		
		this.toolkit = toolkit;
		this.rule = rule;
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		setLayout(layout);
		
		final DashboardElement condition = new DashboardElement(this, "Condition") {
			@Override
			protected Control createClientArea(Composite parent)
			{
				setBorderColor(CONDITION_BORDER_COLOR);
				setTitleColor(TITLE_COLOR);
				return createMatchingArea(parent);
			}
		};
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		condition.setLayoutData(gd);

		final DashboardElement action = new DashboardElement(this, "Action") {
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
	 * Create matching area
	 */
	private Composite createMatchingArea(Composite parent)
	{
		Composite area = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		area.setLayout(layout);
		
		regexp = new LabeledText(area, SWT.NONE);
		toolkit.adapt(regexp);
		regexp.setLabel("Matching regular expression");
		regexp.setText(rule.getMatch());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		regexp.setLayoutData(gd);

		severity = new LabeledText(area, SWT.NONE);
		toolkit.adapt(severity);
		severity.setLabel("Severity");
		severity.setText(Integer.toString(rule.getSeverity()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		severity.setLayoutData(gd);
		
		facility = new LabeledText(area, SWT.NONE);
		toolkit.adapt(facility);
		facility.setLabel("Facility");
		facility.setText(Integer.toString(rule.getFacility()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		facility.setLayoutData(gd);

		tag = new LabeledText(area, SWT.NONE);
		toolkit.adapt(tag);
		tag.setLabel("Syslog tag");
		tag.setText(rule.getTag());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		tag.setLayoutData(gd);
		
		return area;
	}
	
	/**
	 * Create action area
	 */
	private Composite createActionArea(Composite parent)
	{
		Composite area = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		area.setLayout(layout);
		
		long eventCode = 0;
		try
		{
			eventCode = Long.parseLong(rule.getEvent().getEvent());
		}
		catch(NumberFormatException e)
		{
			// TODO: find event code by name
			//((NXCSession)ConsoleSharedData.getSession()).findE
		}
		event = new EventSelector(area, SWT.NONE, true);
		toolkit.adapt(event);
		event.setLabel("Generate event");
		event.setEventCode(eventCode);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		event.setLayoutData(gd);
		
		final WidgetFactory factory = new WidgetFactory() {
			@Override
			public Control createControl(Composite parent, int style)
			{
				return new Spinner(parent, style);
			}
		};
		eventParamCount = (Spinner)WidgetHelper.createLabeledControl(area, SWT.BORDER, factory, "Parameters", WidgetHelper.DEFAULT_LAYOUT_DATA);
		eventParamCount.setMinimum(0);
		eventParamCount.setMaximum(32);
		eventParamCount.setSelection(rule.getEvent().getParameterCount());
		
		context = new LabeledText(area, SWT.NONE);
		toolkit.adapt(context);
		context.setLabel("Activate context");
		context.setText(rule.getContextDefinition().getData());
		
		return area;
	}
	
	/**
	 * Save data from controls into parser rule
	 */
	public void save()
	{
		rule.setMatch(regexp.getText());
		rule.setFacility(Integer.parseInt(facility.getText()));
		rule.setSeverity(Integer.parseInt(severity.getText()));
		rule.setTag(tag.getText());
		rule.getEvent().setEvent(Long.toString(event.getEventCode()));
		rule.getEvent().setParameterCount(eventParamCount.getSelection());
	}
}
