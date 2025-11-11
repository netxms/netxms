/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Data formatter
 */
public class DataFormatter
{
   private static final Logger logger = LoggerFactory.getLogger(DataFormatter.class);

   private String formatString;
   private DataType dataType;
   private MeasurementUnit unit;
   private int multiplierPower;
   private int useMultipliers; //  DciValue.MULTIPLIERS_DEFAULT, DciValue.MULTIPLIERS_ON, DciValue.MULTIPLIERS_OFF
   private boolean useMultipliersByDefault = true;
   private String defaultFormatStringWithMultipliers = "%{m,u}s";
   private String defaultFormatStringWithoutMultipliers = "%{u}s";   


   /**
    * Create new data formatter
    */
   public DataFormatter()
   {
      this.formatString = null;
      this.dataType = DataType.STRING;
      this.unit = null;
      multiplierPower = 0;
      useMultipliers = DciValue.MULTIPLIERS_DEFAULT;
   }
   
   
   /**
    * Create new data formatter 
    * 
    * @param series data series
    */
   public DataFormatter(DataSeries series)
   {
      this.formatString = null;
      this.dataType = series.getDataType();
      this.unit = series.getMeasurementUnit();
      this.multiplierPower = series.getMultiplierPower();
      this.useMultipliers = series.getUseMultiplier();
   }

   /**
    * @param dciValue
    */
   public DataFormatter(DciValue dciValue)
   {
      this.formatString = null;
      this.dataType = dciValue.getDataType();
      this.unit = dciValue.getMeasurementUnit();
      this.multiplierPower = dciValue.getMultiplier();
      this.useMultipliers = dciValue.getMultipliersSelection();
   }


   /**
    * Set format string
    * 
    * @param formatString the formatString to set
    * @return this
    */
   public DataFormatter setFormatString(String formatString)
   {
      this.formatString = formatString;
      return this;
   }
   
   /**
    * Set data type
    * 
    * @param dataType the dataType to set
    * @return this
    */
   public DataFormatter setDataType(DataType dataType)
   {
      this.dataType = dataType;
      return this;
   }
   
   /**
    * Set measurement unit
    * 
    * @param unit the unit to set
    * @return this
    */
   public DataFormatter setMeasurementUnit(MeasurementUnit unit)
   {
      this.unit = unit;
      return this;
   }
   
   /**
    * Set multiplier power
    * 
    * @param multiplierPower the multiplierPower to set
    * @return this
    */
   public DataFormatter setMultiplierPower(int multiplierPower)
   {
      this.multiplierPower = multiplierPower;
      return this;
   }
   
   /**
    * Set multiplier usage flag
    * 
    * @param useMultipliers the useMultipliers to set
    * @return this
    */
   public DataFormatter setUseMultipliers(int useMultipliers)
   {
      this.useMultipliers = useMultipliers;
      return this;
   }

   /**
    * Set default for multipliers
    * 
    * @param useMultipliersByDefault the useMultipliersByDefault to set
    * @return this
    */
   public DataFormatter setDefaultForMultipliers(boolean useMultipliersByDefault)
   {
      this.useMultipliersByDefault = useMultipliersByDefault;
      return this;
   }

   /**
    * Set default format strings
    * 
    * @param withoutMultipliers default format string without multipliers
    * @param withMultipliers default format string with multipliers
    * @return this
    */
   public DataFormatter setDefaultFormatString(String withoutMultipliers, String withMultipliers)
   {
      if (withoutMultipliers != null)
         this.defaultFormatStringWithoutMultipliers = withoutMultipliers;

      if (withMultipliers != null)
         this.defaultFormatStringWithMultipliers = withMultipliers;
      return this;
   }

