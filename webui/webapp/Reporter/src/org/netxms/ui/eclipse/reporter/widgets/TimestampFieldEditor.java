/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.reporter.widgets;

import java.util.Calendar;
import java.util.Date;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DateTime;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Editor for timestamp parameters
 */
public class TimestampFieldEditor extends FieldEditor
{
	public enum Type
	{
		START_DATE, END_DATE, TIMESTAMP;
	};

	private DateTime datePicker;
	private DateTime timePicker;
	private Type type;

	/**
	 * @param parameter
	 * @param toolkit
	 * @param parent
	 */
	public TimestampFieldEditor(ReportParameter parameter, FormToolkit toolkit, Composite parent)
	{
		super(parameter, toolkit, parent);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#createContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContent(Composite parent)
	{
		parseType();

		final Composite area = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		area.setLayout(layout);

		Date date;
		try
		{
			date = new Date(Long.parseLong(parameter.getDefaultValue()) * 1000);
		}
		catch(NumberFormatException e)
		{
			date = new Date();
		}

		final Calendar c = Calendar.getInstance();
		c.setTime(date);

		datePicker = new DateTime(area, SWT.DATE | SWT.DROP_DOWN | SWT.BORDER);
		toolkit.adapt(datePicker);
		datePicker.setDate(c.get(Calendar.YEAR), c.get(Calendar.MONTH), c.get(Calendar.DAY_OF_MONTH));

		if (type == Type.TIMESTAMP)
		{
			timePicker = new DateTime(area, SWT.TIME | SWT.BORDER);
			toolkit.adapt(timePicker);
			timePicker.setTime(c.get(Calendar.HOUR_OF_DAY), c.get(Calendar.MINUTE), c.get(Calendar.SECOND));
		}
		
		return area;
	}

	/**
	 * 
	 */
	private void parseType()
	{
		final String parameterType = parameter.getType();
		if (parameterType.equals("START_DATE")) //$NON-NLS-1$
		{
			type = Type.START_DATE;
		}
		else if (parameterType.equals("END_DATE")) //$NON-NLS-1$
		{
			type = Type.END_DATE;
		}
		else
		{
			type = Type.TIMESTAMP;
		}
	}

	@Override
	public String getValue()
	{
		final Calendar calendar = Calendar.getInstance();
		calendar.clear();
		switch(type)
		{
			case START_DATE:
				calendar.set(datePicker.getYear(), datePicker.getMonth(), datePicker.getDay(), 0, 0, 0);
				break;
			case END_DATE:
				calendar.set(datePicker.getYear(), datePicker.getMonth(), datePicker.getDay(), 23, 59, 59);
				break;
			case TIMESTAMP:
				calendar.set(datePicker.getYear(), datePicker.getMonth(), datePicker.getDay(), timePicker.getHours(),
						timePicker.getMinutes(), timePicker.getSeconds());
				break;
		}

		return Long.toString(calendar.getTimeInMillis() / 1000);
	}
}
