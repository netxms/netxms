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
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.AMDataType;
import org.netxms.client.constants.AMSystemType;

/**
 * Assert attribute (element of asset management schema).
 */
public class AssetAttribute
{
   String name;
   String displayName;
   AMDataType dataType;
   boolean mandatory;
   boolean unique;
   boolean hidden;
   String autofillScript;
   int rangeMin;
   int rangeMax;
   AMSystemType systemType;
   Map<String, String> enumValues;

   /**
    * Create new attribute
    */
   public AssetAttribute()
   {
      name = "";
      displayName = "";
      dataType = AMDataType.STRING;
      mandatory = false;
      unique = false;
      hidden = false;
      autofillScript = "";
      rangeMin = 0;
      rangeMax = 0;
      systemType = AMSystemType.NONE;
      enumValues = new HashMap<String, String>();      
   }

   /**
    * Create attribute from server message.
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public AssetAttribute(NXCPMessage msg, long baseId)
   {
      name = msg.getFieldAsString(baseId++);
      displayName = msg.getFieldAsString(baseId++);
      dataType = AMDataType.getByValue(msg.getFieldAsInt32(baseId++));
      mandatory = msg.getFieldAsBoolean(baseId++);
      unique = msg.getFieldAsBoolean(baseId++);
      hidden = msg.getFieldAsBoolean(baseId++);
      autofillScript = msg.getFieldAsString(baseId++);
      rangeMin = msg.getFieldAsInt32(baseId++);
      rangeMax = msg.getFieldAsInt32(baseId++);
      systemType = AMSystemType.getByValue(msg.getFieldAsInt32(baseId++));
      enumValues = msg.getStringMapFromFields(baseId + 1, baseId);
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
      msg.setField(NXCPCodes.VID_IS_MANDATORY, mandatory);
      msg.setField(NXCPCodes.VID_IS_UNIQUE, unique);
      msg.setField(NXCPCodes.VID_IS_HIDDEN, hidden);
      msg.setField(NXCPCodes.VID_SCRIPT, autofillScript);
      msg.setFieldInt32(NXCPCodes.VID_RANGE_MIN, rangeMin);
      msg.setFieldInt32(NXCPCodes.VID_RANGE_MAX, rangeMax);
      msg.setFieldInt32(NXCPCodes.VID_SYSTEM_TYPE, systemType.getValue());
      msg.setFieldsFromStringMap(enumValues, NXCPCodes.VID_AM_ENUM_MAP_BASE, NXCPCodes.VID_ENUM_COUNT);
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
    * Display name or name if display name is empty 
    * 
    * @return name that should be used as a display name
    */
   public String getEffectiveDisplayName()
   {
      return displayName.isEmpty() ? name : displayName;
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
      return mandatory;
   }

   /**
    * @param isMandatory the isMandatory to set
    */
   public void setMandatory(boolean isMandatory)
   {
      this.mandatory = isMandatory;
   }

   /**
    * @return the isUnique
    */
   public boolean isUnique()
   {
      return unique;
   }

   /**
    * @param isUnique the isUnique to set
    */
   public void setUnique(boolean isUnique)
   {
      this.unique = isUnique;
   }

   /**
    * @return the hidden
    */
   public boolean isHidden()
   {
      return hidden;
   }

   /**
    * @param hidden the hidden to set
    */
   public void setHidden(boolean hidden)
   {
      this.hidden = hidden;
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
   public Map<String, String> getEnumValues()
   {
      return enumValues;
   }

   /**
    * @param enumMapping the enumMapping to set
    */
   public void setEnumValues(Map<String, String> enumMapping)
   {
      this.enumValues = enumMapping;
   }
}
