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
package org.netxms.client.objects.queries;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCommon;
import org.netxms.client.InputField;

/**
 * Predefined object query
 */
public class ObjectQuery
{
   private int id;
   private UUID guid;
   private String name;
   private String description;
   private String source;
   private boolean valid;
   private Map<String, InputField> inputFields;

   /**
    * Create new object query definition.
    *
    * @param name query name
    * @param description query description
    * @param source query source code
    */
   public ObjectQuery(String name, String description, String source)
   {
      this.id = 0;
      this.guid = NXCommon.EMPTY_GUID;
      this.name = name;
      this.description = description;
      this.source = source;
      this.valid = false;
      this.inputFields = new HashMap<String, InputField>();
   }

   /**
    * Create object query definition from NXCP message.
    * 
    * @param msg NXCP message
    * @param baseId single-element array containing base field ID. Will be updated to base field ID for next element in message.
    */
   public ObjectQuery(NXCPMessage msg, long[] baseId)
   {
      long fieldId = baseId[0];
      id = msg.getFieldAsInt32(fieldId++);
      guid = msg.getFieldAsUUID(fieldId++);
      name = msg.getFieldAsString(fieldId++);
      description = msg.getFieldAsString(fieldId++);
      source = msg.getFieldAsString(fieldId++);
      valid = msg.getFieldAsBoolean(fieldId++);
      fieldId += 3;

      int count = msg.getFieldAsInt32(fieldId++);
      inputFields = new HashMap<String, InputField>(count);
      for(int i = 0; i < count; i++)
      {
         InputField f = new InputField(msg, fieldId);
         inputFields.put(f.getName(), f);
         fieldId += 10;
      }

      baseId[0] = fieldId;
   }

   /**
    * Fill NXCP message with query data.
    *
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setFieldInt32(NXCPCodes.VID_QUERY_ID, id);
      msg.setField(NXCPCodes.VID_GUID, guid);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      msg.setField(NXCPCodes.VID_SCRIPT_CODE, source);

      msg.setFieldInt32(NXCPCodes.VID_NUM_PARAMETERS, inputFields.size());
      long fieldId = NXCPCodes.VID_PARAM_LIST_BASE;
      for(InputField f : inputFields.values())
      {
         f.fillMessage(msg, fieldId);
         fieldId += 10;
      }
   }

   /**
    * Get query name.
    *
    * @return query name
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
    * Get query source code.
    *
    * @return query source code
    */
   public String getSource()
   {
      return source;
   }

   /**
    * Set query source code.
    *
    * @param source new query source code
    */
   public void setSource(String source)
   {
      this.source = source;
   }

   /**
    * Get query ID.
    *
    * @return query ID
    */
   public int getId()
   {
      return id;
   }

   /**
    * @param id the id to set
    */
   public void setId(int id)
   {
      this.id = id;
   }

   /**
    * Get query GUID.
    *
    * @return query GUID
    */
   public UUID getGuid()
   {
      return guid;
   }

   /**
    * Check if query is valid (compiled successfully and ready to run).
    *
    * @return true if query is valid
    */
   public boolean isValid()
   {
      return valid;
   }

   /**
    * Get list of all defined input fields.
    *
    * @return list of all defined input fields
    */
   public List<InputField> getInputFields()
   {
      return new ArrayList<InputField>(inputFields.values());
   }

   /**
    * Get definition of input field with given name.
    *
    * @param name field name
    * @return input field definition or null
    */
   public InputField getInputField(String name)
   {
      return inputFields.get(name);
   }

   /**
    * Add input field. If field definition with same name already exist, it will be replaced.
    * 
    * @param f input field definition
    */
   public void addInputField(InputField f)
   {
      inputFields.put(f.getName(), f);
   }

   /**
    * Remove input field with given name.
    *
    * @param name field name
    */
   public void removeInputField(String name)
   {
      inputFields.remove(name);
   }

   /**
    * Replace existing definitions of input fields with newly provided set.
    *
    * @param fields new set of input field definitions
    */
   public void setInputFileds(Collection<InputField> fields)
   {
      inputFields.clear();
      for(InputField f : fields)
         inputFields.put(f.getName(), f);
   }
}
