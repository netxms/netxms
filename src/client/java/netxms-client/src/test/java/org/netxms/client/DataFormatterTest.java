/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.client;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrowsExactly;
import java.util.Locale;
import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.MeasurementUnit;
import org.netxms.client.datacollection.TimeFormatter;

/**
 * Tests for number parsing and formatting with multiplier suffixes in DataFormatter class
 */
public class DataFormatterTest
{
   /**
    * Thin space used by data formatter as separator between value, multiplier suffix, and unit name
    */
   private static final String THIN_SPACE = "\u2009";

   private static final TimeFormatter TIME_FORMATTER = new TimeFormatter() {
      @Override
      public String formatUptime(long seconds)
      {
         return Long.toString(seconds);
      }

      @Override
      public String formatDateAndTime(long timestamp)
      {
         return Long.toString(timestamp);
      }
   };

   private static Locale savedLocale;

   @BeforeAll
   public static void setLocale()
   {
      // Value formatting is locale dependent
      savedLocale = Locale.getDefault();
      Locale.setDefault(Locale.US);
   }

   @AfterAll
   public static void restoreLocale()
   {
      Locale.setDefault(savedLocale);
   }

   @Test
   public void testParseNumberWithoutSuffix()
   {
      assertEquals(100, DataFormatter.parseNumberWithSuffix("100"));
      assertEquals(-42.5, DataFormatter.parseNumberWithSuffix("-42.5"));
      assertEquals(1500, DataFormatter.parseNumberWithSuffix("  1500  "));
   }

   @Test
   public void testParseNumberWithDecimalSuffix()
   {
      assertEquals(1000, DataFormatter.parseNumberWithSuffix("1K"));
      assertEquals(1000000, DataFormatter.parseNumberWithSuffix("1M"));
      assertEquals(1000000000.0, DataFormatter.parseNumberWithSuffix("1G"));
      assertEquals(1000000000000.0, DataFormatter.parseNumberWithSuffix("1T"));
      assertEquals(1000000000000000.0, DataFormatter.parseNumberWithSuffix("1P"));

      assertEquals(2500000, DataFormatter.parseNumberWithSuffix("2.5M"));
      assertEquals(-3000, DataFormatter.parseNumberWithSuffix("-3K"));

      // suffix is case insensitive and can be separated from number by spaces
      assertEquals(1000, DataFormatter.parseNumberWithSuffix("1k"));
      assertEquals(1000, DataFormatter.parseNumberWithSuffix("1 K"));
   }

   @Test
   public void testParseNumberWithBinarySuffix()
   {
      assertEquals(1024, DataFormatter.parseNumberWithSuffix("1Ki"));
      assertEquals(1048576, DataFormatter.parseNumberWithSuffix("1Mi"));
      assertEquals(1073741824, DataFormatter.parseNumberWithSuffix("1Gi"));
      assertEquals(1099511627776.0, DataFormatter.parseNumberWithSuffix("1Ti"));
      assertEquals(1125899906842624.0, DataFormatter.parseNumberWithSuffix("1Pi"));

      assertEquals(2621440, DataFormatter.parseNumberWithSuffix("2.5Mi"));

      assertEquals(1024, DataFormatter.parseNumberWithSuffix("1ki"));
      assertEquals(1024, DataFormatter.parseNumberWithSuffix("1KI"));
   }

   @Test
   public void testParseInvalidNumber()
   {
      assertThrowsExactly(NumberFormatException.class, () -> DataFormatter.parseNumberWithSuffix(null));
      assertThrowsExactly(NumberFormatException.class, () -> DataFormatter.parseNumberWithSuffix(""));
      assertThrowsExactly(NumberFormatException.class, () -> DataFormatter.parseNumberWithSuffix("   "));
      assertThrowsExactly(NumberFormatException.class, () -> DataFormatter.parseNumberWithSuffix("abc"));
      assertThrowsExactly(NumberFormatException.class, () -> DataFormatter.parseNumberWithSuffix("K"));
      assertThrowsExactly(NumberFormatException.class, () -> DataFormatter.parseNumberWithSuffix("Ki"));
      assertThrowsExactly(NumberFormatException.class, () -> DataFormatter.parseNumberWithSuffix("i"));

      // "i" is only valid as part of binary multiplier suffix
      assertThrowsExactly(NumberFormatException.class, () -> DataFormatter.parseNumberWithSuffix("5i"));
   }

   @Test
   public void testFormatNumberWithSuffix()
   {
      // values below smallest multiplier are formatted as is
      assertEquals("0", DataFormatter.formatNumberWithSuffix(0));
      assertEquals("123", DataFormatter.formatNumberWithSuffix(123));
      assertEquals("999", DataFormatter.formatNumberWithSuffix(999));
      assertEquals("1.5", DataFormatter.formatNumberWithSuffix(1.5));

      // exact decimal multipliers are preferred
      assertEquals("1K", DataFormatter.formatNumberWithSuffix(1000));
      assertEquals("1G", DataFormatter.formatNumberWithSuffix(1000000000.0));
      assertEquals("1P", DataFormatter.formatNumberWithSuffix(1000000000000000.0));
      assertEquals("-3K", DataFormatter.formatNumberWithSuffix(-3000));

      // binary multipliers are used when value is not multiple of decimal one
      assertEquals("1Ki", DataFormatter.formatNumberWithSuffix(1024));
      assertEquals("1Mi", DataFormatter.formatNumberWithSuffix(1048576));
      assertEquals("3Gi", DataFormatter.formatNumberWithSuffix(3221225472.0));

      // fractional decimal form is used if it can be read back without precision loss
      assertEquals("1.5K", DataFormatter.formatNumberWithSuffix(1500));
      assertEquals("2.5M", DataFormatter.formatNumberWithSuffix(2500000));
   }

