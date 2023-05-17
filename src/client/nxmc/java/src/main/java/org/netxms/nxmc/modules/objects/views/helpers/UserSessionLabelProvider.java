/**
 * NetXMS - open source network management system
 * Copyright (C) 2019 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.UserSession;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.modules.objects.views.UserSessionsView;

/**
 * User session label provider
 */
public class UserSessionLabelProvider extends LabelProvider implements ITableLabelProvider
{
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
      UserSession s = (UserSession)element;
      switch(columnIndex)
      {
         case UserSessionsView.COLUMN_AGENT_PID:
            return (s.getAgentPID() != 0) ? Long.toString(s.getAgentPID()) : "";
         case UserSessionsView.COLUMN_AGENT_TYPE:
            switch(s.getAgentType())
            {
               case 0:
                  return "session";
               case 1:
                  return "user";
               default:
                  return "none";
            }
         case UserSessionsView.COLUMN_CLIENT_ADDRESS:
            return ((s.getClientAddress() != null) && !s.getClientAddress().isAnyLocalAddress()) ? s.getClientAddress().getHostAddress() : "";
         case UserSessionsView.COLUMN_CLIENT_NAME:
            return s.getClientName();
         case UserSessionsView.COLUMN_DISPLAY:
            return s.getDisplayDescription();
         case UserSessionsView.COLUMN_ID:
            return Integer.toString(s.getId());
         case UserSessionsView.COLUMN_IDLE_TIME:
            return Integer.toString(s.getIdleTime());
         case UserSessionsView.COLUMN_LOGIN_TIME:
            return ((s.getLoginTime() != null) && (s.getLoginTime().getTime() != 0)) ? DateFormatFactory.getDateTimeFormat().format(s.getLoginTime()) : "";
         case UserSessionsView.COLUMN_STATE:
            return s.isConnected() ? "Active" : "Disconnected";
         case UserSessionsView.COLUMN_TERMINAL:
            return s.getTerminal();
         case UserSessionsView.COLUMN_USER:
            return s.getLoginName();
         default:
            return null;
      }
   }
}
