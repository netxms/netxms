/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.dialogs;

import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.ScheduledTask;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.ScheduleEditor;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Scheduled task editor
 */
public class ScheduledTaskEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(ScheduledTaskEditDialog.class);

   private ScheduledTask task;
   private List<String> types;
   private LabeledText parameters;
   private Text comments;
   private Combo taskType;
   private ScheduleEditor scheduleEditor;
   private ObjectSelector objectSelector;

   /**
    * @param shell
    * @param task
    * @param types
    */
   public ScheduledTaskEditDialog(Shell shell, ScheduledTask task, List<String> types)
   {
      super(shell);
      if (task != null)
         this.task = task;
      else
         this.task = new ScheduledTask();
      this.types = types;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Edit Scheduled Task");
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      final Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);  

      taskType = new Combo(dialogArea, SWT.READ_ONLY);
      for(String type : types)
         taskType.add(type);
      int taskId = types.indexOf(task.getTaskHandlerId());
      taskType.select(taskId == -1 ? 0 : taskId);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      taskType.setLayoutData(gd);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      objectSelector.setLabel("Select execution object");
      objectSelector.setObjectClass(AbstractObject.class);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      objectSelector.setLayoutData(gd);
      objectSelector.setObjectId(task.getObjectId());
      
      parameters = new LabeledText(dialogArea, SWT.NONE);
      parameters.setLabel("Parameters");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      parameters.setLayoutData(gd);
      parameters.setText(task.getParameters());
      
      comments = WidgetHelper.createLabeledText(dialogArea, SWT.MULTI | SWT.BORDER, SWT.DEFAULT, "Description",
            task.getComments(), WidgetHelper.DEFAULT_LAYOUT_DATA);
      comments.setTextLimit(255);
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.heightHint = 100;
      gd.verticalAlignment = SWT.FILL;
      comments.setLayoutData(gd);
      
      if ((task.getFlags() & ScheduledTask.SYSTEM) != 0)
      {
         taskType.add(task.getTaskHandlerId());
         taskType.select(types.size());
         taskType.setEnabled(false);
         objectSelector.setEnabled(false);
         parameters.setEnabled(false);
      }

      Group scheduleGroup = new Group(dialogArea, SWT.NONE);
      scheduleGroup.setText(i18n.tr("Schedule"));
      scheduleGroup.setLayout(new GridLayout());

      scheduleEditor = new ScheduleEditor(scheduleGroup, SWT.NONE);
      scheduleEditor.setSchedule(task);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      ScheduledTask schedule = scheduleEditor.getSchedule();
      task.setSchedule(schedule.getSchedule());
      task.setExecutionTime(schedule.getExecutionTime());
      task.setComments(comments.getText());

      if ((task.getFlags() & ScheduledTask.SYSTEM) == 0)
      {
         task.setTaskHandlerId(types.get(taskType.getSelectionIndex()));
         task.setParameters(parameters.getText());
         task.setObjectId(objectSelector.getObjectId());
      }
      super.okPressed();
   }

   /**
    * Get scheduled task
    * 
    * @return
    */
   public ScheduledTask getTask()
   {
      return task;
   }
}
