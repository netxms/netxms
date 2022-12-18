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
package org.netxms.nxmc.modules.logviewer.widgets;

import java.util.Calendar;
import java.util.Date;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.DateTime;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.log.ColumnFilter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Condition editor for timestamp columns
 */
public class TimestampConditionEditor extends ConditionEditor
{
   private final I18n i18n = LocalizationHelper.getI18n(TimestampConditionEditor.class);
   private final String[] OPERATIONS = { i18n.tr("BETWEEN"), i18n.tr("BEFORE"), i18n.tr("AFTER"), i18n.tr("TODAY") };

	private DateTime datePicker1;
	private DateTime timePicker1;
	private DateTime datePicker2;
	private DateTime timePicker2;
	private Label andLabel;
	
	/**
	 * @param parent
	 */
   public TimestampConditionEditor(Composite parent)
	{
      super(parent);
	}

   /**
    * @see org.netxms.nxmc.modules.logviewer.widgets.ui.eclipse.logviewer.widgets.ConditionEditor#getOperations()
    */
	@Override
	protected String[] getOperations()
	{
		return OPERATIONS;
	}

   /**
    * @see org.netxms.nxmc.modules.logviewer.widgets.ui.eclipse.logviewer.widgets.ConditionEditor#createContent(org.netxms.client.log.ColumnFilter)
    */
	@Override
   protected void createContent(ColumnFilter initialFilter)
	{
      Composite group = new Composite(this, SWT.NONE);
      group.setBackground(getBackground());
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 5;
		group.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.CENTER;
		group.setLayoutData(gd);

		final Calendar c = Calendar.getInstance();
		c.setTime(new Date());

		datePicker1 = new DateTime(group, SWT.DATE | SWT.DROP_DOWN);
		datePicker1.setDate(c.get(Calendar.YEAR), c.get(Calendar.MONTH), c.get(Calendar.DAY_OF_MONTH));
      datePicker1.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

		timePicker1 = new DateTime(group, SWT.TIME);
		timePicker1.setTime(c.get(Calendar.HOUR_OF_DAY), c.get(Calendar.MINUTE), c.get(Calendar.SECOND));
      timePicker1.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
		
      andLabel = new Label(group, SWT.NONE);
      andLabel.setText(i18n.tr("  and  "));
      andLabel.setBackground(getBackground());
      andLabel.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

		datePicker2 = new DateTime(group, SWT.DATE | SWT.DROP_DOWN);
		datePicker2.setDate(c.get(Calendar.YEAR), c.get(Calendar.MONTH), c.get(Calendar.DAY_OF_MONTH));
      datePicker2.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

		timePicker2 = new DateTime(group, SWT.TIME);
		timePicker2.setTime(c.get(Calendar.HOUR_OF_DAY), c.get(Calendar.MINUTE), c.get(Calendar.SECOND));
      timePicker2.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
   }

   /**
    * @see org.netxms.nxmc.modules.logviewer.widgets.ui.eclipse.logviewer.widgets.ConditionEditor#operationSelectionChanged(int)
    */
	@Override
	protected void operationSelectionChanged(int selectionIndex)
	{
		if (selectionIndex == 0)	// between
		{
			andLabel.setVisible(true);
         datePicker1.setVisible(true);
         timePicker1.setVisible(true);
			datePicker2.setVisible(true);
			timePicker2.setVisible(true);
		}
		else if (selectionIndex == 3)
		{
		   andLabel.setVisible(false);
		   datePicker1.setVisible(false);
         timePicker1.setVisible(false);
         datePicker2.setVisible(false);
         timePicker2.setVisible(false);
		}
		else
		{
			andLabel.setVisible(false);
         datePicker1.setVisible(true);
         timePicker1.setVisible(true);
			datePicker2.setVisible(false);
			timePicker2.setVisible(false);
		}
	}

   /**
    * @see org.netxms.nxmc.modules.logviewer.widgets.ui.eclipse.logviewer.widgets.ConditionEditor#createFilter()
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
				filter = new ColumnFilter(ColumnFilterType.LESS, timestamp);
				break;
			case 2:	// after
				filter = new ColumnFilter(ColumnFilterType.GREATER, timestamp);
				break;
			case 3: // today
			   c.clear();
			   c.set(datePicker1.getYear(), datePicker1.getMonth(), datePicker1.getDay(),
		            0, 0, 0);
			   long from = c.getTimeInMillis() / 1000;
			   c.clear();
			   c.set(datePicker1.getYear(), datePicker1.getMonth(), datePicker1.getDay(),
                  23, 59, 59);
			   long to = c.getTimeInMillis() / 1000;
			   filter = new ColumnFilter(from, to);
			   break;
			default:
				filter = new ColumnFilter(timestamp, timestamp);
				break;
		}
		return filter;
	}
}
