/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.events.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DateTime;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.ClientLocalizationHelper;
import org.netxms.client.events.TimeFrame;
import org.netxms.client.events.TimeFrameFormatException;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Time frame create/edit dialog
 */
public class TimeFrameEditorDialog  extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(TimeFrameEditorDialog.class);

   private TimeFrame timeFrame;
   private DateTime timePickerFrom;
   private DateTime timePickerTo;
   private Button[] daysOfWeekButton;   
   private LabeledText daysText;
   private Button[] monthsButton;   

   /**
    * Constructor
    * 
    * @param parentShell parent shell
    * @param timeFrame time frame to edit or <code>null</code> to create new one
    */
   public TimeFrameEditorDialog(Shell parentShell, TimeFrame timeFrame)
   {
      super(parentShell);
      this.timeFrame = timeFrame;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((timeFrame == null) ? i18n.tr("Add Time Frame") : i18n.tr("Edit Time Frame"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);

      Composite timeSelector = new Composite(dialogArea, SWT.NONE);
      GridLayout timeSelectorLayout = new GridLayout();
      timeSelectorLayout.numColumns = 3;
      timeSelectorLayout.marginWidth = 0;
      timeSelectorLayout.marginLeft = 0;
      timeSelector.setLayout(timeSelectorLayout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      timeSelector.setLayoutData(gd);

      timePickerFrom = new DateTime(timeSelector, SWT.TIME | SWT.SHORT);
      timePickerFrom.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

      timePickerTo = new DateTime(timeSelector, SWT.TIME | SWT.SHORT);
      timePickerTo.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

      Button anyTimeButton = new Button(timeSelector, SWT.PUSH);
      anyTimeButton.setText(i18n.tr("An&y time"));
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      anyTimeButton.setLayoutData(gd);
      anyTimeButton.addSelectionListener(new SelectionAdapter() {         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            timePickerFrom.setHours(0);
            timePickerFrom.setMinutes(0);
            timePickerTo.setHours(23);
            timePickerTo.setMinutes(59);
         }
      });

      new Label(dialogArea, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Composite daySelector = new Composite(dialogArea, SWT.NONE);
      GridLayout dayLayout = new GridLayout();
      dayLayout.numColumns = 7;
      dayLayout.marginWidth = 0;
      dayLayout.marginLeft = 0;
      daySelector.setLayout(dayLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      daySelector.setLayoutData(gd);

      daysOfWeekButton = new Button[7];
      for (int i = 0; i < 7; i++)
      {
         daysOfWeekButton[i] = new Button(daySelector, SWT.CHECK);
         daysOfWeekButton[i].setText(ClientLocalizationHelper.getText(TimeFrame.DAYS_OF_THE_WEEK[i]));
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         daysOfWeekButton[i].setLayoutData(gd);
      }
      
      Button selectAll = new Button(daySelector, SWT.PUSH);
      selectAll.setText(i18n.tr("Select all"));
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      selectAll.setLayoutData(gd);      
      selectAll.addSelectionListener(new SelectionAdapter() {         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            for (int i = 0; i < 7; i++)
            {
               daysOfWeekButton[i].setSelection(true);
            }          
         }
      });
      
      Button resetWeekDays = new Button(daySelector, SWT.PUSH);
      resetWeekDays.setText(i18n.tr("Clear all"));
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      resetWeekDays.setLayoutData(gd);      
      resetWeekDays.addSelectionListener(new SelectionAdapter() {         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            for (int i = 0; i < 7; i++)
            {
               daysOfWeekButton[i].setSelection(false);
            }          
         }
      });
      
      new Label(dialogArea, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      daysText = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER);
      daysText.setLabel(i18n.tr("Days of the month (e.g. 1-5, 8, 11-13, L)"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      daysText.setLayoutData(gd);

      new Label(dialogArea, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Composite monthSelector = new Composite(dialogArea, SWT.NONE);
      GridLayout monthLayout = new GridLayout();
      monthLayout.numColumns = 6;
      monthLayout.marginWidth = 0;
      monthLayout.marginLeft = 0;
      monthSelector.setLayout(monthLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      monthSelector.setLayoutData(gd);

      monthsButton = new Button[12];
      for (int i = 0; i < 12; i++)
      {
         monthsButton[i] = new Button(monthSelector, SWT.CHECK);
         monthsButton[i].setText(ClientLocalizationHelper.getText(TimeFrame.MONTHS[i]));
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         monthsButton[i].setLayoutData(gd);
      }   
      
      Button selectAllMonths = new Button(monthSelector, SWT.PUSH);
      selectAllMonths.setText(i18n.tr("Select all"));
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      selectAllMonths.setLayoutData(gd); 
      selectAllMonths.addSelectionListener(new SelectionAdapter() {         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            for (int i = 0; i < 12; i++)
            {
               monthsButton[i].setSelection(true);
            }          
         }
      });

      Button resetMonths = new Button(monthSelector, SWT.PUSH);
      resetMonths.setText(i18n.tr("Clear all"));
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      resetMonths.setLayoutData(gd); 
      resetMonths.addSelectionListener(new SelectionAdapter() {         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            for (int i = 0; i < 12; i++)
            {
               monthsButton[i].setSelection(false);
            }          
         }
      });

      if (timeFrame != null)
      {
         timePickerFrom.setMinutes(timeFrame.getStartMinute());
         timePickerFrom.setHours(timeFrame.getStartHour());
         timePickerTo.setMinutes(timeFrame.getEndMinute());
         timePickerTo.setHours(timeFrame.getEndHour());

         for (int i = 0; i < 7; i++)
         {
            daysOfWeekButton[i].setSelection(timeFrame.getDaysOfWeek()[i]);
         }   
         daysText.setText(timeFrame.getDaysOfMonth());

         for (int i = 0; i < 12; i++)
         {
            monthsButton[i].setSelection(timeFrame.getMonths()[i]);
         }  
      }
      else
      {
         timePickerFrom.setHours(0);
         timePickerFrom.setMinutes(0);
         timePickerTo.setHours(23);
         timePickerTo.setMinutes(59);
      }

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {      
      if (timeFrame == null)
      {
         timeFrame = new TimeFrame();
      }

      boolean[] daysOfWeek = new boolean[7];
      for (int i = 0; i < 7; i++)
      {
         daysOfWeek[i] = daysOfWeekButton[i].getSelection();
      }  

      String days = daysText.getText();
      if (days.isBlank())
         days = "1-31";

      boolean[] months = new boolean[12];
      for (int i = 0; i < 12; i++)
      {
         months[i] = monthsButton[i].getSelection();
      }  
      try
      {
         timeFrame.update(timePickerFrom.getHours(), timePickerFrom.getMinutes(), timePickerTo.getHours(), timePickerTo.getMinutes(), 
               daysOfWeek, days, months);
      }
      catch(TimeFrameFormatException e)
      {
         MessageDialogHelper.openError(getShell(), "Error", e.getMessage());
         return;
      }
      
      super.okPressed();
   }

   /**
    * Get new time frame
    * 
    * @return get new time frame
    */
   public TimeFrame getTimeFrame()
   {
      return timeFrame;
   }   
}