   /**
    * Format value
    *
    * @param value The value
    * @param timeFormatter Date and time formatter
    * @return The formatted value
    */
   public String format(String value, TimeFormatter timeFormatter)
   {
      String str = formatString;
      if (str == null || str.isEmpty())
      {
         str = ((useMultipliers == DciValue.MULTIPLIERS_DEFAULT) && useMultipliersByDefault) || 
               (useMultipliers == DciValue.MULTIPLIERS_YES) ? defaultFormatStringWithMultipliers : defaultFormatStringWithoutMultipliers;
      }
      
      if (value == null || value.isEmpty())
         return "";

      StringBuilder sb = new StringBuilder();
      char[] format = str.toCharArray();

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
                  sb.append(timeFormatter.formatUptime((long)Double.parseDouble(value)));
               }
               else if (useUnits && unit.getName().equals("Epoch time"))
               {
                  sb.append(timeFormatter.formatDateAndTime((long)Double.parseDouble(value)));
               }
               else
               {
                  try
                  {
                     Value v = getValueForFormat(value, useMultipliers, format[j]);
                     sb.append(String.format(f, v.value));
                     sb.append(v.suffix);
                     if (useUnits)
                     {
                        String unitName = unit.getName();
                        if (v.suffix.isEmpty() && !unitName.isEmpty())
                        {
                           sb.append("\u2009");
                        }
                        sb.append(unitName);
                     }
                  }
                  catch(IndexOutOfBoundsException | IllegalFormatException | NumberFormatException e) // out of bound may occur if there is no letter after % sign. Like: %*3
                  {
                     logger.debug("Error formatting value \"" + value + "\"", e);
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
   private static final String[] SUFFIX = { "", "\u2009k", "\u2009M", "\u2009G", "\u2009T", "\u2009P" };
   private static final String[] BINARY_SUFFIX = { "", "\u2009Ki", "\u2009Mi", "\u2009Gi", "\u2009Ti", "\u2009Pi" };
   private static final String[] SUFFIX_SMALL = { "\u2009f", "\u2009p", "\u2009n", "\u2009\u03bc", "\u2009m", "" };

   private Value getValueWithMultipliers(String value)
   {
      double d;
      try
      {
         d = Double.parseDouble(value);
      }
      catch(NumberFormatException e)
      {
         return new Value(value);
      }

      boolean useBinaryMultipliers = (unit != null) && unit.isBinary();
      int multiplierPower = (unit != null) ? this.multiplierPower : 0; 
      boolean isSmallNumber = ((d > -0.01) && (d < 0.01) && (d != 0) && (multiplierPower <= 0) && (unit != null) && unit.useMultiplierForUnit()) || (multiplierPower < 0);
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
      
      Value v = new Value();
      if (i >= 0)
      {
         v.value = Double.valueOf(d / multipliers[i]);
         v.suffix = isSmallNumber ? SUFFIX_SMALL[i] : useBinaryMultipliers ? BINARY_SUFFIX[i] : SUFFIX[i];
      }
      else
      {
         v.value = value;
      }
      return v;
   }

   /**
    * Get value ready for formatter
    *
    * @param useMultipliers
    * @return formatted value
    */
   private Value getValueForFormat(String value, boolean useMultipliers, char formatSymbol) throws NumberFormatException
   {
      if ((dataType == DataType.STRING) || (dataType == DataType.NULL))
         return new Value(value);

      Value v = useMultipliers ? getValueWithMultipliers(value) : new Value(value);
      switch(formatSymbol)
      {
         case 's':
         case 'S':
            if (v.value instanceof Double)
            {
               NumberFormat nf = NumberFormat.getNumberInstance();
               nf.setMaximumFractionDigits(2);
               v.value = nf.format((Double)v.value);
            }
            break;
         case 'a':
         case 'A':
         case 'e':
         case 'E':
         case 'f':
         case 'g':
         case 'G':
            if (v.value instanceof String)
            {
               v.value = Double.parseDouble((String)v.value);
            }
            break;
         case 'd':
         case 'o':
         case 'x':
         case 'X':
            if (v.value instanceof String)
            {
               try
               {
                  v.value = Long.parseLong((String)v.value);
               }
               catch(NumberFormatException e)
               {
                  v.value = Double.valueOf(Double.parseDouble((String)v.value)).longValue();
               }
            }
            else if (v.value instanceof Double)
            {
               v.value = ((Double)v.value).longValue();
            }
            break;
         default:
            break;
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

      Value()
      {
      }
      
      Value(Object value)
      {
         this.value = value;
      }
   }
}
