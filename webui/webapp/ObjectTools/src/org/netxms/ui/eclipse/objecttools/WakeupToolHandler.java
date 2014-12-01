/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objecttools;

import org.eclipse.core.runtime.IProgressMonitor;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.api.ObjectToolHandler;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Tool hand;er for "wakeup" internal tool
 */
public class WakeupToolHandler implements ObjectToolHandler
{
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objecttools.api.ObjectToolHandler#canExecuteOnNode(org.netxms.client.objects.AbstractNode, org.netxms.client.objecttools.ObjectTool)
    */
   @Override
   public boolean canExecuteOnNode(AbstractNode node, ObjectTool tool)
   {
      return true;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objecttools.api.ObjectToolHandler#execute(org.netxms.client.objects.AbstractNode, org.netxms.client.objecttools.ObjectTool)
    */
   @Override
   public void execute(final AbstractNode node, ObjectTool tool)
   {
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob(Messages.get().WakeupToolHandler_JobName, null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.wakeupNode(node.getObjectId());
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().WakeupToolHandler_JobError;
         }
      }.start();
   }
}
