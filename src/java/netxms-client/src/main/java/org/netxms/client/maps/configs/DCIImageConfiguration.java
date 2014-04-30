/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.client.maps.configs;

import java.io.StringWriter;
import java.io.Writer;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import org.netxms.base.Glob;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.xml.XMLTools;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementArray;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Base class for DCI image configuration 
 */
@Root(name="dciImageConfiguration")
public class DCIImageConfiguration
{   
   public static final int OP_LE       = 0;
   public static final int OP_LE_EQ    = 1;
   public static final int OP_EQ       = 2;
   public static final int OP_GT_EQ    = 3;
   public static final int OP_GT       = 4;
   public static final int OP_NE       = 5;
   public static final int OP_LIKE     = 6;
   public static final int OP_NOTLIKE  = 7;
   
   @ElementArray(required = true)
   private DCIImageRule[] dciRuleList = new DCIImageRule[0];
   
   @Element(required = true)
   private SingleDciConfig dci = new SingleDciConfig();
   
   @Element(required = true)
   private UUID defaultImage = null;
   
   /**
    * Create DCI list object from XML document
    * 
    * @param xml XML document
    * @return deserialized object
    * @throws Exception if the object cannot be fully deserialized
    */
   public static DCIImageConfiguration createFromXml(final String xml) throws Exception
   {
      Serializer serializer = XMLTools.createSerializer();    
      return serializer.read(DCIImageConfiguration.class, xml);
   }
   
   /**
    * Create XML from configuration.
    * 
    * @return XML document
    * @throws Exception if the schema for the object is not valid
    */
   public String createXml() throws Exception
   {
      Serializer serializer = XMLTools.createSerializer();
      Writer writer = new StringWriter();
      serializer.write(this, writer);
      return writer.toString();
   }
   
	/**
    * @return the dciRuleList
    */
   public DCIImageRule[] getDciRuleArray()
   {
      return dciRuleList;
   }

   /**
    * @param dciRuleList the dciRuleList to set
    */
   public void setDciRuleArray(DCIImageRule[] dciRuleList)
   {
      this.dciRuleList = dciRuleList;
   }
   
   /**
    * Returns DCI array as a list
    */
   public List<DCIImageRule> getRulesAsList()
   {
      List<DCIImageRule> ruleLisr = new ArrayList<DCIImageRule>();
      if(containRuleList())
      {
         for(DCIImageRule dci : dciRuleList)
            ruleLisr.add(dci);
      }
      return ruleLisr;
   }

   /**
    * @return the dci
    */
   public SingleDciConfig getDci()
   {
      return dci;
   }

   /**
    * @param dci the dci to set
    */
   public void setDci(SingleDciConfig dci)
   {
      this.dci = dci;
   }

   /**
    * @return the defaultImage
    */
   public UUID getDefaultImage()
   {
      return defaultImage;
   }

   /**
    * @param defaultImage the defaultImage to set
    */
   public void setDefaultImage(UUID defaultImage)
   {
      this.defaultImage = defaultImage;
   }
   
   public boolean containRuleList()
   {
      if((dciRuleList != null) && (dciRuleList.length > 0))
         return true;
      return false;
   }
   
   /**
    * Checks is any rule applicable on last value
    * All except line and not like are compared with help of T.compateTo(T).
    * Like and not like values can be given with regular expressions
    * so they are always compared as a strings and with help of 
    * Glob.matchIgnoreCase(pattern, string).
    * 
    * @return correct image according to last value
    */
   public UUID getCorrectImage(DciValue dciValue)
   {
      UUID image = null;
      if(containRuleList() && dciValue != null)
      {
         for(int i = 0; i < dciRuleList.length; i++)
         {
            switch(dciRuleList[i].getComparisonType())
            {
               case OP_LE:
                  if(compareValues(dciValue.getValue(), dciRuleList[i].getCompareValue(), dciValue.getDataType()) < 0)
                     image = dciRuleList[i].getImage();
                  break;       
               case OP_LE_EQ:
                  if(compareValues(dciValue.getValue(), dciRuleList[i].getCompareValue(), dciValue.getDataType()) <= 0)
                     image = dciRuleList[i].getImage();
                  break;  
               case OP_EQ:
                  if(compareValues(dciValue.getValue(), dciRuleList[i].getCompareValue(), dciValue.getDataType()) == 0)
                     image = dciRuleList[i].getImage();
                  break;  
               case OP_GT_EQ:
                  if(compareValues(dciValue.getValue(), dciRuleList[i].getCompareValue(), dciValue.getDataType()) >= 0)
                     image = dciRuleList[i].getImage();
                  break;  
               case OP_GT:
                  if(compareValues(dciValue.getValue(), dciRuleList[i].getCompareValue(), dciValue.getDataType()) > 0)
                     image = dciRuleList[i].getImage();
                  break;  
               case OP_NE:
                  if(compareValues(dciValue.getValue(), dciRuleList[i].getCompareValue(), dciValue.getDataType()) != 0)
                     image = dciRuleList[i].getImage();
                  break;  
               case OP_LIKE:
                  if(Glob.matchIgnoreCase(dciRuleList[i].getCompareValue(), dciValue.getValue())) 
                     image = dciRuleList[i].getImage();
                  break;  
               case OP_NOTLIKE:
                  if(!Glob.matchIgnoreCase(dciRuleList[i].getCompareValue(), dciValue.getValue()))
                     image = dciRuleList[i].getImage();
                  break;  
            }
            if (image != null) 
               break;
         }
      }
      return image != null ? image : defaultImage;
   }
   
   /**
    * This function converts both given values to given data Type and then 
    * compares with function T.compareTo(T). If one of values was not possible 
    * to convert, both values will be compared as a Strings. 
    * 
    * @param value1 new DCI value
    * @param value2 value to witch new value is compared
    * @param dataType type to witch both values will be converted
    * @return returns the result of T.compareTo(T) function 
    */
   private int compareValues(String value1, String value2, int dataType)
   {
      int result = 0;
      try
      {
         switch(dataType)
         {
            case DataCollectionObject.DT_INT:
            case DataCollectionObject.DT_UINT:
               result = (new Integer(value1)).compareTo(new Integer(value2));
               break;
            case DataCollectionObject.DT_FLOAT:
               result = (new Float(value1).compareTo(new Float(value2)));
               break;
            case DataCollectionItem.DT_INT64:
            case DataCollectionItem.DT_UINT64:
               result = (new Long(value1).compareTo(new Long(value2)));
               break;
            default:
               result = value1.compareTo(value2);
               break;
         }
      }     
      catch(NumberFormatException e)
      {
         result = value1.compareTo(value2);
      }
      return result;
   }

}
