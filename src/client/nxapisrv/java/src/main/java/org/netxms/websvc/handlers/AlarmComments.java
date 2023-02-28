/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import org.json.JSONObject;
import org.netxms.client.constants.RCC;
import org.netxms.client.events.AlarmComment;

/**
 * Alarm comments handler
 */
public class AlarmComments extends AbstractHandler
{
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      long alarmId = Long.parseLong((String)getRequest().getAttributes().get("alarm-id"));
      return new CommentListResponse(getSession().getAlarmComments(alarmId));
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String, java.util.Map)
    */
   @Override
   protected Object get(String id, Map<String, String> query) throws Exception
   {
      long alarmId = Long.parseLong((String)getRequest().getAttributes().get("alarm-id"));
      long commentId = Long.parseLong(id);
      List<AlarmComment> comments = getSession().getAlarmComments(alarmId);
      for(AlarmComment c : comments)
      {
         if (c.getId() == commentId)
            return c;
      }
      return createErrorResponse(RCC.INVALID_ALARM_NOTE_ID);
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#create(org.json.JSONObject)
    */
   @Override
   protected Object create(JSONObject data) throws Exception
   {
      long alarmId = Long.parseLong((String)getRequest().getAttributes().get("alarm-id"));
      getSession().createAlarmComment(alarmId, data.getString("text"));
      return null;
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#delete(java.lang.String)
    */
   @Override
   protected Object delete(String id) throws Exception
   {
      long alarmId = Long.parseLong((String)getRequest().getAttributes().get("alarm-id"));
      long commentId = Long.parseLong(id);
      getSession().deleteAlarmComment(alarmId, commentId);
      return null;
   }

   /**
    * Response document for comment list
    */
   @SuppressWarnings("unused")
   private static class CommentListResponse
   {
      List<AlarmComment> comments;

      public CommentListResponse(List<AlarmComment> comments)
      {
         this.comments = comments;
      }
   }
}
