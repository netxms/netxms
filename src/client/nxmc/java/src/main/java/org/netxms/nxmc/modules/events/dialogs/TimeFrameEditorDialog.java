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
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DateTime;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.ClientLocalizationHelper;
import org.netxms.client.events.TimeFrame;
import org.netxms.client.events.TimeFrameFormatException;
import org.netxms.nxmc.PreferenceStore;
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
   private static final I18n i18n = LocalizationHelper.getI18n(TimeFrameEditorDialog.class);
   
   private static final String CONFIG_PREFIX = "TimeFrameEditorDialog"; 
  
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
    * @param cron CRON time to edit or null if new 
    */
   public TimeFrameEditorDialog(Shell parentShell, TimeFrame timeFrame)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      this.timeFrame = timeFrame;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Select Event"));
      PreferenceStore settings = PreferenceStore.getInstance();
      int cx = settings.getAsInteger(CONFIG_PREFIX + ".cx", 0);
      int cy = settings.getAsInteger(CONFIG_PREFIX + ".cy", 0);
      if ((cx > 0) && (cy > 0))
      {
         newShell.setSize(cx, cy);
      }
      else
      {      
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
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
      anyTimeButton.setText("Any time");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
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
      selectAll.setText("Select all");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
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
      resetWeekDays.setText("Reset");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
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
      
      daysText = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER);
      daysText.setLabel("Days of the month (e.g. 1-5, 8, 11-13)");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      daysText.setLayoutData(gd);
      
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
      selectAllMonths.setText("Select all");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
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
      resetMonths.setText("Reset");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
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
      
      return dialogArea;
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
   @Override
   protected void cancelPressed()
   {
      saveSettings();
      super.cancelPressed();
   }

   /* (non-Javadoc)
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
      
      saveSettings();
      super.okPressed();
   }
   
   /**
    * Save dialog settings
    */
   private void saveSettings()
   {
      Point size = getShell().getSize();
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set(CONFIG_PREFIX + ".cx", size.x); 
      settings.set(CONFIG_PREFIX + ".cy", size.y); 
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
