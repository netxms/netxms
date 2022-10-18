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
package org.netxms.nxmc.modules.objects.views;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.netxms.client.TextOutputListener;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.TextConsole.IOConsoleOutputStream;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * View for agent action execution results
 */
public class SSHCommandResults extends AbstractCommandResultView implements TextOutputListener
{
   private static final I18n i18n = LocalizationHelper.getI18n(ServerCommandResults.class);
  
   private IOConsoleOutputStream out;
   private Action actionRestart;

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
    * Create actions
    */
   protected void createActions()
   {
      super.createActions();

      actionRestart = new Action(i18n.tr("&Restart"), SharedIcons.RESTART) {
         @Override
         public void run()
         {
            execute();
         }
      };
      actionRestart.setEnabled(false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.MenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionRestart);
      manager.add(new Separator());
      super.fillLocalMenu(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolbar(org.eclipse.jface.action.ToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionRestart);
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
      manager.add(new Separator());
      super.fillContextMenu(manager);
   }

   @Override
   public void execute()
   {
      actionRestart.setEnabled(false);
      out = console.newOutputStream();
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
               session.executeSshCommand(object.object.getObjectId(), executionString, true, SSHCommandResults.this, null);
               out.write(i18n.tr("\n\n*** TERMINATED ***\n\n\n"));
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
                  actionRestart.setEnabled(true);
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
            out.write(text);
      }
      catch(IOException e)
      {
      }
   }

   /**
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

   /* (non-Javadoc)
    * @see org.netxms.client.TextOutputListener#setStreamId(long)
    */
   @Override
   public void setStreamId(long streamId)
   {
   }

   @Override
   public void onError()
   {
   }
}
