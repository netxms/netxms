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
package org.netxms.client.objecttools;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.regex.Pattern;
import org.netxms.base.Glob;
import org.netxms.base.Logger;
import org.netxms.base.NXCPMessage;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;

/**
 * NetXMS object tool representation
 *
 */
public class ObjectTool
{
	public static final int TYPE_INTERNAL       = 0;
	public static final int TYPE_ACTION         = 1;
	public static final int TYPE_TABLE_SNMP     = 2;
	public static final int TYPE_TABLE_AGENT    = 3;
	public static final int TYPE_URL            = 4;
	public static final int TYPE_LOCAL_COMMAND  = 5;
	public static final int TYPE_SERVER_COMMAND = 6;
	public static final int TYPE_FILE_DOWNLOAD  = 7;
	
	public static final int REQUIRES_SNMP             = 0x00000001;
	public static final int REQUIRES_AGENT            = 0x00000002;
	public static final int REQUIRES_OID_MATCH        = 0x00000004;
	public static final int ASK_CONFIRMATION          = 0x00000008;
	public static final int GENERATES_OUTPUT          = 0x00000010;
	public static final int DISABLED                  = 0x00000020;
	public static final int SHOW_IN_COMMANDS          = 0x00000040;
	public static final int REQUIRES_OS_MATCH         = 0x00000080;
   public static final int REQUIRES_TEMPLATE_MATCH   = 0x00000100;
	public static final int SNMP_INDEXED_BY_VALUE     = 0x00010000;
	
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
   protected ObjectToolFilter filter;
	protected byte[] imageData;
	protected Map<String, InputField> inputFields;

	/**
	 * Default implicit constructor.
	 */
	protected ObjectTool()
	{
	   filter = new ObjectToolFilter();
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
		try
      {
         filter = ObjectToolFilter.createFromXml(filterData);
      }
      catch(Exception e)
      {
         filter = new ObjectToolFilter();
         if (!filterData.isEmpty())
            Logger.debug("ObjectTool.ObjectTool", "Failed to create object tool filter from XML");
      }
		
		int count = msg.getFieldAsInt32(baseId + 11);
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
	 * Check if tool is applicable for given node.
	 * 
	 * @param node AbstractNode object
	 * @return true if tool is applicable for given node
	 */
	public boolean isApplicableForNode(AbstractNode node)
	{
		if (((flags & REQUIRES_SNMP) != 0) &&
			 ((node.getFlags() & AbstractNode.NF_IS_SNMP) == 0))
			return false;	// Node does not support SNMP
		
		if (((flags & REQUIRES_AGENT) != 0) &&
				 ((node.getFlags() & AbstractNode.NF_IS_NATIVE_AGENT) == 0))
				return false;	// Node does not have NetXMS agent
		
		if ((flags & REQUIRES_OID_MATCH) != 0)
		{
			if (!Glob.matchIgnoreCase(filter.snmpOid, node.getSnmpOID()))
				return false;	// OID does not match
		}
		
	   if ((flags & REQUIRES_OS_MATCH) != 0)
      {
	      boolean match = false;
	      String[] substrings = filter.toolOS.split(",");
	      for(int i = 0; i < substrings.length; i++)
	      {
	         if (Pattern.matches(substrings[i], node.getPlatformName()))
	         {
	            match = true;
	         }
	      }
	      if (!match)
	         return false;  //Not correct type of OS
      }
	    
	   if ((flags & REQUIRES_TEMPLATE_MATCH) != 0)
      {
	      boolean match = false;
         String[] substrings = filter.toolTemplate.split(",");
         Set<AbstractObject> parents = node.getAllParents(AbstractObject.OBJECT_TEMPLATE);
         for(AbstractObject parent : parents)
         {
            for(int i = 0; i < substrings.length; i++)
            {
               if (Pattern.matches(substrings[i], parent.getObjectName()))
               {
                  match = true;
               }
            }
         }
         if (!match)
            return false;  // Does not belong to those templates
      }
		
		return true;
	}

	/**
	 * Get input field definition by name
	 * 
	 * @param name
	 * @return
	 */
	public InputField getInputField(String name)
	{
	   return inputFields.get(name);
	}
	
	/**
	 * Get all input fields
	 * 
	 * @return
	 */
	public InputField[] getInputFields()
	{
	   return inputFields.values().toArray(new InputField[inputFields.size()]);
	}
	
   /**
	 * @return the id
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
	public int getType()
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
	 * @return
	 */
	public boolean isVisibleInCommands()
	{
	   return (flags & SHOW_IN_COMMANDS) != 0;
	}
	
	/**
	 * @return
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
    * @return the commandName
    */
   public String getCommandName()
   {
      return commandName;
   }
   
   /**
    * @return
    */
   public String getCommandDisplayName()
   {
      if ((commandName != null) && !commandName.isEmpty())
         return commandName;
      return displayName;
   }

   /**
    * @return the commandShortName
    */
   public String getCommandShortName()
   {
      return commandShortName;
   }

   /**
    * @return
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
    * @return the toolOS
    */
   public String getToolOS()
   {
      return filter.toolOS;
   }

   /**
    * @return the toolTemplate
    */
   public String getToolTemplate()
   {
      return filter.toolTemplate;
   }
}
