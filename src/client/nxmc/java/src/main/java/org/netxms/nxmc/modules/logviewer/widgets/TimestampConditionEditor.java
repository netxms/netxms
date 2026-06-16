/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DateTime;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.constants.CalendarPeriod;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.constants.TimeUnit;
import org.netxms.client.log.ColumnFilter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Condition editor for timestamp columns
 */
public class TimestampConditionEditor extends ConditionEditor
{
   private static final int OP_BETWEEN = 0;
   private static final int OP_BEFORE = 1;
   private static final int OP_AFTER = 2;
   private static final int OP_WITHIN_LAST = 3;
   private static final int OP_CURRENT_PERIOD_BASE = 4;   // followed by one entry per CalendarPeriod (TODAY, YESTERDAY, ...)

   private static final TimeUnit[] RELATIVE_UNITS = { TimeUnit.MINUTE, TimeUnit.HOUR, TimeUnit.DAY, TimeUnit.WEEK };

   private final I18n i18n = LocalizationHelper.getI18n(TimestampConditionEditor.class);
   private final String[] OPERATIONS = { i18n.tr("BETWEEN"), i18n.tr("BEFORE"), i18n.tr("AFTER"), i18n.tr("WITHIN LAST"),
         i18n.tr("TODAY"), i18n.tr("YESTERDAY"), i18n.tr("THIS WEEK"), i18n.tr("THIS MONTH") };

	private Composite group;
	private DateTime datePicker1;
	private DateTime timePicker1;
	private DateTime datePicker2;
	private DateTime timePicker2;
	private Label andLabel;
	private Spinner relativeValue;
	private Combo relativeUnit;
	private boolean millisecondTimestamp;

	/**
	 * @param parent
	 */
   public TimestampConditionEditor(Composite parent)
	{
      this(parent, false);
	}

   /**
    * @param parent
    * @param millisecondTimestamp true if associated column stores millisecond timestamps
    */
   public TimestampConditionEditor(Composite parent, boolean millisecondTimestamp)
	{
      super(parent);
      this.millisecondTimestamp = millisecondTimestamp;
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
      group = new Composite(this, SWT.NONE);
      group.setBackground(getBackground());
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 7;
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

      relativeValue = new Spinner(group, SWT.BORDER);
      relativeValue.setMinimum(1);
      relativeValue.setMaximum(1000000);
      relativeValue.setSelection(24);
      relativeValue.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

      relativeUnit = new Combo(group, SWT.READ_ONLY);
      relativeUnit.setItems(i18n.tr("minutes"), i18n.tr("hours"), i18n.tr("days"), i18n.tr("weeks"));
      relativeUnit.select(1);   // hours
      relativeUnit.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

      if (initialFilter != null)
      {
         int operationIndex = -1;
         switch(initialFilter.getType())
         {
            case RANGE:
               operationIndex = OP_BETWEEN;
               setPickerDateTime(datePicker1, timePicker1, initialFilter.getRangeFrom());
               setPickerDateTime(datePicker2, timePicker2, initialFilter.getRangeTo());
               break;
            case LESS:
               operationIndex = OP_BEFORE;
               setPickerDateTime(datePicker1, timePicker1, initialFilter.getNumericValue());
               break;
            case GREATER:
               operationIndex = OP_AFTER;
               setPickerDateTime(datePicker1, timePicker1, initialFilter.getNumericValue());
               break;
            case RELATIVE:
               operationIndex = OP_WITHIN_LAST;
               relativeValue.setSelection(initialFilter.getRelativeValue());
               relativeUnit.select((initialFilter.getRelativeUnit() != null) ? initialFilter.getRelativeUnit().getValue() : 1);
               break;
            case CURRENT_PERIOD:
               operationIndex = OP_CURRENT_PERIOD_BASE + ((initialFilter.getPeriod() != null) ? initialFilter.getPeriod().getValue() : 0);
               break;
            default:
               break;
         }
         if (operationIndex >= 0)
            setSelectedOperation(operationIndex);
      }

      // Apply visibility for the currently selected operation (combo defaults to BETWEEN when no initial filter)
      operationSelectionChanged(getSelectedOperation());
   }

   /**
    * Set date/time picker pair from epoch timestamp (seconds or milliseconds depending on column type).
    */
   private void setPickerDateTime(DateTime datePicker, DateTime timePicker, long timestamp)
   {
      Calendar c = Calendar.getInstance();
      c.setTimeInMillis(millisecondTimestamp ? timestamp : timestamp * 1000L);
      datePicker.setDate(c.get(Calendar.YEAR), c.get(Calendar.MONTH), c.get(Calendar.DAY_OF_MONTH));
      timePicker.setTime(c.get(Calendar.HOUR_OF_DAY), c.get(Calendar.MINUTE), c.get(Calendar.SECOND));
   }

   /**
    * Convert calendar time in milliseconds to the unit expected for this column (seconds or milliseconds).
    */
   private long toFilterValue(long millis)
   {
      return millisecondTimestamp ? millis : millis / 1000;
   }

   /**
    * @see org.netxms.nxmc.modules.logviewer.widgets.ui.eclipse.logviewer.widgets.ConditionEditor#operationSelectionChanged(int)
    */
	@Override
	protected void operationSelectionChanged(int selectionIndex)
	{
      boolean relative = (selectionIndex == OP_WITHIN_LAST);
      boolean range = (selectionIndex == OP_BETWEEN);
      boolean single = (selectionIndex == OP_BEFORE) || (selectionIndex == OP_AFTER);
      
      setControlVisible(datePicker1, range || single);
      setControlVisible(timePicker1, range || single);
      setControlVisible(andLabel, range);
      setControlVisible(datePicker2, range);
      setControlVisible(timePicker2, range);
      setControlVisible(relativeValue, relative);
      setControlVisible(relativeUnit, relative);

      getParent().layout(true, true);
	}

   /**
    * Set control visibility and exclude hidden controls from layout.
    *
    * @param control control to show or hide
    * @param visible true to show
    */
   private static void setControlVisible(Control control, boolean visible)
   {
      control.setVisible(visible);
      Object layoutData = control.getLayoutData();
      if (layoutData instanceof GridData)
         ((GridData)layoutData).exclude = !visible;
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
		final long timestamp = toFilterValue(c.getTimeInMillis());

      int operation = getSelectedOperation();
		ColumnFilter filter;
		switch(operation)
		{
			case OP_BETWEEN:
				c.clear();
				c.set(datePicker2.getYear(), datePicker2.getMonth(), datePicker2.getDay(),
						timePicker2.getHours(), timePicker2.getMinutes(), timePicker2.getSeconds());
				filter = new ColumnFilter(timestamp, toFilterValue(c.getTimeInMillis()));
				break;
			case OP_BEFORE:
				filter = new ColumnFilter(ColumnFilterType.LESS, timestamp);
				break;
			case OP_AFTER:
				filter = new ColumnFilter(ColumnFilterType.GREATER, timestamp);
				break;
			case OP_WITHIN_LAST:
				filter = new ColumnFilter(relativeValue.getSelection(), RELATIVE_UNITS[relativeUnit.getSelectionIndex()]);
				break;
			default:   // calendar period (TODAY, YESTERDAY, ...)
				filter = new ColumnFilter(CalendarPeriod.getByValue(operation - OP_CURRENT_PERIOD_BASE));
				break;
		}
		return filter;
	}
}
