/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Raden Solutions
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
package org.netxms.ui.eclipse.serverconfig.dialogs;

import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.ScheduledTask;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.ui.eclipse.widgets.ScheduleSelector;

/**
 * Scheduled task editor
 */
public class ScheduledTaskEditor extends Dialog
{
   private ScheduledTask scheduledTask;
   private List<String> scheduleTypeList;
   private LabeledText textParameters;
   private Text textComments;
   private Combo scheduleType;
   private ScheduleSelector scheduleSelector;
   private ObjectSelector selector;

   /**
    * @param shell
    * @param task
    * @param scheduleType
    */
   public ScheduledTaskEditor(Shell shell, ScheduledTask task, List<String> scheduleType)
   {
      super(shell);
      if(task != null)
         scheduledTask = task;
      else
         scheduledTask = new ScheduledTask();
      
      this.scheduleTypeList = scheduleType;
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Edit Scheduled Task");
   }


   /* (non-Javadoc)
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
      

      scheduleType = new Combo(dialogArea, SWT.READ_ONLY);
      for(String type : scheduleTypeList)
         scheduleType.add(type);
      int taskId = scheduleTypeList.indexOf(scheduledTask.getTaskHandlerId());
      scheduleType.select(taskId == -1 ? 0 : taskId);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      scheduleType.setLayoutData(gd);
      
      selector = new ObjectSelector(dialogArea, SWT.NONE, true);
      selector.setLabel("Select execution object");
      selector.setObjectClass(AbstractObject.class);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      selector.setLayoutData(gd);
      if(scheduledTask.getObjectId() != 0)
         selector.setObjectId(scheduledTask.getObjectId());
      
      textParameters = new LabeledText(dialogArea, SWT.NONE);
      textParameters.setLabel("Parameters");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      textParameters.setLayoutData(gd);
      textParameters.setText(scheduledTask.getParameters());
      
      textComments = WidgetHelper.createLabeledText(dialogArea, SWT.MULTI | SWT.BORDER, SWT.DEFAULT, "Description",
            scheduledTask.getComments(), WidgetHelper.DEFAULT_LAYOUT_DATA);
      textComments.setTextLimit(255);
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.heightHint = 100;
      gd.verticalAlignment = SWT.FILL;
      textComments.setLayoutData(gd);
      
      if((scheduledTask.getFlags() & ScheduledTask.SYSTEM) != 0)
      {
         scheduleType.add(scheduledTask.getTaskHandlerId());
         scheduleType.select(scheduleTypeList.size());
         scheduleType.setEnabled(false);
         selector.setEnabled(false);
         textParameters.setEnabled(false);
      }
      
      scheduleSelector = new ScheduleSelector(dialogArea, SWT.NONE);
      scheduleSelector.setSchedule(scheduledTask);
      
      return dialogArea;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      ScheduledTask task = scheduleSelector.getSchedule();
      scheduledTask.setSchedule(task.getSchedule());
      scheduledTask.setExecutionTime(task.getExecutionTime());
      scheduledTask.setComments(textComments.getText());
      
      if((scheduledTask.getFlags() & ScheduledTask.SYSTEM) == 0)
      {
         scheduledTask.setTaskHandlerId(scheduleTypeList.get(scheduleType.getSelectionIndex()));
         scheduledTask.setParameters(textParameters.getText());
         scheduledTask.setObjectId(selector.getObjectId());
      }
      super.okPressed();
   }
   
   /**
    * Get scheduled task
    * 
    * @return
    */
   public ScheduledTask getScheduledTask()
   {
      return scheduledTask;
   }
}
