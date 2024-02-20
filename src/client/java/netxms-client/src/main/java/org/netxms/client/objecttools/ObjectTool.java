/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.client.objecttools;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.netxms.base.NXCPMessage;
import org.netxms.client.InputField;
import org.netxms.client.ObjectMenuFilter;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.client.xml.XMLTools;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * NetXMS object tool representation
 */
public class ObjectTool implements ObjectAction
{
	public static final int TYPE_INTERNAL       = 0;
	public static final int TYPE_ACTION         = 1;
	public static final int TYPE_SNMP_TABLE     = 2;
	public static final int TYPE_AGENT_LIST     = 3;
	public static final int TYPE_URL            = 4;
	public static final int TYPE_LOCAL_COMMAND  = 5;
	public static final int TYPE_SERVER_COMMAND = 6;
	public static final int TYPE_FILE_DOWNLOAD  = 7;
   public static final int TYPE_SERVER_SCRIPT  = 8;
   public static final int TYPE_AGENT_TABLE    = 9;
   public static final int TYPE_SSH_COMMAND    = 10;

   public static final int ASK_CONFIRMATION          = 0x00000001;
   public static final int GENERATES_OUTPUT          = 0x00000002;
   public static final int DISABLED                  = 0x00000004;
   public static final int SHOW_IN_COMMANDS          = 0x00000008;
   public static final int SNMP_INDEXED_BY_VALUE     = 0x00000010;
   public static final int RUN_IN_CONTAINER_CONTEXT  = 0x00000020;
   public static final int SUPPRESS_SUCCESS_MESSAGE  = 0x00000040;
   public static final int SETUP_TCP_TUNNEL          = 0x00000080;

   private static final Logger logger = LoggerFactory.getLogger(ObjectTool.class);

	protected long id;
	protected String name;
	protected String displayName;
	protected int type;
	protected int flags;
	protected String description;
	protected String data;
	protected String confirmationText;
	protected String commandName;
   protected String commandShortName;
   protected ObjectMenuFilter filter;
	protected byte[] imageData;
   protected int remotePort;
	protected Map<String, InputField> inputFields;

	/**
	 * Default implicit constructor.
	 */
	protected ObjectTool()
	{
	   filter = new ObjectMenuFilter();
	   inputFields = new HashMap<String, InputField>(0);
	}

	/**
	 * Create object tool from NXCP message. Intended to be called only by NXCSession methods.
	 * 
	 * @param msg NXCP message
	 * @param baseId Base variable ID
	 */
	public ObjectTool(NXCPMessage msg, long baseId)
	{
		id = msg.getFieldAsInt64(baseId);
		name = msg.getFieldAsString(baseId + 1);
		type = msg.getFieldAsInt32(baseId + 2);
		data = msg.getFieldAsString(baseId + 3);
		flags = msg.getFieldAsInt32(baseId + 4);
      String filterData = msg.getFieldAsString(baseId + 6);
		description = msg.getFieldAsString(baseId + 5);
		confirmationText = msg.getFieldAsString(baseId + 7);
		commandName = msg.getFieldAsString(baseId + 8);
      commandShortName = msg.getFieldAsString(baseId + 9);
		imageData = msg.getFieldAsBinary(baseId + 10);
      remotePort = msg.getFieldAsInt32(baseId + 11);
		try
      {
         filter = XMLTools.createFromXml(ObjectMenuFilter.class, filterData);
      }
      catch(Exception e)
      {
         filter = new ObjectMenuFilter();
         if (!filterData.isEmpty())
            logger.debug("Cannot create ObjectMenuFilter object from XML document", e);
      }

      int count = msg.getFieldAsInt32(baseId + 19);
      inputFields = new HashMap<String, InputField>(count);

		long fieldId = baseId + 20;
		for(int i = 0; i < count; i++)
		{
		   InputField f = new InputField(msg, fieldId);
		   inputFields.put(f.getName(), f);
		   fieldId += 10;
		}

		if ((type == TYPE_ACTION) || 
		    (type == TYPE_FILE_DOWNLOAD) || 
		    (type == TYPE_LOCAL_COMMAND) || 
		    (type == TYPE_SERVER_COMMAND) || 
		    (type == TYPE_URL))
		{
	      validateInputFields();
		}
		createDisplayName();
	}

	/**
	 * Check if all input fields referenced in tool have definitions
	 */
	protected void validateInputFields()
	{
	   Set<String> names = new HashSet<String>();
	   char[] in = data.toCharArray();
	   char br = 0;
	   int start = 0;
	   for(int i = 0; i < in.length; i++)
	   {
	      if (in[i] == br)
	      {
	         if (br == ')')
	            names.add(new String(Arrays.copyOfRange(in, start, i)));
            br = 0;
	      }
	      else if ((in[i] == '%') && (br == 0))
	      {
   	      i++;
   	      if (i == in.length)
   	         break;  // invalid input
   	      switch(in[i])
   	      {
   	         case '(':
   	            br = ')';
   	            break;
               case '[':
                  br = ']';
                  break;
               case '{':
                  br = '}';
                  break;
               case '<':
                  br = '>';
                  break;
   	         default:
   	            break;
   	      }
   	      start = i + 1;
	      }
	   }
	   
	   for(String n : names)
	   {
	      if (!inputFields.containsKey(n))
	      {
	         inputFields.put(n, new InputField(n));
	      }
	   }
	}

