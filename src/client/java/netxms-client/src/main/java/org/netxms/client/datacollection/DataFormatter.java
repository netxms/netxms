/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.client.datacollection;

import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.util.Arrays;
import java.util.IllegalFormatException;
import org.netxms.client.constants.DataType;

/**
 * Data formatter
 */
public class DataFormatter
{
   private String formatString;
   private DataType dataType;
   private MeasurementUnit unit;

   /**
    * Create new data formatter with binary multipliers option set to off.
    * 
    * @param formatString format string
    * @param dataType data type
    */
   public DataFormatter(String formatString, DataType dataType)
   {
      this.formatString = formatString;
      this.dataType = dataType;
      this.unit = null;
   }

   /**
    * Create new data formatter.
    * 
    * @param formatString format string
    * @param dataType data type
    * @param unit measurement unit
    */
   public DataFormatter(String formatString, DataType dataType, MeasurementUnit unit)
   {
      this.formatString = formatString;
      this.dataType = dataType;
      this.unit = unit;
      if (formatString == null || formatString.isEmpty())
         this.formatString = "%{m,u}s";
   }

   /**
    * Format value
    *
    * @param value The value
    * @param formatter Date and time formatter 
    * @return The format
    */
   public String format(String value, TimeFormatter formatter)
   {
      if (value == null || value.isEmpty())
         return "";

      StringBuilder sb = new StringBuilder();
      char[] format = formatString.toCharArray();

      for(int i = 0; i < format.length; i++)
      {
         if (format[i] == '%' && (i + 1 != format.length))
         {
            i++;
            if (format[i] == '%')
            {
               sb.append('%');
            }
            else
            {
               boolean useMultipliers = false;
               boolean useUnits = false;

               if (format[i] == '*')
               {
                  i++;
                  useMultipliers = true;
               }
               else if (format[i] == '{' && (i + 1 != format.length))
               {    
                  int end = i;
                  for(; (end < format.length) && (format[end] != '}'); end++) //find ending part
                     ;
                  if (format[end] == '}' && (end + 1 < format.length))
                  {
                     if (i + 1 != end)
                     {
                        String[] items = new String(Arrays.copyOfRange(format, i + 1, end)).split(",");
                        for (String item : items)
                        {
                           if ((item.trim().compareToIgnoreCase("u") == 0) || (item.trim().compareToIgnoreCase("units") == 0))
                           {
                              useUnits = unit != null;
                           }
                           else if ((item.trim().compareToIgnoreCase("m") == 0) || (item.trim().compareToIgnoreCase("multipliers") == 0))
                           {
                              useMultipliers = true;
                           }
                        }
                     }
                     i = end + 1;
                  }
               }

               int j;
               for(j = i; (j < format.length) && !Character.isLetter(format[j]); j++)
                  ;

               if (j + 1 < format.length && (format[j] == 't' || format[j] == 'T') && Character.isLetter(format[j + 1])) //t or T is prefix for date and time conversion characters
                  j++;

               final String f = "%" + new String(Arrays.copyOfRange(format, i, j + 1));
               i = j;

               if (useUnits && unit.getName().equals("Uptime"))
               {      
                  sb.append(formatter.formatUptime((long)Double.parseDouble(value)));   
               }
               else if (useUnits && unit.getName().equals("Epoch time"))
               {
                  sb.append(formatter.formatDateAndTime((long)Double.parseDouble(value)));
               }
               else
               {
                  try
                  {
                     Value v = getValueForFormat(value, useMultipliers, format[j] == 's' || format[j] == 'S', format[j] == 'd');
                     sb.append(String.format(f, v.value));
                     sb.append(v.suffix);
                     if (useUnits)
                     {
                        String unitName = unit.getName();
                        if (v.suffix.isEmpty() && !unitName.isEmpty())
                        {
                           sb.append(" ");
                        }
                        sb.append(unitName);
                     }
                  }
                  catch(IndexOutOfBoundsException | IllegalFormatException e) // out of bound may occur if there is no letter after % sign. Like: %*3
                  {
                     sb.append("<INVALID FORMAT> (");
                     sb.append(f.trim()); //trim required in case of out of bound
                     sb.append(")");
                  }
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

   private static final double[] DECIMAL_MULTIPLIERS = { 1L, 1000L, 1000000L, 1000000000L, 1000000000000L, 1000000000000000L };
   private static final double[] DECIMAL_MULTIPLIERS_SMALL = { 0.000000000000001, 0.000000000001, 0.000000001, 0.000001, 0.001, 1 };
   private static final double[] BINARY_MULTIPLIERS = { 1L, 0x400L, 0x100000L, 0x40000000L, 0x10000000000L, 0x4000000000000L };
   private static final String[] SUFFIX = { "", " k", " M", " G", " T", " P" };
   private static final String[] BINARY_SUFFIX = { "", " Ki", " Mi", " Gi", " Ti", " Pi" };
   private static final String[] SUFFIX_SMALL = { " f", " p", " n", " \u03bc", " m", "" };

   /**
    * Get value ready for formatter
    *
    * @param useMultipliers
    * @return formatted value
    */
   private Value getValueForFormat(String value, boolean useMultipliers, boolean stringOutput, boolean decimalOutput)
   {
      Value v = new Value();

      if ((dataType != DataType.INT32) && (dataType != DataType.UINT32) && (dataType != DataType.COUNTER32) &&
          (dataType != DataType.INT64) && (dataType != DataType.UINT64) && (dataType != DataType.COUNTER64) &&
          (dataType != DataType.FLOAT))
      {
         v.value = value;
         return v;
      }

      try
      {
         if (useMultipliers)
         {
            boolean useBinaryMultipliers = (unit != null) && unit.isBinary();
            int multiplierPower = (unit != null) ? unit.getMultipierPower() : 0;
            Double d = Double.parseDouble(value);
            boolean isSmallNumber = ((d > -0.01) && (d < 0.01) && d != 0 && multiplierPower <= 0 && unit.useMultiplierForUnit()) || multiplierPower < 0;
            double[] multipliers = isSmallNumber ? DECIMAL_MULTIPLIERS_SMALL : useBinaryMultipliers ? BINARY_MULTIPLIERS : DECIMAL_MULTIPLIERS;

            int i = 0;
            if (multiplierPower != 0)
            {
               if (isSmallNumber)
                  multiplierPower = 5 + multiplierPower;
               i = Integer.min(multiplierPower, multipliers.length - 1);
            }
            else if ((unit == null || unit.useMultiplierForUnit()))
            {
               for(i = multipliers.length - 1; i >= 0; i--)
               {
                  if ((d >= multipliers[i]) || (d <= -multipliers[i]))
                     break;
               }
            }
            else
            {
               multipliers = DECIMAL_MULTIPLIERS;
            }
            
            if (i >= 0)
            {
               if (stringOutput)
               {
                  NumberFormat nf = NumberFormat.getNumberInstance();
                  nf.setMaximumFractionDigits(2);
                  v.value = nf.format(d / multipliers[i]);
               }
               else
               {
                  v.value = Double.valueOf(d / multipliers[i]);
               }
               v.suffix = isSmallNumber ? SUFFIX_SMALL[i] : useBinaryMultipliers ? BINARY_SUFFIX[i] : SUFFIX[i];
            }
            else if (stringOutput)
            {
               v.value = value;
            }
            else
            {
               if (dataType == DataType.FLOAT)
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
            if (dataType == DataType.FLOAT)
               v.value = Double.parseDouble(value);
            else
               v.value = Long.parseLong(value);
         }
      }
      catch(NumberFormatException e)
      {
         v.value = value;
      }

      if (decimalOutput && (v.value instanceof Double))
         v.value = ((Double)v.value).longValue();

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
    * Get rounded value for chart labels
    *
    * @param value to round
    * @param step of label
    * @param maxPrecision desired precision
    * @return rounded value
    */
   public static String roundDecimalValue(double value, double step, int maxPrecision)
   {
      if (value == 0)
         return "0";

      double absValue = Math.abs(value);
      final double[] multipliers = DECIMAL_MULTIPLIERS;

      int i;
      for(i = multipliers.length - 1; i >= 0; i--)
      {
         if (absValue >= multipliers[i])
            break;
      }

      int precision;
      if ((step < 1) || (i < 0))
         precision = (calculatePrecision(step) > maxPrecision) ? maxPrecision : calculatePrecision(step);
      else
         precision = (calculatePrecision(step / multipliers[i]) > maxPrecision) ? maxPrecision : calculatePrecision(step / multipliers[i]);

      DecimalFormat df = new DecimalFormat();
      df.setMaximumFractionDigits(precision);
      return df.format((i < 0 ? value : (value / multipliers[i]))) + (i < 0 ? "" : SUFFIX[i]);
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
