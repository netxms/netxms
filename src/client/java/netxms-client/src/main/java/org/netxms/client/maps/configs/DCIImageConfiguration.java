/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.maps.configs;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import org.netxms.base.Glob;
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.DciValue;

/**
 * Base class for DCI image configuration 
 */
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

   private DCIImageRule[] dciRuleList = new DCIImageRule[0];
   private MapImageDataSource dci = new MapImageDataSource();
   private UUID defaultImage = null;

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
    *
    * @return TODO
    */
   public List<DCIImageRule> getRulesAsList()
   {
      List<DCIImageRule> rules = new ArrayList<DCIImageRule>();
      if (containRuleList())
      {
         for(DCIImageRule dci : dciRuleList)
            rules.add(dci);
      }
      return rules;
   }

   /**
    * @return the dci
    */
   public MapImageDataSource getDci()
   {
      return dci;
   }

   /**
    * @param dci the dci to set
    */
   public void setDci(MapImageDataSource dci)
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

   /**
    * Check if this configuration has rule list.
    *
    * @return true if this configuration has rule list
    */
   public boolean containRuleList()
   {
      if ((dciRuleList != null) && (dciRuleList.length > 0))
         return true;
      return false;
   }

   /**
    * Checks is any rule applicable on last value. All except "like" and "not like" are compared with <code>T.compateTo(T)</code>.
    * Values for "Like" and "not like" can be provided as regular expressions so they are always compared as a strings and with help
    * of <code>Glob.matchIgnoreCase(pattern, string)</code>.
    *
    * @param dciValue DCI value to check
    * @return correct image according to last value
    */
   public UUID getCorrectImage(DciValue dciValue)
   {
      UUID image = null;
      if (containRuleList() && dciValue != null)
      {
         for(int i = 0; i < dciRuleList.length; i++)
         {
            switch(dciRuleList[i].getComparisonType())
            {
               case OP_LE:
                  if (compareValues(dciValue.getValue(), dciRuleList[i].getCompareValue(), dciValue.getDataType()) < 0)
                     image = dciRuleList[i].getImage();
                  break;
               case OP_LE_EQ:
                  if (compareValues(dciValue.getValue(), dciRuleList[i].getCompareValue(), dciValue.getDataType()) <= 0)
                     image = dciRuleList[i].getImage();
                  break;
               case OP_EQ:
                  if (compareValues(dciValue.getValue(), dciRuleList[i].getCompareValue(), dciValue.getDataType()) == 0)
                     image = dciRuleList[i].getImage();
                  break;
               case OP_GT_EQ:
                  if (compareValues(dciValue.getValue(), dciRuleList[i].getCompareValue(), dciValue.getDataType()) >= 0)
                     image = dciRuleList[i].getImage();
                  break;
               case OP_GT:
                  if (compareValues(dciValue.getValue(), dciRuleList[i].getCompareValue(), dciValue.getDataType()) > 0)
                     image = dciRuleList[i].getImage();
                  break;
               case OP_NE:
                  if (compareValues(dciValue.getValue(), dciRuleList[i].getCompareValue(), dciValue.getDataType()) != 0)
                     image = dciRuleList[i].getImage();
                  break;
               case OP_LIKE:
                  if (Glob.matchIgnoreCase(dciRuleList[i].getCompareValue(), dciValue.getValue()))
                     image = dciRuleList[i].getImage();
                  break;
               case OP_NOTLIKE:
                  if (!Glob.matchIgnoreCase(dciRuleList[i].getCompareValue(), dciValue.getValue()))
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
   private int compareValues(String value1, String value2, DataType dataType)
   {
      int result = 0;
      try
      {
         switch(dataType)
         {
            case INT32:
            case UINT32:
               result = Integer.valueOf(value1).compareTo(Integer.valueOf(value2));
               break;
            case FLOAT:
               result = Float.valueOf(value1).compareTo(Float.valueOf(value2));
               break;
            case INT64:
            case UINT64:
               result = Long.valueOf(value1).compareTo(Long.valueOf(value2));
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
