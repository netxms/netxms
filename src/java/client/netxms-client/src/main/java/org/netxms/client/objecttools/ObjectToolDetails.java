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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;

import org.netxms.base.Logger;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Detailed information about object tool
 *
 */
public class ObjectToolDetails extends ObjectTool
{
	private boolean modified;
	private List<Long> accessList;
	private List<ObjectToolTableColumn> columns;
	
	/**
	 * Create new tool object
	 * 
	 * @param toolId tool id
	 * @param type tool type
	 * @param name tool name
	 */
	public ObjectToolDetails(long toolId, int type, String name)
	{
		modified = false;
		this.id = toolId;
		this.type = type;
		this.name = name;

		data = "";
		flags = 0;
		description = "";
		filter = new ObjectToolFilter();
		confirmationText = "";
		accessList = new ArrayList<Long>(0);
		columns = new ArrayList<ObjectToolTableColumn>(0);
		commandName = "";
		commandShortName = "";
		imageData = null;
		inputFields = new HashMap<String, InputField>();

		createDisplayName();
	}
	
	/**
	 * Create object tool from NXCP message containing detailed tool information.
	 * Intended to be called only by NXCSession methods.
	 * 
	 * @param msg NXCP message
	 */
	public ObjectToolDetails(NXCPMessage msg)
	{
		modified = false;
		id = msg.getFieldAsInt64(NXCPCodes.VID_TOOL_ID);
		name = msg.getFieldAsString(NXCPCodes.VID_NAME);
		type = msg.getFieldAsInt32(NXCPCodes.VID_TOOL_TYPE);
		data = msg.getFieldAsString(NXCPCodes.VID_TOOL_DATA);
		flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
		description = msg.getFieldAsString(NXCPCodes.VID_DESCRIPTION);
		String filterData = msg.getFieldAsString(NXCPCodes.VID_TOOL_FILTER);
		confirmationText = msg.getFieldAsString(NXCPCodes.VID_CONFIRMATION_TEXT);
		commandName = msg.getFieldAsString(NXCPCodes.VID_COMMAND_NAME);
      commandShortName = msg.getFieldAsString(NXCPCodes.VID_COMMAND_SHORT_NAME);
		imageData = msg.getFieldAsBinary(NXCPCodes.VID_IMAGE_DATA);
      try
      {
         filter = ObjectToolFilter.createFromXml(filterData);
      }
      catch(Exception e)
      {
         filter = new ObjectToolFilter();
         Logger.debug("ObjectToolDetails.ObjectToolDetails", "Failed to convert object tool filter to string");
      }
		
		Long[] acl = msg.getFieldAsUInt32ArrayEx(NXCPCodes.VID_ACL);
		accessList = (acl != null) ? new ArrayList<Long>(Arrays.asList(acl)) : new ArrayList<Long>(0);
		
		int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_COLUMNS);
		columns = new ArrayList<ObjectToolTableColumn>(count);
		long varId = NXCPCodes.VID_COLUMN_INFO_BASE;
		for(int i = 0; i < count; i++)
		{
			columns.add(new ObjectToolTableColumn(msg, varId));
			varId += 4;
		}

