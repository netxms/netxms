/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objecttools;

import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.services.ObjectToolHandler;
import org.xnap.commons.i18n.I18n;

/**
 * Tool hand;er for "wakeup" internal tool
 */
public class WakeupToolHandler implements ObjectToolHandler
{
   private final I18n i18n = LocalizationHelper.getI18n(WakeupToolHandler.class);

   /**
    * @see org.netxms.nxmc.services.ObjectToolHandler#getId()
    */
   @Override
   public String getId()
   {
      return "wakeup";
   }

   /**
    * @see org.netxms.nxmc.services.ObjectToolHandler#canExecuteOnNode(org.netxms.client.objects.AbstractNode,
    *      org.netxms.client.objecttools.ObjectTool)
    */
   @Override
   public boolean canExecuteOnNode(AbstractNode node, ObjectTool tool)
   {
      return true;
   }

   /**
    * @see org.netxms.nxmc.services.ObjectToolHandler#execute(org.netxms.client.objects.AbstractNode,
    *      org.netxms.client.objecttools.ObjectTool, java.util.Map, org.netxms.nxmc.base.views.ViewPlacement)
    */
   @Override
   public void execute(AbstractNode node, ObjectTool tool, Map<String, String> inputValues, ViewPlacement viewPlacement)
   {
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Sendiong wakeup packet to node"), null, viewPlacement.getMessageAreaHolder()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.wakeupNode(node.getObjectId());
            runInUIThread(() -> viewPlacement.getMessageAreaHolder().addMessage(MessageArea.SUCCESS, i18n.tr("Wakeup packet sent to node {0}", node.getNameWithAlias())));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot send wakeup packet to node");
         }
      }.start();
   }
}
