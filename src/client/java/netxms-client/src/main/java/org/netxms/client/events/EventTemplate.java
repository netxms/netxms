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
package org.netxms.client.events;

import java.util.HashSet;
import java.util.Set;
import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCommon;
import org.netxms.client.constants.Severity;

/**
 * Event template
 */
public class EventTemplate
{
   public static final int FLAG_WRITE_TO_LOG = 0x0001;
   public static final int FLAG_DO_NOT_MONITOR = 0x0002;

   private int code;
   private UUID guid;
   private String name;
   private String message;
   private String description;
	private Severity severity;
	private int flags;
	private String tagList; 
   private Set<String> tags;

	/**
	 * Create new empty event template.
	 * 
	 * @param code Event code assigned by server
	 */
   public EventTemplate(int code)
	{
      this.code = code;
      guid = NXCommon.EMPTY_GUID;
      name = "";
      message = "";
      description = "";
		severity = Severity.NORMAL;
		flags = FLAG_WRITE_TO_LOG;
		tagList= "";
		tags = new HashSet<String>();
	}

	/**
	 * Create event template object from NXCP message.
	 * 
	 * @param msg NXCP message
    * @param baseId base field id
	 */
	public EventTemplate(final NXCPMessage msg, long baseId)
	{
      code = msg.getFieldAsInt32(baseId + 1);
      description = msg.getFieldAsString(baseId + 2);
      name = msg.getFieldAsString(baseId + 3);
		severity = Severity.getByValue(msg.getFieldAsInt32(baseId + 4));
		flags = msg.getFieldAsInt32(baseId + 5);
		message = msg.getFieldAsString(baseId + 6);

      tags = new HashSet<String>();
		tagList = msg.getFieldAsString(baseId + 7);
		if ((tagList != null) && !tagList.isEmpty())
		{
		   for(String s : tagList.split(","))
		      tags.add(s);
		}

      guid = msg.getFieldAsUUID(baseId + 8);
      if (guid == null)
         guid = NXCommon.EMPTY_GUID;
	}

	/**
	 * Copy constructor.
	 * 
	 * @param src Original event template object
	 */
	public EventTemplate(final EventTemplate src)
	{
      code = src.code;
      guid = src.guid;
      name = src.name;
      description = src.description;
      severity = ((EventTemplate)src).severity;
      flags = ((EventTemplate)src).flags;
      message = ((EventTemplate)src).message;
      tagList = src.tagList;
      tags = new HashSet<String>(src.tags);
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.client.events.EventObject#fillMessage(org.netxms.base.NXCPMessage)
	 */
	public void fillMessage(NXCPMessage msg)
	{
      msg.setFieldInt32(NXCPCodes.VID_EVENT_CODE, (int) code);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_MESSAGE, message);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      msg.setFieldInt32(NXCPCodes.VID_SEVERITY, severity.getValue());
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
      if (!tags.isEmpty())
      {
         StringBuilder sb = new StringBuilder();
         for(String s : tags)
         {
            if (sb.length() > 0)
               sb.append(',');
            sb.append(s);
         }
         msg.setField(NXCPCodes.VID_TAGS, sb.toString());
      }
	}

	/**
	 * Set all attributes from another event template object.
	 * 
	 * @param src Original event template object
	 */
	public void setAll(final EventTemplate src)
	{
      code = src.code;
      guid = src.guid;
      name = src.name;
      message = src.message;
      description = src.description;
		severity = src.severity;
		flags = src.flags;
		tagList = src.tagList;
		tags = new HashSet<String>(src.tags);
	}

	/**
	 * @return the severity
	 */
	public Severity getSeverity()
	{
		return severity;
	}

	/**
	 * @param severity the severity to set
	 */
	public void setSeverity(Severity severity)
	{
		this.severity = severity;
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
	 * @return the message
	 */
	public String getMessage()
	{
		return message;
	}

	/**
	 * @param message the message to set
	 */
	public void setMessage(String message)
	{
		this.message = message;
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
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @param description the description to set
    */
   public void setDescription(String description)
   {
      this.description = description;
   }   

   /**
    * @return the code
    */
   public int getCode()
   {
      return code;
   }

   /**
    * @return the guid
    */
   public UUID getGuid()
   {
      return guid;
   }

   /**
    * @param code the code to set
    */
   public void setCode(int code)
   {
      this.code = code;
   }

   /**
    * @return the tags
    */
   public Set<String> getTags()
   {
      return tags;
   }

   /**
    * @param tags the tags to set
    */
   public void setTags(Set<String> tags)
   {
      this.tags = tags;
      if (!tags.isEmpty())
      {
         StringBuilder sb = new StringBuilder();
         for(String s : tags)
         {
            if (sb.length() > 0)
               sb.append(',');
            sb.append(s);
         }
         tagList = sb.toString();
      }
      else
      {
         tagList = "";
      }
   }
   
   /**
    * Get tags as list
    * 
    * @return tags as comma separated list
    */
   public String getTagList()
   {
      return (tagList != null) ? tagList : "";
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "EventTemplate [code=" + code + ", name=" + name + ", message=" + message + ", description=" + description
            + ", severity=" + severity + ", flags=" + flags + ", tagList=" + tagList + ", tags=" + tags + "]";
   }
}
