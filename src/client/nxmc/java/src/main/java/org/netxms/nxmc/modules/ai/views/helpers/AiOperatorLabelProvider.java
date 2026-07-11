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
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.ai.AiOperator;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.views.AiOperatorManager;
import org.netxms.nxmc.tools.ViewerElementUpdater;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for AI operator instance list
 */
public class AiOperatorLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(AiOperatorLabelProvider.class);

   private NXCSession session = Registry.getSession();
   private TableViewer viewer;

   /**
    * Constructor
    *
    * @param viewer owning viewer
    */
   public AiOperatorLabelProvider(TableViewer viewer)
   {
      this.viewer = viewer;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      AiOperator operator = (AiOperator)element;
      switch(columnIndex)
      {
         case AiOperatorManager.COLUMN_ID:
            return Integer.toString(operator.getId());
         case AiOperatorManager.COLUMN_NAME:
            return operator.getName();
         case AiOperatorManager.COLUMN_DESCRIPTION:
            return operator.getDescription();
         case AiOperatorManager.COLUMN_STATE:
            return getStateText(operator);
         case AiOperatorManager.COLUMN_OWNER:
            int userId = operator.getOwnerUserId();
            AbstractUserObject user = session.findUserDBObjectById(userId, new ViewerElementUpdater(viewer, element));
            return (user != null) ? user.getName() : ("[" + Integer.toString(userId) + "]");
         case AiOperatorManager.COLUMN_MODEL_SLOT:
            return operator.getModelSlot();
         case AiOperatorManager.COLUMN_MIN_INTERVAL:
            return Integer.toString(operator.getMinInterval());
         case AiOperatorManager.COLUMN_MAX_INTERVAL:
            return Integer.toString(operator.getMaxInterval());
         case AiOperatorManager.COLUMN_TOKEN_BUDGET:
            return (operator.getDailyTokenBudget() > 0) ? Integer.toString(operator.getDailyTokenBudget()) : i18n.tr("Unlimited");
         case AiOperatorManager.COLUMN_TOKENS_USED:
            return Long.toString(operator.getTokensUsedToday());
         case AiOperatorManager.COLUMN_ITERATION:
            return Integer.toString(operator.getIteration());
         case AiOperatorManager.COLUMN_LAST_EXECUTION:
            return formatTime(operator.getLastExecutionTime());
         case AiOperatorManager.COLUMN_NEXT_EXECUTION:
            return operator.isEnabled() ? formatTime(operator.getNextExecutionTime()) : "";
         case AiOperatorManager.COLUMN_FOCUS:
            return operator.getCurrentFocus();
         case AiOperatorManager.COLUMN_EXPLANATION:
            return operator.getLastExplanation();
      }
      return null;
   }

   /**
    * Get state text for operator instance.
    *
    * @param operator operator instance
    * @return state text
    */
   public String getStateText(AiOperator operator)
   {
      if (!operator.isEnabled())
         return i18n.tr("Disabled");
      return operator.isExecuting() ? i18n.tr("Running") : i18n.tr("Waiting");
   }

   /**
    * Format timestamp, treating epoch value 0 as "never".
    *
    * @param time timestamp
    * @return formatted timestamp or empty string
    */
   private static String formatTime(Date time)
   {
      if ((time == null) || (time.getTime() == 0))
         return "";
      return DateFormatFactory.getDateTimeFormat().format(time);
   }
}
