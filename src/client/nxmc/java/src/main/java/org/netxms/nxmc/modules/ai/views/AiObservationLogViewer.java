/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.ai.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.netxms.client.TableRow;
import org.netxms.client.constants.AiObservationState;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logviewer.views.LogViewer;
import org.xnap.commons.i18n.I18n;

/**
 * Log viewer for AI operator observations with triage actions (acknowledge/dismiss)
 */
public class AiObservationLogViewer extends LogViewer
{
   private final I18n i18n = LocalizationHelper.getI18n(AiObservationLogViewer.class);

   /**
    * Internal constructor used for cloning
    */
   protected AiObservationLogViewer()
   {
      super();
   }

   /**
    * Create new observation log viewer.
    *
    * @param viewName view name
    * @param additionalId additional view ID part (to distinguish views in different perspectives)
    */
   public AiObservationLogViewer(String viewName, String additionalId)
   {
      super(viewName, "AIOperatorObservations", additionalId);
   }

   /**
    * @see org.netxms.nxmc.modules.logviewer.views.LogViewer#fillContextMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillContextMenu(IMenuManager mgr)
   {
      if ((getLogHandle() != null) && !viewer.getStructuredSelection().isEmpty())
      {
         mgr.add(new Action(i18n.tr("&Acknowledge")) {
            @Override
            public void run()
            {
               changeObservationState(AiObservationState.ACKNOWLEDGED);
            }
         });
         mgr.add(new Action(i18n.tr("&Dismiss")) {
            @Override
            public void run()
            {
               changeObservationState(AiObservationState.DISMISSED);
            }
         });
         mgr.add(new Action(i18n.tr("&Mark as new")) {
            @Override
            public void run()
            {
               changeObservationState(AiObservationState.NEW);
            }
         });
         mgr.add(new Separator());
      }
      super.fillContextMenu(mgr);
   }

   /**
    * Change state of selected observations.
    *
    * @param state new state
    */
   private void changeObservationState(AiObservationState state)
   {
      Log logHandle = getLogHandle();
      if (logHandle == null)
         return;

      int idColumn = -1;
      int index = 0;
      for(LogColumn lc : logHandle.getColumns())
      {
         if ((lc.getFlags() & LogColumn.LCF_RECORD_ID) != 0)
         {
            idColumn = index;
            break;
         }
         index++;
      }
      if (idColumn == -1)
         return;

      IStructuredSelection selection = viewer.getStructuredSelection();
      final long[] observationIds = new long[selection.size()];
      int i = 0;
      for(Object o : selection.toList())
         observationIds[i++] = ((TableRow)o).getValueAsLong(idColumn);

      new Job(i18n.tr("Changing observation state"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(long id : observationIds)
               session.setAiObservationState(id, state);
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot change observation state");
         }
      }.start();
   }
}
