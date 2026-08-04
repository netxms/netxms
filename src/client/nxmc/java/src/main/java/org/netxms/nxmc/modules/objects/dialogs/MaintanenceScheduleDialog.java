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
package org.netxms.nxmc.modules.objects.dialogs;

import java.util.Date;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.DateTimeSelector;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.RecurrenceEditor;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for entering maintenance schedule
 */
public class MaintanenceScheduleDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(MaintanenceScheduleDialog.class);

   private Date startDate;
   private Date endDate;
   private String schedule;
   private int duration;
   private String comments;
   private boolean recurring = false;
   private Button radioOneTime;
   private Button radioRecurring;
   private DateTimeSelector startDateSelector;
   private DateTimeSelector endDateSelector;
   private Label labelStartDate;
   private Label labelEndDate;
   private RecurrenceEditor recurrenceEditor;
   private LabeledSpinner durationSpinner;
   private LabeledText commentsEditor;

   /**
    * @param parentShell
    */
   public MaintanenceScheduleDialog(Shell parentShell)
   {
      super(parentShell);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Schedule Maintenance"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      final SelectionListener listener = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            updateEnablement();
         }
      };

      radioOneTime = new Button(dialogArea, SWT.RADIO);
      radioOneTime.setText(i18n.tr("&One time"));
      radioOneTime.setSelection(true);
      radioOneTime.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false, 2, 1));
      radioOneTime.addSelectionListener(listener);

      labelStartDate = new Label(dialogArea, SWT.NONE);
      labelStartDate.setText(i18n.tr("Start time"));

      startDateSelector = new DateTimeSelector(dialogArea, SWT.NONE);
      startDateSelector.setValue(new Date());
      startDateSelector.setToolTipText(i18n.tr("Start time"));

      labelEndDate = new Label(dialogArea, SWT.NONE);
      labelEndDate.setText(i18n.tr("End time"));

      endDateSelector = new DateTimeSelector(dialogArea, SWT.NONE);
      endDateSelector.setValue(new Date());
      endDateSelector.setToolTipText(i18n.tr("End time"));

      radioRecurring = new Button(dialogArea, SWT.RADIO);
      radioRecurring.setText(i18n.tr("&Recurring"));
      radioRecurring.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false, 2, 1));
      radioRecurring.addSelectionListener(listener);

      recurrenceEditor = new RecurrenceEditor(dialogArea, SWT.NONE);
      recurrenceEditor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      durationSpinner = new LabeledSpinner(dialogArea, SWT.NONE);
      durationSpinner.setLabel(i18n.tr("Duration (minutes)"));
      durationSpinner.setRange(1, 44640);
      durationSpinner.setSelection(60);
      durationSpinner.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false, 2, 1));

      commentsEditor = new LabeledText(dialogArea, SWT.NONE);
      commentsEditor.setLabel(i18n.tr("Comments"));
      commentsEditor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      updateEnablement();
      return dialogArea;
   }

   /**
    * Update control enablement according to selected schedule type
    */
   private void updateEnablement()
   {
      boolean oneTime = radioOneTime.getSelection();
      startDateSelector.setEnabled(oneTime);
      endDateSelector.setEnabled(oneTime);
      recurrenceEditor.setEnabled(!oneTime);
      durationSpinner.setEnabled(!oneTime);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      recurring = radioRecurring.getSelection();
      comments = commentsEditor.getText();
      if (recurring)
      {
         schedule = recurrenceEditor.getSchedule();
         duration = durationSpinner.getSelection();
         if (schedule.isEmpty())
         {
            MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Schedule cannot be empty!"));
            return;
         }
      }
      else
      {
         startDate = startDateSelector.getValue();
         endDate = endDateSelector.getValue();
         if (startDate.after(endDate))
         {
            MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Start time must be earlier than end time!"));
            return;
         }
      }
      super.okPressed();
   }

   /**
    * Check if recurring maintenance schedule was selected.
    *
    * @return true if recurring maintenance schedule was selected
    */
   public boolean isRecurring()
   {
      return recurring;
   }

   /**
    * Get start time
    *
    * @return start time
    */
   public Date getStartTime()
   {
      return startDate;
   }

   /**
    * Get end time
    *
    * @return end time
    */
   public Date getEndTime()
   {
      return endDate;
   }

   /**
    * Get recurrence schedule as cron expression (valid only if recurring schedule was selected).
    *
    * @return recurrence schedule as cron expression
    */
   public String getSchedule()
   {
      return schedule;
   }

   /**
    * Get maintenance window duration in minutes (valid only if recurring schedule was selected).
    *
    * @return maintenance window duration in minutes
    */
   public int getDuration()
   {
      return duration;
   }

   /**
    * Get comments
    *
    * @return comments
    */
   public String getComments()
   {
      return comments;
   }
}
