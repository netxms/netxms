/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.client.events;

import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * Alarm note (comment)
 *
 */
public class AlarmComment
{
	private long id;
	private long alarmId;
	private long userId;
	private String userName;
	private Date lastChangeTime;
	private String text;
	
	/**
	 * Create alarm note object from NXCP message.
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public AlarmComment(NXCPMessage msg, long baseId)
	{
		id = msg.getVariableAsInt64(baseId);
		alarmId = msg.getVariableAsInt64(baseId + 1);
		lastChangeTime = msg.getVariableAsDate(baseId + 2);
		userId = msg.getVariableAsInt64(baseId + 3);
		text = msg.getVariableAsString(baseId + 4);
      userName = msg.getVariableAsString(baseId + 5);
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the alarmId
	 */
	public long getAlarmId()
	{
		return alarmId;
	}

	/**
	 * @return the userId
	 */
	public long getUserId()
	{
		return userId;
	}

	/**
	 * @return the lastChangeTime
	 */
	public Date getLastChangeTime()
	{
		return lastChangeTime;
	}

	/**
	 * @return the text
	 */
	public String getText()
	{
		return text;
	}

   /**
    * @return the userName
    */
   public String getUserName()
   {
      return userName;
   }
}
