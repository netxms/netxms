/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views;

import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.modules.objecttools.views.AbstractCommandResultView;
import org.xnap.commons.i18n.I18n;

/**
 * View for agent action execution results
 */
public class SSHCommandResults extends AbstractCommandResultView
{
   private final I18n i18n = LocalizationHelper.getI18n(SSHCommandResults.class);

   /**
    * Constructor
    * 
    * @param node
    * @param tool
    * @param inputValues
    * @param maskedFields
    */
   public SSHCommandResults(ObjectContext node, ObjectTool tool, final Map<String, String> inputValues, final List<String> maskedFields)
   {
      super(node, tool, inputValues, maskedFields);
   }

   /**
    * Clone constructor
    */
   protected SSHCommandResults()
   {
      super();
   }

   /**
    * @see org.netxms.nxmc.modules.objecttools.views.AbstractCommandResultView#execute()
    */
   @Override
   public void execute()
   {
      if (!restoreUserInputFields())
         return;
      
      actionRestart.setEnabled(false);
      createOutputStream();
      Job job = new Job(String.format(i18n.tr("Execute action on node %s"), object.object.getObjectName()), this) {
         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot execute action on node %s"), object.object.getObjectName());
         }

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               session.executeSshCommand(object.object.getObjectId(), object.getAlarmId(), tool.getData(), inputValues, maskedFields, true, getOutputListener(), null);
               writeToOutputStream(i18n.tr("\n\n*** TERMINATED ***\n\n\n"));
            }
            finally
            {
               closeOutputStream();
            }
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  actionRestart.setEnabled(true);
               }
            });
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
   }
}
