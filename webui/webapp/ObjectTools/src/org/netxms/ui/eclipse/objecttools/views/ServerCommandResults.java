/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objecttools.views;

import java.io.IOException;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.ISaveablePart2;
import org.netxms.client.NXCSession;
import org.netxms.client.TextOutputListener;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.TextConsole.IOConsoleOutputStream;

/**
 * View for server command execution results
 */
public class ServerCommandResults extends AbstractCommandResults implements TextOutputListener, ISaveablePart2
{
   public static final String ID = "org.netxms.ui.eclipse.objecttools.views.ServerCommandResults"; //$NON-NLS-1$

   private IOConsoleOutputStream out;
   private String lastCommand = null;
   private Action actionRestart;
   private Action actionStop;
   private Map<String, String> lastInputValues = null;
   private long streamId = 0;
   private NXCSession session;
   private boolean isRunning = false;

   /**
    * Create actions
    */
   protected void createActions()
   {
      super.createActions();
      session = (NXCSession)ConsoleSharedData.getSession();
      
      actionRestart = new Action(Messages.get().LocalCommandResults_Restart, SharedIcons.RESTART) {
         @Override
         public void run()
         {
            executeCommand(lastCommand, lastInputValues);
         }
      };
      actionRestart.setEnabled(false);
      
      actionStop = new Action("Stop", SharedIcons.TERMINATE) {
         @Override
         public void run()
         {
            stopCommand();
         }
      };
      actionStop.setEnabled(false);
   }
   
   /**
    * Fill local pull-down menu
    * 
    * @param manager Menu manager for pull-down menu
    */
   protected void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionRestart);
      manager.add(actionStop);
      manager.add(new Separator());
      super.fillLocalPullDown(manager);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionRestart);
      manager.add(actionStop);
      manager.add(new Separator());
      super.fillLocalToolBar(manager);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager manager)
   {
      manager.add(actionRestart);
      manager.add(actionStop);
      manager.add(new Separator());
      super.fillContextMenu(manager);
   }
   
   /**
    * @param command
    * @param inputValues 
    */
   public void executeCommand(final String command, final Map<String, String> inputValues)
   {
      if (isRunning)
      {
         MessageDialog.openError(Display.getCurrent().getActiveShell(), "Error", "Command already running!");
         return;
      }
      
      isRunning = true;
      actionRestart.setEnabled(false);
      actionStop.setEnabled(true);
      out = console.newOutputStream();
      lastCommand = command;
      lastInputValues = inputValues;
      final String terminated = Messages.get().LocalCommandResults_Terminated;
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
               session.executeServerCommand(nodeId, command, inputValues, true, ServerCommandResults.this, null);
               out.write(terminated);
            }
            catch (SWTException e)
            {
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
            if (!console.isDisposed())
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     actionRestart.setEnabled(true);
                     actionStop.setEnabled(false);
                     isRunning = false;
                  }
               });
            }
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
                     actionStop.setEnabled(false);
                     actionRestart.setEnabled(true);
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
         if (out != null && !console.isDisposed())
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
}
