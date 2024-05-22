/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.base.widgets;

import java.util.Calendar;
import java.util.Date;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.DateTime;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Date/time selection widget
 */
public class DateTimeSelector extends Composite
{
	private DateTime datePicker;
	private DateTime timePicker;

	/**
	 * @param parent
	 * @param style
	 */
	public DateTimeSelector(Composite parent, int style)
	{
		super(parent, style);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		setLayout(layout);

      datePicker = new DateTime(this, SWT.DATE | SWT.BORDER);
      datePicker.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
      datePicker.setFont(JFaceResources.getDialogFont());
      timePicker = new DateTime(this, SWT.TIME | SWT.BORDER);
      timePicker.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
      timePicker.setFont(JFaceResources.getDialogFont());
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

   /**
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