	/**
	 * Create display name
	 */
	protected void createDisplayName()
	{
		String[] parts = name.split("->");
		if (parts.length > 0)
		{
			displayName = parts[parts.length - 1].replace("&", "").trim();
		}
		else
		{
			displayName = name.replace("&", "").trim();
		}
	}

	/**
    * Check if tool is applicable for given object.
    * 
    * @param object AbstractObject object
    * @return true if tool is applicable for given object
    */
   public boolean isApplicableForObject(AbstractObject object)
   {
      return (isRunInContainerContext() == isContainerObject(object)) && filter.isApplicableForObject(object);
	}

	/**
	 * Get input field definition by name
	 * 
	 * @param name input field name
	 * @return input field or null if not found
	 */
	public InputField getInputField(String name)
	{
	   return inputFields.get(name);
	}

	/**
	 * Get all input fields
	 * 
	 * @return list of all defined input fields
	 */
	public InputField[] getInputFields()
	{
	   return inputFields.values().toArray(new InputField[inputFields.size()]);
	}

   /**
    * Get tool ID
    * 
	 * @return tool ID
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the type
	 */
	public int getToolType()
	{
		return type;
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

	/**
    * @return the remotePort
    */
   public int getRemotePort()
   {
      return remotePort;
   }

   /**
    * Check if this tool should be visible in commands
    * 
    * @return true if this tool should be visible in commands
    */
	public boolean isVisibleInCommands()
	{
	   return (flags & SHOW_IN_COMMANDS) != 0;
	}
	
	/**
	 * Check if tool is enabled
	 * 
	 * @return true if enabled
	 */
	public boolean isEnabled()
	{
	   return (flags & DISABLED) == 0;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the snmpOid
	 */
	public String getSnmpOid()
	{
		return filter.snmpOid;
	}

	/**
	 * @return the data
	 */
	public String getData()
	{
		return data;
	}

	/**
	 * @return the confirmationText
	 */
	public String getConfirmationText()
	{
		return confirmationText;
	}

	/**
	 * @return the displayName
	 */
	public String getDisplayName()
	{
		return displayName;
	}

   /**
    * Get command name
    * 
    * @return command name
    */
   public String getCommandName()
   {
      return commandName;
   }
   
   /**
    * Get command display name
    * 
    * @return command display name
    */
   public String getCommandDisplayName()
   {
      if ((commandName != null) && !commandName.isEmpty())
         return commandName;
      return displayName;
   }

   /**
    * Get command short name
    * 
    * @return command short name
    */
   public String getCommandShortName()
   {
      return commandShortName;
   }

   /**
    * Get command short display name
    * 
    * @return command short display name
    */
   public String getCommandShortDisplayName()
   {
      if ((commandShortName != null) && !commandShortName.isEmpty())
         return commandShortName;
      return getCommandDisplayName();
   }

   /**
    * @return the imageData
    */
   public byte[] getImageData()
   {
      return imageData;
   }

   /**
    * @return the toolRemOS
    */
   public String getToolNodeOS()
   {
      return filter.toolNodeOS;
   }
   
   /**
    * @return the toolLocOS
    */
   public String getToolWorkstationOS()
   {
      return filter.toolWorkstationOS;
   }

   /**
    * @return the toolTemplate
    */
   public String getToolTemplate()
   {
      return filter.toolTemplate;
   }

   /**
    * @see org.netxms.client.objecttools.ObjectAction#getMenuFilter()
    */
   @Override
   public ObjectMenuFilter getMenuFilter()
   {
      return filter;
   }

   /**
    * @see org.netxms.client.objecttools.ObjectAction#setMenuFilter(org.netxms.client.ObjectMenuFilter)
    */
   @Override
   public void setMenuFilter(ObjectMenuFilter filter)
   {
      this.filter = filter;      
   }

   /**
    * Returns the RUN_CONTAINER_CONTEXT flag status
    * 
    * @return if object tool capable of running on container
    */
   public boolean isRunInContainerContext()
   {
      return (flags & ObjectTool.RUN_IN_CONTAINER_CONTEXT) > 0;
   }

   /**
    * Check if given object is container.
    *
    * @param object object to test
    * @return true if given object is a container
    */
   public static boolean isContainerObject(AbstractObject object)
   {
      return ((object instanceof Collector) || (object instanceof Container) || (object instanceof Rack) || (object instanceof ServiceRoot) || (object instanceof Subnet) || (object instanceof Cluster) || (object instanceof Zone));
   }
}
