/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.eclipse.jface.action.Separator;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * View for agent action execution results
 */
public class AgentActionResults extends AbstractCommandResultView
{
   private I18n i18n = LocalizationHelper.getI18n(AgentActionResults.class);

   private Action actionRestart;

   /**
    * Constructor
    * 
    * @param node
    * @param tool
    * @param inputValues
    * @param maskedFields
    */
   public AgentActionResults(ObjectContext node, ObjectTool tool, Map<String, String> inputValues, List<String> maskedFields)
   {
      super(node, tool, inputValues, maskedFields);
   }

   /**
    * Clone constructor
    */
   protected AgentActionResults()
   {
      super();
   }
   
   /**
    * @see org.netxms.nxmc.modules.objecttools.views.AbstractCommandResultView#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      actionRestart.setEnabled(true);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      AgentActionResults view = (AgentActionResults)super.cloneView();
      view.executionString = executionString;
      view.inputValues = inputValues;
      view.maskedFields = maskedFields;
      return view;
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

   /**
    * @see org.netxms.nxmc.modules.objecttools.views.AbstractCommandResultView#execute()
    */
   @Override
   public void execute()
   {
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
               session.executeActionWithExpansion(object.object.getObjectId(), object.getAlarmId(), executionString, true, inputValues, maskedFields, getOutputListener(), null);
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
