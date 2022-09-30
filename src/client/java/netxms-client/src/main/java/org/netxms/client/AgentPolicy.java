/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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

import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Generic agent policy object
 */
public class AgentPolicy
{   
   public static final String AGENT_CONFIG = "AgentConfig";
   public static final String FILE_DELIVERY = "FileDelivery";
   public static final String LOG_PARSER = "LogParserConfig";
   public static final String SUPPORT_APPLICATION = "SupportApplicationConfig";
   
   public static final long EXPAND_MACRO = 1;
   
   private UUID guid;
   private String name;
	private String policyType;
	private String content;	
	private int flags;
	
	/**
	 * Create from NXCP message.
	 *
	 * @param msg NXCP message
	 */
	public AgentPolicy(NXCPMessage msg)
	{		
		guid = msg.getFieldAsUUID(NXCPCodes.VID_GUID);
		name = msg.getFieldAsString(NXCPCodes.VID_NAME);
		policyType = msg.getFieldAsString(NXCPCodes.VID_POLICY_TYPE);
		content = msg.getFieldAsString(NXCPCodes.VID_CONFIG_FILE_DATA);
		flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
	}
	
	/**
	 * Create policy object from scratch.
	 *
	 * @param name policy name
	 * @param type policy type
	 */
	public AgentPolicy(String name, String type)
   {
	   this.guid = null;
	   this.name = name;
	   this.policyType = type;
	   this.content = "";
   }
	
   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public AgentPolicy(NXCPMessage msg, long baseId)
   {
      guid = msg.getFieldAsUUID(baseId);
      policyType = msg.getFieldAsString(baseId + 1);
      name = msg.getFieldAsString(baseId + 2);
      content = msg.getFieldAsString(baseId + 3);
      flags = msg.getFieldAsInt32(baseId + 4);
   }

   /**
    * Create copy of given policy object. All fields except GUID will be copied, and GUID will be set to null.
    *
    * @param src source policy object
    */
   public AgentPolicy(AgentPolicy src)
   {
      guid = null;
      name = src.name;
      policyType = src.policyType;
      content = src.content;
      flags = src.flags;
   }

   /**
    * Update all fields from given policy object.
    *
    * @param src source policy object
    */
   public void update(AgentPolicy src)
   {
      guid = src.guid;
      name = src.name;
      policyType = src.policyType;
      content = src.content;
      flags = src.flags;
   }
   
   /**
    * Fill NXCP message with polic data.
    *
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      if (guid != null)
         msg.setField(NXCPCodes.VID_GUID, guid);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_POLICY_TYPE, policyType);
      msg.setField(NXCPCodes.VID_CONFIG_FILE_DATA, content);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
   }

	/**
	 * @return the policyType
	 */
	public String getPolicyType()
	{
		return policyType;
	}

   /**
    * @return the guid
    */
   public UUID getGuid()
   {
      return guid;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return the content
    */
   public String getContent()
   {
      return content;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * @param content the content to set
    */
   public void setContent(String content)
   {
      this.content = content;
   }

   /**
    * @param newObjectGuid the new object GUID to set
    */
   public void setGuid(UUID newObjectGuid)
   {
      guid = newObjectGuid;
   }

   /**
    * @return the flags
    */
   public int getFlags()
   {
      return flags;
   }

   /**
    * @param flags the flags to set
    */
   public void setFlags(int flags)
   {
      this.flags = flags;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return name;
   }
}
