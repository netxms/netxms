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
package org.netxms.ui.eclipse.widgets;

import java.util.Calendar;
import java.util.Date;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.DateTime;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Date/time selection widget
 */
public class DateTimeSelector extends Composite
{
	private static final long serialVersionUID = 1L;

	private DateTime datePicker;
	private DateTime timePicker;
	
	/**
	 * @param parent
	 * @param style
	 */
	public DateTimeSelector(Composite parent, int style)
	{
		super(parent, style);
		RowLayout layout = new RowLayout();
		layout.type = SWT.HORIZONTAL;
		layout.marginBottom = 0;
		layout.marginTop = 0;
		layout.marginLeft = 0;
		layout.marginRight = 0;
		layout.spacing = WidgetHelper.INNER_SPACING;
		setLayout(layout);

		datePicker = new DateTime(this, SWT.DATE | SWT.DROP_DOWN);
		timePicker = new DateTime(this, SWT.TIME);
	}

	/**
	 * @return the timestamp
	 */
	public Date getValue()
	{
		final Calendar c = Calendar.getInstance();
		c.clear();
		c.set(datePicker.getYear(), datePicker.getMonth(), datePicker.getDay(),
				timePicker.getHours(), timePicker.getMinutes(), timePicker.getSeconds());
		return c.getTime();
	}

	/**
	 * @param timestamp the timestamp to set
	 */
	public void setValue(Date date)
	{
		if (date == null)
			return;
		
		final Calendar c = Calendar.getInstance();
		c.setTime(date);
		datePicker.setDate(c.get(Calendar.YEAR), c.get(Calendar.MONTH), c.get(Calendar.DAY_OF_MONTH));
		timePicker.setTime(c.get(Calendar.HOUR_OF_DAY), c.get(Calendar.MINUTE), c.get(Calendar.SECOND));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
	 */
	@Override
	public void setEnabled(boolean enabled)
	{
		super.setEnabled(enabled);
		datePicker.setEnabled(enabled);
		timePicker.setEnabled(enabled);
	}
}
