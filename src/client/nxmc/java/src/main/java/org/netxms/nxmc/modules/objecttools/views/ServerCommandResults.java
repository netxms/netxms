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
package org.netxms.nxmc.modules.objecttools.views;

import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * View for server command execution results
 */
public class ServerCommandResults extends AbstractCommandResultView
{
   private final I18n i18n = LocalizationHelper.getI18n(ServerCommandResults.class);

   private String lastCommand = null;
   private Action actionStop;
   private boolean isRunning = false;

   /**
    * Constructor
    * 
    * @param node
    * @param tool
    * @param inputValues
    * @param maskedFields
    */
   public ServerCommandResults(ObjectContext node, ObjectTool tool, Map<String, String> inputValues, List<String> maskedFields)
   {
      super(node, tool, inputValues, maskedFields);
   }

   /**
    * Clone constructor
    */
   protected ServerCommandResults()
   {
      super();
   }   

   /**
    * Create actions
    */
   protected void createActions()
   {
      super.createActions();
      
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
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.MenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionStop);
      super.fillLocalMenu(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolbar(org.eclipse.jface.action.ToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionStop);
      super.fillLocalToolBar(manager);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager manager)
   {
      manager.add(actionStop);
      super.fillContextMenu(manager);
   }
   
   /**
    * @see org.netxms.nxmc.modules.objecttools.views.AbstractCommandResultView#execute()
    */
   @Override
   public void execute()
   {
      if (!restoreUserInputFields())
         return;
      
      if (isRunning)
      {
         MessageDialog.openError(Display.getCurrent().getActiveShell(), "Error", "Command already running!");
         return;
      }

      isRunning = true;
      actionRestart.setEnabled(false);
      actionStop.setEnabled(true);
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
               session.executeServerCommand(object.object.getObjectId(), object.getAlarmId(), tool.getData(), inputValues, maskedFields, true, getOutputListener(), null);
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
                  actionStop.setEnabled(false);
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
         Job job = new Job("Stop server command for node: " + object.object.getObjectName(), this) {
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
                     actionStop.setEnabled(false);
                     actionRestart.setEnabled(true);
                  }
               });
            }
            
            @Override
            protected String getErrorMessage()
            {
               return "Failed to stop server command for node: " + object.object.getObjectName();
            }
         };
         job.start();
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#beforeClose()
    */
   @Override
   public boolean beforeClose()
   {
      if (isRunning)
      {
         if (MessageDialog.openConfirm(Display.getCurrent().getActiveShell(), "Stop command", "Do you wish to stop the command \"" + lastCommand + "\"? "))
         {
            stopCommand();
            return true;
         }
         return false;
      }
      return true;
   }
}
