/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Raden Soultions
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
package org.netxms.ui.eclipse.objecttools.widgets;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.ToolItem;
import org.eclipse.ui.ISaveablePart2;
import org.eclipse.ui.console.IOConsoleOutputStream;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.TextOutputListener;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Server command executor and output provider widget
 */
public class ServerCommandExecutor extends AbstractObjectToolExecutor implements TextOutputListener, ISaveablePart2
{
   private IOConsoleOutputStream out;
   private String lastCommand = null;
   private Action actionStop;
   private Map<String, String> lastInputValues = null;
   private long streamId = 0;
   private long nodeId;
   private NXCSession session;
   private boolean isRunning = false;
   private List<String> maskedFields;
   private ToolItem stopItem;

   /**
    * Constructor
    * 
    * @param resultArea parent area for output
    * @param viewPart parent view part
    * @param object object to execute command on 
    * @param tool object tool to execute
    * @param inputValues input values provided by user
    * @param maskedFields list of input values that should be mased
    */
   public ServerCommandExecutor(Composite resultArea, ViewPart viewPart, AbstractNode object,
         ObjectTool tool, Map<String, String> inputValues, List<String> maskedFields) 
   { 
      super(resultArea, SWT.NONE, viewPart);
      this.lastInputValues = inputValues;
      this.maskedFields = maskedFields;
      lastCommand = tool.getData();
      nodeId = object.getObjectId();
      session = ConsoleSharedData.getSession();
   }
   
   @Override
   protected void createActions(ViewPart viewPart)
   {
      super.createActions(viewPart);
      
      actionStop = new Action("Stop", SharedIcons.TERMINATE) {
         @Override
         public void run()
         {
            stopCommand();
         }
      };
      actionStop.setEnabled(false);
   }
   
   @Override
   protected void fillContextMenu(final IMenuManager manager)
   {
      manager.add(actionStop);
      super.fillContextMenu(manager);      
   }
   
   /**
    * Create toolbar items
    */
   @Override
   protected void createToolbarItems()
   {
      super.createToolbarItems();
      
      stopItem = new ToolItem(toolBar, SWT.PUSH);
      stopItem.setText("Stop");
      stopItem.setToolTipText("Stop execution");
      stopItem.setImage(SharedIcons.IMG_TERMINATE);
      stopItem.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            actionStop.run();
         }
      });
   }
   
   /**
    * Is stop action should be enabed
    * 
    * @param enabled true to enable
    */
   private void enableStopAction(boolean enabled)
   {
      actionStop.setEnabled(enabled);
      stopItem.setEnabled(enabled);      
   }

   @Override
   public void execute()
   {
      if (isRunning)
      {
         MessageDialog.openError(Display.getCurrent().getActiveShell(), "Error", "Command already running!");
         return;
      }
      
      isRunning = true;
      enableRestartAction(false);
      enableStopAction(true);
      out = console.newOutputStream();
      ConsoleJob job = new ConsoleJob(String.format(Messages.get().ObjectToolsDynamicMenu_ExecuteOnNode, session.getObjectName(nodeId)), null, Activator.PLUGIN_ID, null) {
         @Override
         protected String getErrorMessage()
         {
            return String.format(Messages.get().ObjectToolsDynamicMenu_CannotExecuteOnNode, session.getObjectName(nodeId));
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            try
            {
               session.executeServerCommand(nodeId, lastCommand, lastInputValues, maskedFields, true, ServerCommandExecutor.this, null);
               out.write(Messages.get().LocalCommandResults_Terminated);
            }
            finally
            {
               if (out != null)
               {
                  out.close();
                  out = null;
               }
            }
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  enableRestartAction(true);
                  enableStopAction(false);
                  isRunning = false;
               }
            });
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
   }

   /**
    * Stops running server command
    */
   private void stopCommand()
   {
      if (streamId > 0)
      {
         ConsoleJob job = new ConsoleJob("Stop server command for node: " + session.getObjectName(nodeId), null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               session.stopServerCommand(streamId);
            }
            
            @Override
            protected void jobFinalize()
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     enableRestartAction(false);
                     enableStopAction(true);
                  }
               });
            }
            
            @Override
            protected String getErrorMessage()
            {
               return "Failed to stop server command for node: " + session.getObjectName(nodeId);
            }
         };
         job.start();
      }
   }

   /* (non-Javadoc)
    * @see org.netxms.client.ActionExecutionListener#messageReceived(java.lang.String)
    */
   @Override
   public void messageReceived(String text)
   {
      try
      {
         if (out != null)
            out.write(text.replace("\r", "")); //$NON-NLS-1$ //$NON-NLS-2$
      }
      catch(IOException e)
      {
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      super.dispose();
   }

   /* (non-Javadoc)
    * @see org.netxms.client.TextOutputListener#setStreamId(long)
    */
   @Override
   public void setStreamId(long streamId)
   {
      this.streamId = streamId;
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
    */
   @Override
   public void doSave(IProgressMonitor monitor)
   {
      stopCommand();
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#doSaveAs()
    */
   @Override
   public void doSaveAs()
   {
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#isDirty()
    */
   @Override
   public boolean isDirty()
   {
      return isRunning;
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
    */
   @Override
   public boolean isSaveAsAllowed()
   {
      return false;
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#isSaveOnCloseNeeded()
    */
   @Override
   public boolean isSaveOnCloseNeeded()
   {
      return isRunning;
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart2#promptToSaveOnClose()
    */
   @Override
   public int promptToSaveOnClose()
   {
      return MessageDialog.openConfirm(Display.getCurrent().getActiveShell(), "Stop command", "Do you wish to stop the command \"" + lastCommand + "\"? ") ? 0 : 2;
   }

   @Override
   public void onError()
   {
   }

}