      count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_FIELDS);
      inputFields = new HashMap<String, InputField>(count);
      
      long fieldId = NXCPCodes.VID_FIELD_LIST_BASE;
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
	 * Fill NXCP message with tool's data.
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(NXCPMessage msg)
	{
	   String filterData;
      try
      {
         filterData = filter.createXml();
      }
      catch(Exception e)
      {
         filterData = "";
         Logger.debug("ObjectTool.ObjectTool", "Failed to convert object tool filter to XML");
      }
		msg.setFieldInt32(NXCPCodes.VID_TOOL_ID, (int)id);
		msg.setField(NXCPCodes.VID_NAME, name);
		msg.setField(NXCPCodes.VID_DESCRIPTION, description);
		msg.setField(NXCPCodes.VID_TOOL_FILTER, filterData);
		msg.setField(NXCPCodes.VID_CONFIRMATION_TEXT, confirmationText);
		msg.setField(NXCPCodes.VID_TOOL_DATA, data);
		msg.setFieldInt16(NXCPCodes.VID_TOOL_TYPE, type);
		msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
		msg.setField(NXCPCodes.VID_COMMAND_NAME, commandName);
      msg.setField(NXCPCodes.VID_COMMAND_SHORT_NAME, commandShortName);
		if (imageData != null)
		   msg.setField(NXCPCodes.VID_IMAGE_DATA, imageData);

		msg.setFieldInt32(NXCPCodes.VID_ACL_SIZE, accessList.size());
		msg.setField(NXCPCodes.VID_ACL, accessList.toArray(new Long[accessList.size()]));
		
		msg.setFieldInt16(NXCPCodes.VID_NUM_COLUMNS, columns.size());
		long fieldId = NXCPCodes.VID_COLUMN_INFO_BASE;
		for(int i = 0; i < columns.size(); i++)
		{
			ObjectToolTableColumn c = columns.get(i);
			msg.setField(fieldId++, c.getName());
			msg.setField(fieldId++, c.getSnmpOid());
			msg.setFieldInt16(fieldId++, c.getFormat());
			msg.setFieldInt16(fieldId++, c.getSubstringIndex());
		}
		
		msg.setFieldInt16(NXCPCodes.VID_NUM_FIELDS, inputFields.size());
      fieldId = NXCPCodes.VID_FIELD_LIST_BASE;
      for(InputField f : inputFields.values())
      {
         f.fillMessage(msg, fieldId);
         fieldId += 10;
      }
	}

	/**
	 * @return the accessList
	 */
	public List<Long> getAccessList()
	{
		return accessList;
	}

	/**
	 * @return the columns
	 */
	public List<ObjectToolTableColumn> getColumns()
	{
		return columns;
	}
	
	/**
	 * Add or replace input field definition.
	 * 
	 * @param f
	 */
	public void addInputField(InputField f)
	{
	   inputFields.put(f.getName(), f);
	}
	
	/**
	 * Set input field definitions
	 * 
	 * @param fields
	 */
	public void setInputFields(Collection<InputField> fields)
	{
	   inputFields.clear();
	   for(InputField f : fields)
	      inputFields.put(f.getName(), f);
	}

	/**
	 * @param id the id to set
	 */
	public void setId(long id)
	{
		this.id = id;
		modified = true;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
		createDisplayName();
		modified = true;
	}

	/**
	 * @param type the type to set
	 */
	public void setType(int type)
	{
		this.type = type;
		modified = true;
	}

	/**
	 * @param flags the flags to set
	 */
	public void setFlags(int flags)
	{
		this.flags = flags;
		modified = true;
	}

	/**
	 * @param description the description to set
	 */
	public void setDescription(String description)
	{
		this.description = description;
		modified = true;
	}

	/**
	 * @param snmpOid the snmpOid to set
	 */
	public void setSnmpOid(String snmpOid)
	{
	   filter.snmpOid = snmpOid;
		modified = true;
	}
	

	/**
    * @param toolTemplate the comma separated list of template name regexps
    */
   public void setToolTemplate(String toolTemplate)
   {
      filter.toolTemplate = toolTemplate;
      modified = true;
   }

   /**
    * @param toolOS  the comma separated list of OS name regexps
    */
   public void setToolOS(String toolOS)
   {
      filter.toolOS = toolOS;
      modified = true;
   }

	/**
	 * @param data the data to set
	 */
	public void setData(String data)
	{
		this.data = data;
		modified = true;
	}

	/**
	 * @param confirmationText the confirmationText to set
	 */
	public void setConfirmationText(String confirmationText)
	{
		this.confirmationText = confirmationText;
		modified = true;
	}

	/**
	 * @return the modified
	 */
	public boolean isModified()
	{
		return modified;
	}

	/**
	 * @param accessList the accessList to set
	 */
	public void setAccessList(List<Long> accessList)
	{
		this.accessList = accessList;
		modified = true;
	}

	/**
	 * @param columns the columns to set
	 */
	public void setColumns(List<ObjectToolTableColumn> columns)
	{
		this.columns = columns;
		modified = true;
	}
	
	/**
	 * @param commandName
	 */
	public void setCommandName(String commandName)
	{
	   this.commandName = commandName;
	   modified = true;
	}
	
   /**
    * @param commandShortName
    */
   public void setCommandShortName(String commandShortName)
   {
      this.commandShortName = commandShortName;
      modified = true;
   }
   
	/**
	 * @param imageData
	 */
	public void setImageData(byte[] imageData)
	{
	   this.imageData = imageData;
	   modified = true;
	}
}
