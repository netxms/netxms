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
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.TextOutputListener;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objects.ObjectContext;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.TextConsole.IOConsoleOutputStream;

/**
 * Action executor widget to run an action and display it's result 
 *
 */
public class ActionExecutor extends AbstractObjectToolExecutor implements TextOutputListener
{
   private IOConsoleOutputStream out;
   private String executionString;
   private long alarmId;
   private Map<String, String> inputValues;
   private List<String> maskedFields;
   protected long nodeId;
   
   /**
    * Constructor for action execution
    * 
    * @param parent parent control
    * @param viewPart parent view
    * @param ctx execution context
    * @param tool object tool to execute
    * @param inputValues input values provided by user
    * @param maskedFields list of the fields that should be masked
    */
   public ActionExecutor(Composite parent, ViewPart viewPart, ObjectContext ctx, ObjectTool tool, Map<String, String> inputValues, List<String> maskedFields)
   {
      super(parent, SWT.NONE, viewPart);
      alarmId = ctx.getAlarmId();
      nodeId = ctx.object.getObjectId();
      this.executionString = tool.getData();
      this.inputValues = inputValues;
      this.maskedFields = maskedFields;
   }

   @Override
   public void execute()
   {
      enableRestartAction(false);
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
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
               session.executeActionWithExpansion(nodeId, alarmId, executionString, true, inputValues, maskedFields, ActionExecutor.this, null);
               out.write(Messages.get(getDisplay()).LocalCommandResults_Terminated);
            }
            finally
            {
               out.close();
               out = null;
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
               }
            });
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();

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
            out.write(text);
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
      if (out != null)
      {
         try
         {
            out.close();
         }
         catch(IOException e)
         {
         }
         out = null;
      }
      super.dispose();
   }

   @Override
   public void setStreamId(long streamId)
   {      
   }

   @Override
   public void onError()
   {
   }

}
