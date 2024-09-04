/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2022 Raden Soultions
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
package org.netxms.nxmc.modules.objecttools.widgets;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.xnap.commons.i18n.I18n;

/**
 * Server command executor and output provider widget
 */
public class ServerCommandExecutor extends AbstractObjectToolExecutor
{
   private I18n i18n = LocalizationHelper.getI18n(ServerCommandExecutor.class);
   
   private NXCSession session;

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
   public ServerCommandExecutor(Composite resultArea, ObjectContext context, ActionSet actionSet, CommonContext objectToolInfo)
   { 
      super(resultArea, context, actionSet, objectToolInfo);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.modules.objecttools.widgets.AbstractObjectToolExecutor#executeInternal(org.eclipse.swt.widgets.Display)
    */
   @Override
   protected void executeInternal(Display display) throws Exception
   {
      session.executeServerCommand(objectContext.object.getObjectId(), objectContext.getAlarmId(), objectToolInfo.tool.getData(), objectToolInfo.inputValues, objectToolInfo.maskedFields, true, getOutputListener(), null);
      writeOutput(i18n.tr("\n\n*** TERMINATED ***\n\n\n"));
   }

   /**
    * @see org.netxms.nxmc.modules.objecttools.widgets.AbstractObjectToolExecutor#terminate()
    */
   @Override
   public void terminate()
   {
      if (streamId > 0)
      {
         Job job = new Job(String.format("Stop server command for node %s", objectContext.object.getObjectName()), null) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
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
    * @see org.netxms.nxmc.modules.objecttools.widgets.AbstractObjectToolExecutor#isTerminateSupported()
    */
   @Override
   protected boolean isTerminateSupported()
   {
      return true;
   }
}
