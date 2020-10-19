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
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
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
 * Server script executor and output provider widget
 */
public class ServerScriptExecutor extends AbstractObjectToolExecutor implements TextOutputListener
{
   private IOConsoleOutputStream out;
   private String script = null;
   private Map<String, String> inputValues = null;
   private long alarmId;
   private long nodeId;

   /**
    * Constructor
    * 
    * @param resultArea parent composite for result
    * @param viewPart parent view
    * @param ctx execution context
    * @param actionSet action set
    * @param tool object tool to execute
    * @param inputValues input values for execution
    */
   public ServerScriptExecutor(Composite resultArea, ViewPart viewPart, ObjectContext ctx, ActionSet actionSet, ObjectTool tool, Map<String, String> inputValues)
   {
      super(resultArea, viewPart, ctx, actionSet);
      script = tool.getData();
      alarmId = ctx.alarm != null ? ctx.alarm.getId() : 0;
      nodeId = ctx.object.getObjectId();
      this.inputValues = inputValues;
   }

   /**
    * @see org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor#execute()
    */
   @Override
   public void execute()
   {
      setRunning(true);
      final NXCSession session = ConsoleSharedData.getSession();
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
               session.executeLibraryScript(nodeId, alarmId, script, inputValues, ServerScriptExecutor.this);
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
    * @see org.eclipse.swt.widgets.Widget#dispose()
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

   /**
    * @see org.netxms.client.TextOutputListener#setStreamId(long)
    */
   @Override
   public void setStreamId(long streamId)
   {
   }

   /**
    * @see org.netxms.client.TextOutputListener#onError()
    */
   @Override
   public void onError()
   {
   }
}
