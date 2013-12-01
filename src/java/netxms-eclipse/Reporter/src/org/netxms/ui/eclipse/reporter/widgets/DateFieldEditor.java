package org.netxms.ui.eclipse.reporter.widgets;

import java.util.Calendar;
import java.util.Date;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.api.client.reporting.ReportParameter;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.reporter.widgets.helpers.ScheduleDateConverter;
import org.netxms.ui.eclipse.tools.WidgetHelper;

public class DateFieldEditor extends FieldEditor
{
   private Composite parentArea;
   private Combo[] dateElements;
   private String[] dateElementNames;

   private long offset = 0;

   public DateFieldEditor(ReportParameter parameter, FormToolkit toolkit, Composite parent)
   {
      super(parameter, toolkit, parent);
   }

   @Override
   protected void createContent(Composite parent)
   {
      dateElementNames = new String[] { "Year", "Month", "Day", "Hours", "Minutes" };
      toolkit = new FormToolkit(getDisplay());
      parentArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 6;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      layout.makeColumnsEqualWidth = true;
      parentArea.setLayout(layout);

      Calendar dateTime = Calendar.getInstance();
      try
      {
         dateTime.setTime(new Date(Long.parseLong(parameter.getDefaultValue()) * 1000));
      }
      catch(NumberFormatException e)
      {
         dateTime.setTime(new Date());
      }

      dateElements = new Combo[dateElementNames.length];
      for(int i = 0; i < dateElementNames.length; i++)
      {
         Combo cb = WidgetHelper.createLabeledCombo(parentArea, SWT.BORDER, dateElementNames[i], WidgetHelper.DEFAULT_LAYOUT_DATA);
         cb.setText(getDateTimeText(dateElementNames[i], dateTime));
         if (!dateElementNames[i].equalsIgnoreCase("Minutes"))
         {
            cb.add("current");
            cb.add("previous");
            cb.add("next");
         }
         dateElements[i] = cb;
      }

      Button openCalendar = new Button(parentArea, SWT.PUSH);
      GridData gd = new GridData(22, 22);
      gd.verticalAlignment = SWT.DOWN;
      openCalendar.setLayoutData(gd);
      openCalendar.setImage(SharedIcons.IMG_CALENDAR);
      openCalendar.addSelectionListener(new SelectionAdapter() {
         public void widgetSelected(SelectionEvent e)
         {
            CalendarDialog dialog = new CalendarDialog(getShell());
            if (dialog.open() == Window.OK)
            {
               Calendar date = Calendar.getInstance();
               date.setTime(dialog.getDateValue());
               for(int i = 0; i < dateElementNames.length; i++)
                  dateElements[i].setText(getDateTimeText(dateElementNames[i], date));
            }
         }
      });
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#getValue()
    */
   @Override
   public String getValue()
   {
      String value = null;
      ScheduleDateConverter conv = new ScheduleDateConverter(dateElementNames, 
            new String[] { 
               dateElements[0].getText(), // year
               dateElements[1].getText(), // month
               dateElements[2].getText(), // day
               dateElements[3].getText(), // hour
               dateElements[4].getText()  // minute
            }
      );
      Date date = conv.getDateTime();
      if (date != null)
      {
         value = String.valueOf(date.getTime());
         offset = conv.getOffset();
      }
      return value;
   }

   /**
    * Get dateTime text for Combo
    * 
    * @param comboName
    * @param dateTime
    */
   private String getDateTimeText(String comboName, Calendar dateTime)
   {
      String cbText;
      if (comboName.equalsIgnoreCase("Year"))
         cbText = String.valueOf(dateTime.get(Calendar.YEAR));
      else if (comboName.equalsIgnoreCase("Month"))
         cbText = String.valueOf(dateTime.get(Calendar.MONTH) + 1);
      else if (comboName.equalsIgnoreCase("Day"))
         cbText = String.valueOf(dateTime.get(Calendar.DAY_OF_MONTH));
      else if (comboName.equalsIgnoreCase("Hours"))
         cbText = String.valueOf(dateTime.get(Calendar.HOUR_OF_DAY));
      else if (comboName.equalsIgnoreCase("Minutes"))
         cbText = String.valueOf(dateTime.get(Calendar.MINUTE));
      else
         cbText = "";
      return cbText;
   }

   /**
    * @return the offset
    */
   public long getOffset()
   {
      return offset;
   }
}
