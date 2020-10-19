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
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.console.IOConsoleOutputStream;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.TextOutputListener;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objects.ObjectContext;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Server command executor and output provider widget
 */
public class ServerCommandExecutor extends AbstractObjectToolExecutor implements TextOutputListener
{
   private IOConsoleOutputStream out;
   private String lastCommand = null;
   private Map<String, String> lastInputValues = null;
   private long streamId = 0;
   private NXCSession session;
   private List<String> maskedFields;

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
   public ServerCommandExecutor(Composite resultArea, ViewPart viewPart, ObjectContext context, ActionSet actionSet,
         ObjectTool tool, Map<String, String> inputValues, List<String> maskedFields)
   { 
      super(resultArea, viewPart, context, actionSet);
      this.lastInputValues = inputValues;
      this.maskedFields = maskedFields;
      lastCommand = tool.getData();
      session = ConsoleSharedData.getSession();
   }

   /**
    * @see org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor#execute()
    */
   @Override
   public void execute()
   {
      if (isRunning())
      {
         MessageDialog.openError(Display.getCurrent().getActiveShell(), "Error", "Command already running!");
         return;
      }
      
      setRunning(true);
      out = console.newOutputStream();
      ConsoleJob job = new ConsoleJob(
            String.format(Messages.get().ObjectToolsDynamicMenu_ExecuteOnNode, objectContext.object.getObjectName()), null,
            Activator.PLUGIN_ID, null) {
         @Override
         protected String getErrorMessage()
         {
            return String.format(Messages.get().ObjectToolsDynamicMenu_CannotExecuteOnNode, objectContext.object.getObjectName());
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            try
            {
               session.executeServerCommand(objectContext.object.getObjectId(), lastCommand, lastInputValues, maskedFields, true,
                     ServerCommandExecutor.this, null);
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
                  setRunning(false);
               }
            });
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
   }

   /**
    * @see org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor#terminate()
    */
   @Override
   public void terminate()
   {
      if (streamId > 0)
      {
         ConsoleJob job = new ConsoleJob(String.format("Stop server command for node %s", objectContext.object.getObjectName()),
               null, Activator.PLUGIN_ID, null) {
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
                     setRunning(false);
                  }
               });
            }
            
            @Override
            protected String getErrorMessage()
            {
               return String.format("Failed to stop server command for node %s", objectContext.object.getObjectName());
            }
         };
         job.start();
      }
   }

   /**
    * @see org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor#isTerminateSupported()
    */
   @Override
   protected boolean isTerminateSupported()
   {
      return true;
   }

   /**
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

   /**
    * @see org.netxms.client.TextOutputListener#setStreamId(long)
    */
   @Override
   public void setStreamId(long streamId)
   {
      this.streamId = streamId;
   }

   /**
    * @see org.netxms.client.TextOutputListener#onError()
    */
   @Override
   public void onError()
   {
   }
}
