/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objecttools.dialogs;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.constants.Severity;
import org.netxms.client.objects.AbstractNode;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.objects.ObjectContext;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Dialog to show object tool execution status
 */
public class ToolExecutionStatusDialog extends Dialog
{
   private Map<Long, Task> tasks = new HashMap<Long, Task>();
   private TableViewer viewer;

   /**
    * @param parentShell
    */
   public ToolExecutionStatusDialog(Shell parentShell, Set<ObjectContext> nodes)
   {
      super(parentShell);
      for(ObjectContext c : nodes)
      {
         Task task = new Task(c);
         tasks.put(task.node.getObjectId(), task);
      }
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Tool Execution Status");
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, IDialogConstants.CANCEL_ID, IDialogConstants.CLOSE_LABEL, false);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION | SWT.H_SCROLL | SWT.V_SCROLL);
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.heightHint = 600;
      viewer.getControl().setLayoutData(gd);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new TaskLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((Task)e1).node.getObjectName().compareToIgnoreCase(((Task)e2).node.getObjectName());
         }
      });

      TableColumn column = new TableColumn(viewer.getTable(), SWT.NONE);
      column.setText("Node");
      column.setWidth(500);

      column = new TableColumn(viewer.getTable(), SWT.NONE);
      column.setText("Status");
      column.setWidth(200);

      viewer.getTable().setHeaderVisible(true);
      viewer.getTable().setLinesVisible(true);

      viewer.setInput(tasks.values().toArray());

      return dialogArea;
   }

   /**
    * Update task execution status.
    *
    * @param nodeId node ID
    * @param failureReason if not null indicates task execution failure and points to failure reason
    */
   public void updateExecutionStatus(long nodeId, String failureReason)
   {
      if (viewer.getControl().isDisposed())
         return;

      final Task task = tasks.get(nodeId);
      if (task != null)
      {
         task.status = (failureReason != null) ? Task.FAILED : Task.COMPLETED;
         task.failureReason = failureReason;
         viewer.getControl().getDisplay().asyncExec(new Runnable() {
            @Override
            public void run()
            {
               viewer.update(task, null);
            }
         });
      }
   }

   /**
    * Tool execution task
    */
   private static class Task
   {
      static final int RUNNING = 0;
      static final int COMPLETED = 1;
      static final int FAILED = 2;

      AbstractNode node;
      int status;
      String failureReason;

      Task(ObjectContext context)
      {
         node = context.object;
         status = RUNNING;
         failureReason = null;
      }
   }

   /**
    * Label provider for task list
    */
   private static class TaskLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      private ILabelProvider workbenchLabelProvider = WorkbenchLabelProvider.getDecoratingWorkbenchLabelProvider();

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         switch(columnIndex)
         {
            case 0:
               return workbenchLabelProvider.getImage(((Task)element).node);
            case 1:
               switch(((Task)element).status)
               {
                  case Task.RUNNING:
                     return SharedIcons.IMG_EXECUTE;
                  case Task.COMPLETED:
                     return StatusDisplayInfo.getStatusImage(Severity.NORMAL);
                  case Task.FAILED:
                     return StatusDisplayInfo.getStatusImage(Severity.CRITICAL);
               }
         }
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         switch(columnIndex)
         {
            case 0:
               return ((Task)element).node.getObjectName();
            case 1:
               switch(((Task)element).status)
               {
                  case Task.RUNNING:
                     return "In progress";
                  case Task.COMPLETED:
                     return "OK";
                  case Task.FAILED:
                     return ((Task)element).failureReason;
               }
         }
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
       */
      @Override
      public void dispose()
      {
         workbenchLabelProvider.dispose();
         super.dispose();
      }
   }
}