   @Test
   public void testFormatParseRoundTrip()
   {
      double[] values = {
         0, 1, -1, 999, 1000, 1023, 1024, 1500, 1234567, 2500000, 1048576, 3221225472.0,
         1000000000.0, 1000000000000.0, 1000000000000000.0, 1000000000000000.0 + 1, 12345678901234567.0,
         -3000, 1.5, 0.001, Double.MIN_VALUE, Double.MAX_VALUE,
         Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY
      };
      for(double v : values)
      {
         String text = DataFormatter.formatNumberWithSuffix(v);
         assertEquals(v, DataFormatter.parseNumberWithSuffix(text), "round trip failed for " + v + " formatted as \"" + text + "\"");
      }
   }

   /**
    * Format value as it will be displayed for DCI with given data type and measurement unit and default format string.
    */
   private static String format(String value, DataType dataType, String unit, int useMultipliers)
   {
      DataFormatter formatter = new DataFormatter().setDataType(dataType).setUseMultipliers(useMultipliers);
      if (unit != null)
         formatter.setMeasurementUnit(new MeasurementUnit(unit));
      return formatter.format(value, TIME_FORMATTER);
   }

   @Test
   public void testFormatFloatValuesBelowOne()
   {
      // Values between -1 and 1 have no applicable multiplier and should be rounded instead of being displayed as raw six decimal places
      assertEquals("0.68" + THIN_SPACE + "A", format("0.680000", DataType.FLOAT, "A", DciValue.MULTIPLIERS_DEFAULT));
      assertEquals("-0.68" + THIN_SPACE + "A", format("-0.680000", DataType.FLOAT, "A", DciValue.MULTIPLIERS_DEFAULT));
      assertEquals("0.68", format("0.680000", DataType.FLOAT, null, DciValue.MULTIPLIERS_DEFAULT));
      assertEquals("0" + THIN_SPACE + "A", format("0.000000", DataType.FLOAT, "A", DciValue.MULTIPLIERS_DEFAULT));

      // Values above 1 are not affected
      assertEquals("3.03" + THIN_SPACE + "A", format("3.030000", DataType.FLOAT, "A", DciValue.MULTIPLIERS_DEFAULT));

      // Small values still use small multipliers if unit is set
      assertEquals("5" + THIN_SPACE + "mA", format("0.005000", DataType.FLOAT, "A", DciValue.MULTIPLIERS_DEFAULT));

      // Without unit small multipliers are not applicable, so significant digits should be preserved
      assertEquals("0.005", format("0.005000", DataType.FLOAT, null, DciValue.MULTIPLIERS_DEFAULT));
      assertEquals("0.00012", format("0.000123", DataType.FLOAT, null, DciValue.MULTIPLIERS_DEFAULT));

      // Units that do not use multipliers at all
      assertEquals("0.68" + THIN_SPACE + "%", format("0.680000", DataType.FLOAT, "%", DciValue.MULTIPLIERS_DEFAULT));
   }

   @Test
   public void testFormatFloatValuesWithoutMultipliers()
   {
      assertEquals("3.03" + THIN_SPACE + "A", format("3.030000", DataType.FLOAT, "A", DciValue.MULTIPLIERS_NO));
      assertEquals("0.68" + THIN_SPACE + "A", format("0.680000", DataType.FLOAT, "A", DciValue.MULTIPLIERS_NO));
      assertEquals("0.005" + THIN_SPACE + "A", format("0.005000", DataType.FLOAT, "A", DciValue.MULTIPLIERS_NO));
      assertEquals("1,234.57" + THIN_SPACE + "A", format("1234.567000", DataType.FLOAT, "A", DciValue.MULTIPLIERS_NO));

      // Integer values are displayed as is
      assertEquals("1234567" + THIN_SPACE + "A", format("1234567", DataType.INT32, "A", DciValue.MULTIPLIERS_NO));
   }

   @Test
   public void testFormatIntegerValues()
   {
      assertEquals("1.23" + THIN_SPACE + "MA", format("1234567", DataType.INT32, "A", DciValue.MULTIPLIERS_DEFAULT));
      assertEquals("999" + THIN_SPACE + "A", format("999", DataType.INT32, "A", DciValue.MULTIPLIERS_DEFAULT));
      assertEquals("0" + THIN_SPACE + "A", format("0", DataType.INT32, "A", DciValue.MULTIPLIERS_DEFAULT));
   }
}
