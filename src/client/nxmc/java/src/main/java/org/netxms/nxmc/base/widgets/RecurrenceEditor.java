/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StackLayout;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Editor for recurring schedules (cron expressions). Provides preset builder for common recurrence patterns (every N minutes,
 * hourly, daily, weekly, monthly) with raw cron expression input as an escape hatch.
 */
public class RecurrenceEditor extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(RecurrenceEditor.class);

   private static final int MODE_MINUTES = 0;
   private static final int MODE_HOURLY = 1;
   private static final int MODE_DAILY = 2;
   private static final int MODE_WEEKLY = 3;
   private static final int MODE_MONTHLY = 4;
   private static final int MODE_CUSTOM = 5;

   private Combo recurrenceMode;
   private Composite parameterArea;
   private StackLayout parameterAreaLayout;
   private Composite minutesParameters;
   private Spinner minuteInterval;
   private Composite hourlyParameters;
   private Spinner hourlyMinute;
   private Composite dailyParameters;
   private Spinner dailyHour;
   private Spinner dailyMinute;
   private Composite weeklyParameters;
   private Spinner weeklyHour;
   private Spinner weeklyMinute;
   private Button[] weekDays;
   private Composite monthlyParameters;
   private Spinner monthlyDay;
   private Spinner monthlyHour;
   private Spinner monthlyMinute;
   private Composite customParameters;
   private Text textSchedule;

   /**
    * @param parent parent composite
    * @param style widget style
    */
   public RecurrenceEditor(Composite parent, int style)
   {
      super(parent, style);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      setLayout(layout);

      recurrenceMode = new Combo(this, SWT.READ_ONLY);
      recurrenceMode.add(i18n.tr("Every N minutes"));
      recurrenceMode.add(i18n.tr("Hourly"));
      recurrenceMode.add(i18n.tr("Daily"));
      recurrenceMode.add(i18n.tr("Weekly"));
      recurrenceMode.add(i18n.tr("Monthly"));
      recurrenceMode.add(i18n.tr("Custom (cron expression)"));
      recurrenceMode.select(MODE_DAILY);
      recurrenceMode.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            showModeParameters();
            updateGeneratedSchedule();
            updateEnablement(true);
         }
      });

      createParameterArea();

      textSchedule = new Text(this, SWT.BORDER);
      textSchedule.setTextLimit(255);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textSchedule.setLayoutData(gd);

      showModeParameters();
      updateGeneratedSchedule();
      updateEnablement(true);
   }

   /**
    * Create stack of per-mode parameter controls
    */
   private void createParameterArea()
   {
      parameterArea = new Composite(this, SWT.NONE);
      parameterAreaLayout = new StackLayout();
      parameterArea.setLayout(parameterAreaLayout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      parameterArea.setLayoutData(gd);

      final ModifyListener modifyListener = new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            updateGeneratedSchedule();
         }
      };
      final SelectionListener selectionListener = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            updateGeneratedSchedule();
         }
      };

      minutesParameters = new Composite(parameterArea, SWT.NONE);
      minutesParameters.setLayout(newParameterLayout(3));
      new Label(minutesParameters, SWT.NONE).setText(i18n.tr("Every"));
      minuteInterval = new Spinner(minutesParameters, SWT.BORDER);
      minuteInterval.setMinimum(1);
      minuteInterval.setMaximum(59);
      minuteInterval.setSelection(5);
      minuteInterval.addModifyListener(modifyListener);
      new Label(minutesParameters, SWT.NONE).setText(i18n.tr("minutes"));

      hourlyParameters = new Composite(parameterArea, SWT.NONE);
      hourlyParameters.setLayout(newParameterLayout(2));
      new Label(hourlyParameters, SWT.NONE).setText(i18n.tr("At minute"));
      hourlyMinute = new Spinner(hourlyParameters, SWT.BORDER);
      hourlyMinute.setMinimum(0);
      hourlyMinute.setMaximum(59);
      hourlyMinute.setSelection(0);
      hourlyMinute.addModifyListener(modifyListener);

      dailyParameters = new Composite(parameterArea, SWT.NONE);
      dailyParameters.setLayout(newParameterLayout(4));
      new Label(dailyParameters, SWT.NONE).setText(i18n.tr("At"));
      dailyHour = createHourSpinner(dailyParameters, modifyListener);
      new Label(dailyParameters, SWT.NONE).setText(":");
      dailyMinute = createMinuteSpinner(dailyParameters, modifyListener);

      weeklyParameters = new Composite(parameterArea, SWT.NONE);
      weeklyParameters.setLayout(newParameterLayout(11));
      new Label(weeklyParameters, SWT.NONE).setText(i18n.tr("At"));
      weeklyHour = createHourSpinner(weeklyParameters, modifyListener);
      new Label(weeklyParameters, SWT.NONE).setText(":");
      weeklyMinute = createMinuteSpinner(weeklyParameters, modifyListener);
      final String[] dayNames = { i18n.tr("Mon"), i18n.tr("Tue"), i18n.tr("Wed"), i18n.tr("Thu"), i18n.tr("Fri"), i18n.tr("Sat"), i18n.tr("Sun") };
      weekDays = new Button[7];
      for(int i = 0; i < 7; i++)
      {
         weekDays[i] = new Button(weeklyParameters, SWT.CHECK);
         weekDays[i].setText(dayNames[i]);
         weekDays[i].addSelectionListener(selectionListener);
      }

      monthlyParameters = new Composite(parameterArea, SWT.NONE);
      monthlyParameters.setLayout(newParameterLayout(6));
      new Label(monthlyParameters, SWT.NONE).setText(i18n.tr("On day"));
      monthlyDay = new Spinner(monthlyParameters, SWT.BORDER);
      monthlyDay.setMinimum(1);
      monthlyDay.setMaximum(31);
      monthlyDay.setSelection(1);
      monthlyDay.addModifyListener(modifyListener);
      new Label(monthlyParameters, SWT.NONE).setText(i18n.tr("at"));
      monthlyHour = createHourSpinner(monthlyParameters, modifyListener);
      new Label(monthlyParameters, SWT.NONE).setText(":");
      monthlyMinute = createMinuteSpinner(monthlyParameters, modifyListener);

      customParameters = new Composite(parameterArea, SWT.NONE);
      customParameters.setLayout(newParameterLayout(1));
      new Label(customParameters, SWT.NONE).setText(i18n.tr("Cron expression: minute hour day-of-month month day-of-week"));
   }

   /**
    * Create layout for parameter composite
    *
    * @param columns number of columns
    * @return layout object
    */
   private static GridLayout newParameterLayout(int columns)
   {
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = columns;
      return layout;
   }

   /**
    * Create spinner for hour selection
    *
    * @param parent parent composite
    * @param modifyListener listener to attach
    * @return created spinner
    */
   private static Spinner createHourSpinner(Composite parent, ModifyListener modifyListener)
   {
      Spinner spinner = new Spinner(parent, SWT.BORDER);
      spinner.setMinimum(0);
      spinner.setMaximum(23);
      spinner.setSelection(0);
      spinner.addModifyListener(modifyListener);
      return spinner;
   }

   /**
    * Create spinner for minute selection
    *
    * @param parent parent composite
    * @param modifyListener listener to attach
    * @return created spinner
    */
   private static Spinner createMinuteSpinner(Composite parent, ModifyListener modifyListener)
   {
      Spinner spinner = new Spinner(parent, SWT.BORDER);
      spinner.setMinimum(0);
      spinner.setMaximum(59);
      spinner.setSelection(0);
      spinner.addModifyListener(modifyListener);
      return spinner;
   }

   /**
    * Show parameter controls for currently selected recurrence mode
    */
   private void showModeParameters()
   {
      switch(recurrenceMode.getSelectionIndex())
      {
         case MODE_MINUTES:
            parameterAreaLayout.topControl = minutesParameters;
            break;
         case MODE_HOURLY:
            parameterAreaLayout.topControl = hourlyParameters;
            break;
         case MODE_DAILY:
            parameterAreaLayout.topControl = dailyParameters;
            break;
         case MODE_WEEKLY:
            parameterAreaLayout.topControl = weeklyParameters;
            break;
         case MODE_MONTHLY:
            parameterAreaLayout.topControl = monthlyParameters;
            break;
         default:
            parameterAreaLayout.topControl = customParameters;
            break;
      }
      parameterArea.layout();
   }

   /**
    * Regenerate cron expression from preset controls (no-op in custom mode)
    */
   private void updateGeneratedSchedule()
   {
      int mode = recurrenceMode.getSelectionIndex();
      if (mode == MODE_CUSTOM)
         return;

      String schedule;
      switch(mode)
      {
         case MODE_MINUTES:
            schedule = "*/" + minuteInterval.getSelection() + " * * * *";
            break;
         case MODE_HOURLY:
            schedule = hourlyMinute.getSelection() + " * * * *";
            break;
         case MODE_WEEKLY:
            schedule = weeklyMinute.getSelection() + " " + weeklyHour.getSelection() + " * * " + getSelectedWeekDays();
            break;
         case MODE_MONTHLY:
            schedule = monthlyMinute.getSelection() + " " + monthlyHour.getSelection() + " " + monthlyDay.getSelection() + " * *";
            break;
         default: // MODE_DAILY
            schedule = dailyMinute.getSelection() + " " + dailyHour.getSelection() + " * * *";
            break;
      }
      textSchedule.setText(schedule);
   }

   /**
    * Get selected week days as cron day-of-week list (1 = Monday ... 6 = Saturday, 0 = Sunday). If no days are selected, returns
    * "*" (any day).
    *
    * @return cron day-of-week field value
    */
   private String getSelectedWeekDays()
   {
      StringBuilder sb = new StringBuilder();
      for(int i = 0; i < 7; i++)
      {
         if (weekDays[i].getSelection())
         {
            if (sb.length() > 0)
               sb.append(',');
            sb.append((i + 1) % 7);
         }
      }
      return (sb.length() > 0) ? sb.toString() : "*";
   }

   /**
    * Update control enablement
    *
    * @param enabled true if editor is enabled
    */
   private void updateEnablement(boolean enabled)
   {
      recurrenceMode.setEnabled(enabled);
      setChildrenEnabled(minutesParameters, enabled);
      setChildrenEnabled(hourlyParameters, enabled);
      setChildrenEnabled(dailyParameters, enabled);
      setChildrenEnabled(weeklyParameters, enabled);
      setChildrenEnabled(monthlyParameters, enabled);
      setChildrenEnabled(customParameters, enabled);
      textSchedule.setEnabled(enabled && (recurrenceMode.getSelectionIndex() == MODE_CUSTOM));
   }

   /**
    * Set enablement for all children of given composite
    *
    * @param parent parent composite
    * @param enabled true to enable
    */
   private static void setChildrenEnabled(Composite parent, boolean enabled)
   {
      for(Control c : parent.getChildren())
         c.setEnabled(enabled);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
    */
   @Override
   public void setEnabled(boolean enabled)
   {
      super.setEnabled(enabled);
      updateEnablement(enabled);
   }

   /**
    * Get configured schedule as cron expression.
    *
    * @return cron expression
    */
   public String getSchedule()
   {
      return textSchedule.getText().trim();
   }

   /**
    * Set schedule from cron expression. Recognized expressions select the matching preset; anything else switches editor to custom
    * mode.
    *
    * @param schedule cron expression
    */
   public void setSchedule(String schedule)
   {
      parseSchedule(schedule);
      showModeParameters();
      updateEnablement(isEnabled());
   }

   /**
    * Parse cron expression and set up matching preset controls; falls back to custom mode if the expression does not match any
    * preset form.
    *
    * @param schedule cron expression
    */
   private void parseSchedule(String schedule)
   {
      textSchedule.setText(schedule);
      recurrenceMode.select(MODE_CUSTOM);

      String[] fields = schedule.trim().split("\\s+");
      if ((fields.length != 5) || !fields[3].equals("*"))
         return;

      // Every N minutes: */N * * * *
      if (fields[0].startsWith("*/") && fields[1].equals("*") && fields[2].equals("*") && fields[4].equals("*"))
      {
         int interval = parseIntField(fields[0].substring(2), 1, 59);
         if (interval >= 0)
         {
            recurrenceMode.select(MODE_MINUTES);
            minuteInterval.setSelection(interval);
         }
         return;
      }

      int minute = parseIntField(fields[0], 0, 59);
      if (minute < 0)
         return;

      // Hourly: M * * * *
      if (fields[1].equals("*") && fields[2].equals("*") && fields[4].equals("*"))
      {
         recurrenceMode.select(MODE_HOURLY);
         hourlyMinute.setSelection(minute);
         return;
      }

      int hour = parseIntField(fields[1], 0, 23);
      if (hour < 0)
         return;

      // Daily: M H * * *
      if (fields[2].equals("*") && fields[4].equals("*"))
      {
         recurrenceMode.select(MODE_DAILY);
         dailyHour.setSelection(hour);
         dailyMinute.setSelection(minute);
         return;
      }

      // Weekly: M H * * day-list
      if (fields[2].equals("*"))
      {
         boolean[] days = new boolean[7];
         for(String d : fields[4].split(","))
         {
            int day = parseIntField(d, 0, 7);
            if (day < 0)
               return;
            days[(day + 6) % 7] = true;   // cron 0/7 = Sunday -> index 6, 1 = Monday -> index 0
         }
         recurrenceMode.select(MODE_WEEKLY);
         weeklyHour.setSelection(hour);
         weeklyMinute.setSelection(minute);
         for(int i = 0; i < 7; i++)
            weekDays[i].setSelection(days[i]);
         return;
      }

      // Monthly: M H D * *
      if (fields[4].equals("*"))
      {
         int day = parseIntField(fields[2], 1, 31);
         if (day < 0)
            return;
         recurrenceMode.select(MODE_MONTHLY);
         monthlyDay.setSelection(day);
         monthlyHour.setSelection(hour);
         monthlyMinute.setSelection(minute);
      }
   }

   /**
    * Parse integer cron field value with range check.
    *
    * @param value field text
    * @param min minimal allowed value
    * @param max maximal allowed value
    * @return parsed value or -1 if value is not a number within given range
    */
   private static int parseIntField(String value, int min, int max)
   {
      try
      {
         int n = Integer.parseInt(value);
         return ((n >= min) && (n <= max)) ? n : -1;
      }
      catch(NumberFormatException e)
      {
         return -1;
      }
   }
}
