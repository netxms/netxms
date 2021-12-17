/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.websvc.handlers;

import java.util.List;
import java.util.Map;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;
import org.netxms.websvc.json.ResponseContainer;

/**
 * Notification handler
 */
public class NotificationHandler extends AbstractHandler
{
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      List<SessionNotification> notifications = getSessionToken().pollNotificationQueue(60);
      if (!getSession().isConnected())
         return createErrorResponseRepresentation(RCC.COMM_FAILURE);
      if (notifications.isEmpty())
         return createErrorResponseRepresentation(RCC.TIMEOUT);
      return new ResponseContainer("notifications", notifications);
   }
}
