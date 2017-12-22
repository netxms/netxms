/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.client.datacollection;

import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.util.Arrays;
import java.util.IllegalFormatException;

/**
 * Data formatter
 */
public class DataFormatter
{
   private String formatString;
   private boolean useBinaryMultipliers;
   private int dataType;
   
   /**
    * @param formatString The format
    * @param dataType The data type
    * @param useBinaryMultipliers true if multipliers should be used
    */
   public DataFormatter(String formatString, int dataType, boolean useBinaryMultipliers)
   {
      this.formatString = formatString;
      this.dataType = dataType;
      this.useBinaryMultipliers = useBinaryMultipliers;
   }
   
   /**
    * @param formatString The format
    * @param dataType The data type
    */
   public DataFormatter(String formatString, int dataType)
   {
      this(formatString, dataType, false);
   }
   
   /**
    * @param formatString The format
    */
   public DataFormatter(int precision, String formatString)
   {
      this(formatString, DataCollectionObject.DT_STRING, false);
   }
   
   /**
    * Format value
    * 
    * @param value The value
    * @return The format
    */
   public String format(String value)
   {
      StringBuilder sb = new StringBuilder();
      char[] format = formatString.toCharArray();
      
      for(int i = 0; i < format.length; i++)
      {
         if (format[i] == '%')
         {
            i++;
            if (format[i] == '%')
            {
               sb.append('%');
            }
            else
            {
               int j;
               for(j = i; (j < format.length) && !Character.isLetter(format[j]); j++);
               
               boolean useMultipliers = false;
               if (format[i] == '*')
               {
                  i++;
                  useMultipliers = true;
               }
               
               final String f = "%" + new String(Arrays.copyOfRange(format, i, j + 1));
               i = j;
               
               try
               {
                  Value v = getValueForFormat(value, useMultipliers, format[j] == 's');
                  sb.append(String.format(f, v.value));
                  sb.append(v.suffix);
               }
               catch(IllegalFormatException e)
               {
                  sb.append("<INVALID FORMAT> (");
                  sb.append(f);
                  sb.append(")");
               }
            }
         }
         else
         {
            sb.append(format[i]);
         }
      }
      return sb.toString();
   }
   
   private static final long[] DECIMAL_MULTIPLIERS = { 1L, 1000L, 1000000L, 1000000000L, 1000000000000L, 1000000000000000L }; 
   private static final long[] BINARY_MULTIPLIERS = { 0x400L, 0x100000L, 0x40000000L, 0x10000000000L, 0x1000000000000000L };
   private static final String[] SUFFIX = { "", " k", " M", " G", " T", " P" }; 

   /**
    * Get value ready for formatter
    * 
    * @param useMultipliers
    * @return
    */
   private Value getValueForFormat(String value, boolean useMultipliers, boolean stringOutput)
   {
      Value v = new Value();
      
      if ((dataType != DataCollectionObject.DT_INT) && 
          (dataType != DataCollectionObject.DT_UINT) &&
          (dataType != DataCollectionObject.DT_INT64) &&
          (dataType != DataCollectionObject.DT_UINT64) &&
          (dataType != DataCollectionObject.DT_FLOAT))
      {
         v.value = value;
         return v;
      }
      
      try
      {
         if (useMultipliers)
         {
            final long[] multipliers = useBinaryMultipliers ? BINARY_MULTIPLIERS : DECIMAL_MULTIPLIERS;
            Double d = Double.parseDouble(value);
            int i;
            for(i = multipliers.length - 1; i >= 0; i--)
            {
               if ((d >= multipliers[i]) || (d <= -multipliers[i]))
                  break;
            }
            if (i >= 0)
            {
               if (stringOutput)
               {
                  NumberFormat nf = NumberFormat.getNumberInstance();
                  nf.setMaximumFractionDigits(2);
                  v.value = nf.format(d / (double)multipliers[i]);
               }
               else
               {
                  v.value = Double.valueOf(d / (double)multipliers[i]);
               }
               v.suffix = SUFFIX[i];
            }
            else if (stringOutput)
            {
               v.value = value;
            }
            else
            {
               if (dataType == DataCollectionObject.DT_FLOAT)
                  v.value = Double.parseDouble(value);
               else
                  v.value = Long.parseLong(value);
            }
         }
         else if (stringOutput)
         {
            v.value = value;
         }
         else
         {
            if (dataType == DataCollectionObject.DT_FLOAT)
               v.value = Double.parseDouble(value);
            else
               v.value = Long.parseLong(value);
         }
      }     
      catch(NumberFormatException e)
      {
         v.value = value;
      }
      return v;
   }
   
   /**
    * Calculate precision of number
    * 
    * @param number to calculate precision of
    * @return decimal place count
    */
   private static int calculatePrecision(double number)
   {
      int i = 0;
      if (number == 0 || number >= 1)
         return i;
      for(i = 1; i < 1000; i++)
      {
         if (((number) *= 10) >= 1)
            break;
      }
      return i;
   }
   
   /**
    * Get rounded value for char labels
    * 
    * @param value to round
    * @param step of label
    * @return rounded value
    */
   public static String roundDecimalValue(double value, double step, int maxPrecision)
   {
      if (value == 0)
         return "0";
      
      double absValue = Math.abs(value);
      final long[] multipliers = DECIMAL_MULTIPLIERS;
      
      int i;
      for(i = multipliers.length - 1; i >= 0 ; i--)
      {
         if (absValue >= multipliers[i])
            break;
      }

      int precision;
      if (step < 1 || i < 0)
         precision = (calculatePrecision(step) > maxPrecision) ? maxPrecision : calculatePrecision(step);
      else
         precision = (calculatePrecision(step / multipliers[i]) > maxPrecision) ? maxPrecision : calculatePrecision(step / multipliers[i]);
      
      DecimalFormat df = new DecimalFormat();
      df.setMaximumFractionDigits(precision);
      return df.format((i < 0 ? value : (value / multipliers[i]))) + (i < 0 ? "" : SUFFIX[i]);
      /*return new DecimalFormat("0" + (precision > FORMATS.length ? FORMATS[FORMATS.length - 1] : FORMATS[precision]))
            .format((i < 0 ? value : (value / multipliers[i]))) + (i < 0 ? "" : SUFFIX[i]);*/
   }

   /**
    * Class to hold value for formatter
    */
   private class Value
   {
      Object value;
      String suffix = "";
   }
}
