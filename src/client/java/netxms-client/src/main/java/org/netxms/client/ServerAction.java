/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.client;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Represents NetXMS server's action
 *
 */
public class ServerAction
{
	public static final int EXEC_LOCAL = 0;
	public static final int EXEC_REMOTE = 1;
	public static final int SEND_EMAIL = 2;
	public static final int SEND_NOTIFICATION = 3;
	public static final int FORWARD_EVENT = 4;
	public static final int EXEC_NXSL_SCRIPT = 5;
   public static final int XMPP_MESSAGE = 6;
	
	private long id;
	private int type;
	private String name;
	private String data;
	private String recipientAddress;
	private String emailSubject;
	private boolean disabled;
	private String channelName;

	/**
	 * Create server action object with given ID
	 * 
	 * @param id Action ID
	 */
	public ServerAction(long id)
	{
		this.id = id;
		type = EXEC_LOCAL;
		name = "New action";
		data = "";
		disabled = false;
		channelName = "";
	}
	
	/**
	 * Create server action object from NXCP message
	 * 
	 * @param msg NXCP message
	 */
	protected ServerAction(final NXCPMessage msg)
	{
		id = msg.getFieldAsInt64(NXCPCodes.VID_ACTION_ID);
		type = msg.getFieldAsInt32(NXCPCodes.VID_ACTION_TYPE);
		name = msg.getFieldAsString(NXCPCodes.VID_ACTION_NAME);
		data = msg.getFieldAsString(NXCPCodes.VID_ACTION_DATA);
		recipientAddress = msg.getFieldAsString(NXCPCodes.VID_RCPT_ADDR);
		emailSubject = msg.getFieldAsString(NXCPCodes.VID_EMAIL_SUBJECT);
		disabled = msg.getFieldAsBoolean(NXCPCodes.VID_IS_DISABLED);
      channelName = msg.getFieldAsString(NXCPCodes.VID_CHANNEL_NAME);
	}
	
	/**
	 * Fill NXCP message with action's data
	 * @param msg NXCP message
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		msg.setFieldInt32(NXCPCodes.VID_ACTION_ID, (int)id);
		msg.setFieldInt16(NXCPCodes.VID_ACTION_TYPE, type);
		msg.setField(NXCPCodes.VID_ACTION_NAME, name);
		msg.setField(NXCPCodes.VID_ACTION_DATA, data);
		msg.setField(NXCPCodes.VID_RCPT_ADDR, recipientAddress);
		msg.setField(NXCPCodes.VID_EMAIL_SUBJECT, emailSubject);
		msg.setFieldInt16(NXCPCodes.VID_IS_DISABLED, disabled ? 1 : 0);
      msg.setField(NXCPCodes.VID_CHANNEL_NAME, channelName);
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @param type the type to set
	 */
	public void setType(int type)
	{
		this.type = type;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the data
	 */
	public String getData()
	{
		return data;
	}

	/**
	 * @param data the data to set
	 */
	public void setData(String data)
	{
		this.data = data;
	}

	/**
	 * @return the recipientAddress
	 */
	public String getRecipientAddress()
	{
		return recipientAddress;
	}

	/**
	 * @param recipientAddress the recipientAddress to set
	 */
	public void setRecipientAddress(String recipientAddress)
	{
		this.recipientAddress = recipientAddress;
	}

	/**
	 * @return the emailSubject
	 */
	public String getEmailSubject()
	{
		return emailSubject;
	}

	/**
	 * @param emailSubject the emailSubject to set
	 */
	public void setEmailSubject(String emailSubject)
	{
		this.emailSubject = emailSubject;
	}

	/**
	 * @return the disabled
	 */
	public boolean isDisabled()
	{
		return disabled;
	}

	/**
	 * @param disabled the disabled to set
	 */
	public void setDisabled(boolean disabled)
	{
		this.disabled = disabled;
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @param id the id to set
	 */
	public void setId(long id)
	{
		this.id = id;
	}

   /**
    * @return the channelName
    */
   public String getChannelName()
   {
      return channelName;
   }

   /**
    * @param channelName the channelName to set
    */
   public void setChannelName(String channelName)
   {
      this.channelName = channelName;
   }
}
