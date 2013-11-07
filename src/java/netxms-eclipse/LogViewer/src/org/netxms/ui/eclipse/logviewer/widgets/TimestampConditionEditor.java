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
package org.netxms.ui.eclipse.logviewer.widgets;

import java.util.Calendar;
import java.util.Date;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.DateTime;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.client.log.ColumnFilter;
import org.netxms.ui.eclipse.logviewer.Messages;

/**
 * Condition editor for timestamp columns
 */
public class TimestampConditionEditor extends ConditionEditor
{
	private static final String[] OPERATIONS = { Messages.TimestampConditionEditor_Between, Messages.TimestampConditionEditor_Before, Messages.TimestampConditionEditor_After };
	
	private DateTime datePicker1;
	private DateTime timePicker1;
	private DateTime datePicker2;
	private DateTime timePicker2;
	private Label andLabel;
	
	/**
	 * @param parent
	 * @param toolkit
	 * @param column
	 * @param parentElement
	 */
	public TimestampConditionEditor(Composite parent, FormToolkit toolkit)
	{
		super(parent, toolkit);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#getOperations()
	 */
	@Override
	protected String[] getOperations()
	{
		return OPERATIONS;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#createContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createContent(Composite parent)
	{
		Composite group = new Composite(this, SWT.NONE);
		RowLayout layout = new RowLayout();
		layout.type = SWT.HORIZONTAL;
		layout.marginBottom = 0;
		layout.marginTop = 0;
		layout.marginLeft = 0;
		layout.marginRight = 0;
		group.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		group.setLayoutData(gd);

		final Calendar c = Calendar.getInstance();
		c.setTime(new Date());

		datePicker1 = new DateTime(group, SWT.DATE | SWT.DROP_DOWN);
		datePicker1.setDate(c.get(Calendar.YEAR), c.get(Calendar.MONTH), c.get(Calendar.DAY_OF_MONTH));

		timePicker1 = new DateTime(group, SWT.TIME);
		timePicker1.setTime(c.get(Calendar.HOUR_OF_DAY), c.get(Calendar.MINUTE), c.get(Calendar.SECOND));
		
		andLabel = toolkit.createLabel(group, Messages.TimestampConditionEditor_And);

		datePicker2 = new DateTime(group, SWT.DATE | SWT.DROP_DOWN);
		datePicker2.setDate(c.get(Calendar.YEAR), c.get(Calendar.MONTH), c.get(Calendar.DAY_OF_MONTH));

		timePicker2 = new DateTime(group, SWT.TIME);
		timePicker2.setTime(c.get(Calendar.HOUR_OF_DAY), c.get(Calendar.MINUTE), c.get(Calendar.SECOND));
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#operationSelectionChanged(int)
	 */
	@Override
	protected void operationSelectionChanged(int selectionIndex)
	{
		if (selectionIndex == 0)	// between
		{
			andLabel.setVisible(true);
			datePicker2.setVisible(true);
			timePicker2.setVisible(true);
		}
		else
		{
			andLabel.setVisible(false);
			datePicker2.setVisible(false);
			timePicker2.setVisible(false);
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#createFilter()
	 */
	@Override
	public ColumnFilter createFilter()
	{
		final Calendar c = Calendar.getInstance();
		c.clear();
		c.set(datePicker1.getYear(), datePicker1.getMonth(), datePicker1.getDay(),
				timePicker1.getHours(), timePicker1.getMinutes(), timePicker1.getSeconds());
		final long timestamp = c.getTimeInMillis() / 1000;
		
		ColumnFilter filter;
		switch(getSelectedOperation())
		{
			case 0:	// between
				c.clear();
				c.set(datePicker2.getYear(), datePicker2.getMonth(), datePicker2.getDay(),
						timePicker2.getHours(), timePicker2.getMinutes(), timePicker2.getSeconds());
				filter = new ColumnFilter(timestamp, c.getTimeInMillis() / 1000);
				break;
			case 1:	// before
				filter = new ColumnFilter(ColumnFilter.LESS, timestamp);
				break;
			case 2:	// after
				filter = new ColumnFilter(ColumnFilter.GREATER, timestamp);
				break;
			default:
				filter = new ColumnFilter(timestamp, timestamp);
				break;
		}
		return filter;
	}
}
