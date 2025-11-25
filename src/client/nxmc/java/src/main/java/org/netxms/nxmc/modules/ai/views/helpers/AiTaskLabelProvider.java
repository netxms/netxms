/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.ai.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.ai.AiAgentTask;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.views.AiTaskManager;
import org.netxms.nxmc.tools.ViewerElementUpdater;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for action list 
 */
public class AiTaskLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(AiTaskLabelProvider.class);

   public final String[] stateNames = { i18n.tr("Scheduled"), i18n.tr("Running"), i18n.tr("Completed"), i18n.tr("Failed") };

   private NXCSession session = Registry.getSession();
   private TableViewer viewer;

   /**
    * Constructor
    * 
    * @param viewer owning viewer
    */
   public AiTaskLabelProvider(TableViewer viewer)
   {
      this.viewer = viewer;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
      return (columnIndex == 0) ? getImage(element) : null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
      AiAgentTask task = (AiAgentTask)element;
		switch(columnIndex)
		{
         case AiTaskManager.COLUMN_DESCRIPTION:
            return task.getDescription();
         case AiTaskManager.COLUMN_EXPLANATION:
            return task.getExplanation();
         case AiTaskManager.COLUMN_ID:
            return Long.toString(task.getId());
         case AiTaskManager.COLUMN_LAST_EXECUTION:
            return DateFormatFactory.getDateTimeFormat().format(task.getLastExecutionTime());
         case AiTaskManager.COLUMN_NEXT_EXECUTION:
            return DateFormatFactory.getDateTimeFormat().format(task.getNextExecutionTime());
         case AiTaskManager.COLUMN_OWNER:
            int userId = task.getUserId();
            AbstractUserObject user = session.findUserDBObjectById(userId, new ViewerElementUpdater(viewer, element));
            return (user != null) ? user.getName() : ("[" + Long.toString(userId) + "]");
         case AiTaskManager.COLUMN_STATE:
            try
            {
               return stateNames[task.getState().getValue()];
            }
            catch(ArrayIndexOutOfBoundsException e)
            {
               return i18n.tr("Unknown");
            }
      }
		return null;
	}
}
