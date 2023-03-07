/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Reden Solutions
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
package org.netxms.client.asset;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.AMDataType;
import org.netxms.client.constants.AMSystemType;

/**
 * Assert management class 
 */
public class AssetManagementAttribute
{
   String name;
   String displayName;
   AMDataType dataType;
   boolean isMandatory;
   boolean isUnique;
   String autofillScript;
   int rangeMin;
   int rangeMax;
   AMSystemType systemType;
   Map<String, String> enumMapping;
   
   /**
    * New item constructor
    */
   public AssetManagementAttribute()
   {
      name = "";
      displayName = "";
      dataType = AMDataType.STRING;
      isMandatory = false;
      isUnique = false;
      autofillScript = "";
      rangeMin = -1;
      rangeMax = - 1;
      systemType = AMSystemType.NONE;
      enumMapping = new HashMap<String, String>();      
   }
   
   /**
    * Constructor form server message 
    * 
    * @param response message with data
    * @param fieldId base field id
    */
   public AssetManagementAttribute(NXCPMessage response, long fieldId)
   {
      name = response.getFieldAsString(fieldId++);
      displayName = response.getFieldAsString(fieldId++);
      dataType = AMDataType.getByValue(response.getFieldAsInt32(fieldId++));
      isMandatory = response.getFieldAsBoolean(fieldId++);
      isUnique = response.getFieldAsBoolean(fieldId++);
      autofillScript = response.getFieldAsString(fieldId++);
      rangeMin = response.getFieldAsInt32(fieldId++);
      rangeMax = response.getFieldAsInt32(fieldId++);
      systemType = AMSystemType.getByValue(response.getFieldAsInt32(fieldId++));
      int enumMappingCount = response.getFieldAsInt32(fieldId++);
      enumMapping = new HashMap<String, String>();   
      for (int i = 0; i < enumMappingCount; i++)
      {
         enumMapping.put(response.getFieldAsString(fieldId++), response.getFieldAsString(fieldId++));
      }
   }

   /**
    * Fill message with attribute data
    * 
    * @param msg message to fill
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_DISPLAY_NAME, displayName);
      msg.setFieldInt32(NXCPCodes.VID_DATA_TYPE, dataType.getValue());
      msg.setField(NXCPCodes.VID_IS_MANDATORY, isMandatory);
      msg.setField(NXCPCodes.VID_IS_UNIQUE, isUnique);
      msg.setField(NXCPCodes.VID_SCRIPT, autofillScript);
      msg.setFieldInt32(NXCPCodes.VID_RANGE_MIN, rangeMin);
      msg.setFieldInt32(NXCPCodes.VID_RANGE_MAX, rangeMax);
      msg.setFieldInt32(NXCPCodes.VID_SYSTEM_TYPE, systemType.getValue());
      msg.setFieldInt32(NXCPCodes.VID_ENUM_COUNT, enumMapping.size());
      long baseId = NXCPCodes.VID_AM_ENUM_MAP_BASE;
      for (Entry<String, String> entry : enumMapping.entrySet())
      {
         msg.setField(baseId++, entry.getKey());
         msg.setField(baseId++, entry.getValue());         
      }
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
    * @return the displayName
    */
   public String getDisplayName()
   {
      return displayName;
   }

   /**
    * @param displayName the displayName to set
    */
   public void setDisplayName(String displayName)
   {
      this.displayName = displayName;
   }

   /**
    * @return the dataType
    */
   public AMDataType getDataType()
   {
      return dataType;
   }

   /**
    * @param dataType the dataType to set
    */
   public void setDataType(AMDataType dataType)
   {
      this.dataType = dataType;
   }

   /**
    * @return the isMandatory
    */
   public boolean isMandatory()
   {
      return isMandatory;
   }

   /**
    * @param isMandatory the isMandatory to set
    */
   public void setMandatory(boolean isMandatory)
   {
      this.isMandatory = isMandatory;
   }

   /**
    * @return the isUnique
    */
   public boolean isUnique()
   {
      return isUnique;
   }

   /**
    * @param isUnique the isUnique to set
    */
   public void setUnique(boolean isUnique)
   {
      this.isUnique = isUnique;
   }

   /**
    * @return the autofillScript
    */
   public String getAutofillScript()
   {
      return autofillScript;
   }

   /**
    * @param autofillScript the autofillScript to set
    */
   public void setAutofillScript(String autofillScript)
   {
      this.autofillScript = autofillScript;
   }

   /**
    * @return the rangeMin
    */
   public int getRangeMin()
   {
      return rangeMin;
   }

   /**
    * @param rangeMin the rangeMin to set
    */
   public void setRangeMin(int rangeMin)
   {
      this.rangeMin = rangeMin;
   }

   /**
    * @return the rangeMax
    */
   public int getRangeMax()
   {
      return rangeMax;
   }

   /**
    * @param rangeMax the rangeMax to set
    */
   public void setRangeMax(int rangeMax)
   {
      this.rangeMax = rangeMax;
   }

   /**
    * @return the systemType
    */
   public AMSystemType getSystemType()
   {
      return systemType;
   }

   /**
    * @param systemType the systemType to set
    */
   public void setSystemType(AMSystemType systemType)
   {
      this.systemType = systemType;
   }

   /**
    * @return the enumMapping
    */
   public Map<String, String> getEnumMapping()
   {
      return enumMapping;
   }

   /**
    * @param enumMapping the enumMapping to set
    */
   public void setEnumMapping(Map<String, String> enumMapping)
   {
      this.enumMapping = enumMapping;
   }
}
