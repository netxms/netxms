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

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.ai.AiAgentTask;
import org.netxms.nxmc.modules.ai.views.AiTaskManager;

/**
 * Comparator for AI task list
 */
public class AiTaskComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		TableColumn sortColumn = ((TableViewer)viewer).getTable().getSortColumn();
		if (sortColumn == null)
			return 0;

		int rc;
      switch((Integer)sortColumn.getData("ID"))
		{
         case AiTaskManager.COLUMN_DESCRIPTION:
            rc = ((AiAgentTask)e1).getDescription().compareToIgnoreCase(((AiAgentTask)e2).getDescription());
				break;
         case AiTaskManager.COLUMN_EXPLANATION:
            rc = ((AiAgentTask)e1).getExplanation().compareToIgnoreCase(((AiAgentTask)e2).getExplanation());
            break;
         case AiTaskManager.COLUMN_ID:
            rc = Long.signum(((AiAgentTask)e1).getId() - ((AiAgentTask)e2).getId());
            break;
         case AiTaskManager.COLUMN_STATE:
            rc = Integer.signum(((AiAgentTask)e1).getState().getValue() - ((AiAgentTask)e2).getState().getValue());
            break;
         case AiTaskManager.COLUMN_LAST_EXECUTION:
            rc = Long.signum(((AiAgentTask)e1).getLastExecutionTime().getTime() - ((AiAgentTask)e2).getLastExecutionTime().getTime());
            break;
         case AiTaskManager.COLUMN_NEXT_EXECUTION:
            rc = Long.signum(((AiAgentTask)e1).getNextExecutionTime().getTime() - ((AiAgentTask)e2).getNextExecutionTime().getTime());
            break;
         case AiTaskManager.COLUMN_OWNER:
            rc = Long.signum(((AiAgentTask)e1).getUserId() - ((AiAgentTask)e2).getUserId());
            break;
			default:
				rc = 0;
				break;
		}
		int dir = ((TableViewer)viewer).getTable().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}
}
