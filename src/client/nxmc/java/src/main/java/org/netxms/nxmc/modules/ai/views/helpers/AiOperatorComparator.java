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
package org.netxms.nxmc.modules.ai.views.helpers;

import java.util.Date;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.ai.AiOperator;
import org.netxms.nxmc.modules.ai.views.AiOperatorManager;

/**
 * Comparator for AI operator instance list
 */
public class AiOperatorComparator extends ViewerComparator
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

      AiOperator o1 = (AiOperator)e1;
      AiOperator o2 = (AiOperator)e2;
      int rc;
      switch((Integer)sortColumn.getData("ID"))
      {
         case AiOperatorManager.COLUMN_ID:
            rc = Integer.signum(o1.getId() - o2.getId());
            break;
         case AiOperatorManager.COLUMN_NAME:
            rc = o1.getName().compareToIgnoreCase(o2.getName());
            break;
         case AiOperatorManager.COLUMN_DESCRIPTION:
            rc = o1.getDescription().compareToIgnoreCase(o2.getDescription());
            break;
         case AiOperatorManager.COLUMN_STATE:
            rc = getStateOrder(o1) - getStateOrder(o2);
            break;
         case AiOperatorManager.COLUMN_OWNER:
            rc = Integer.signum(o1.getOwnerUserId() - o2.getOwnerUserId());
            break;
         case AiOperatorManager.COLUMN_MODEL_SLOT:
            rc = o1.getModelSlot().compareToIgnoreCase(o2.getModelSlot());
            break;
         case AiOperatorManager.COLUMN_MIN_INTERVAL:
            rc = Integer.signum(o1.getMinInterval() - o2.getMinInterval());
            break;
         case AiOperatorManager.COLUMN_MAX_INTERVAL:
            rc = Integer.signum(o1.getMaxInterval() - o2.getMaxInterval());
            break;
         case AiOperatorManager.COLUMN_TOKEN_BUDGET:
            rc = Integer.signum(o1.getDailyTokenBudget() - o2.getDailyTokenBudget());
            break;
         case AiOperatorManager.COLUMN_TOKENS_USED:
            rc = Long.signum(o1.getTokensUsedToday() - o2.getTokensUsedToday());
            break;
         case AiOperatorManager.COLUMN_ITERATION:
            rc = Integer.signum(o1.getIteration() - o2.getIteration());
            break;
         case AiOperatorManager.COLUMN_LAST_EXECUTION:
            rc = Long.signum(getTime(o1.getLastExecutionTime()) - getTime(o2.getLastExecutionTime()));
            break;
         case AiOperatorManager.COLUMN_NEXT_EXECUTION:
            rc = Long.signum(getTime(o1.getNextExecutionTime()) - getTime(o2.getNextExecutionTime()));
            break;
         case AiOperatorManager.COLUMN_FOCUS:
            rc = o1.getCurrentFocus().compareToIgnoreCase(o2.getCurrentFocus());
            break;
         case AiOperatorManager.COLUMN_EXPLANATION:
            rc = o1.getLastExplanation().compareToIgnoreCase(o2.getLastExplanation());
            break;
         default:
            rc = 0;
            break;
      }
      int dir = ((TableViewer)viewer).getTable().getSortDirection();
      return (dir == SWT.UP) ? rc : -rc;
   }

   /**
    * Get sort order for operator state.
    *
    * @param operator operator instance
    * @return sort order
    */
   private static int getStateOrder(AiOperator operator)
   {
      if (!operator.isEnabled())
         return 0;
      return operator.isExecuting() ? 2 : 1;
   }

   /**
    * Get time in milliseconds from possibly null Date.
    *
    * @param date date object or null
    * @return time in milliseconds or 0
    */
   private static long getTime(Date date)
   {
      return (date != null) ? date.getTime() : 0;
   }
}
