/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection;

import org.netxms.client.constants.DataOrigin;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.TimeFormatter;
import org.netxms.client.snmp.MibObject;
import org.netxms.nxmc.modules.snmp.shared.MibCache;

/**
 * Helper for formatting DCI values for display. Wraps {@link DciValue#getFormattedValue} with optional mapping table
 * substitution (delegated to the per-session {@link MappingTableCache}) and MIB enum label substitution for SNMP DCIs
 * that have the {@link DataCollectionItem#DCF_USE_SNMP_MIB_ENUM} flag set.
 */
public final class DciValueFormatter
{
   private DciValueFormatter()
   {
   }

   /**
    * Format DCI value for display, applying mapping table lookup and MIB enum label substitution when applicable.
    *
    * @param value DCI value to format
    * @param useMultipliers whether to apply unit multipliers
    * @param timeFormatter time formatter for duration/timestamp data types
    * @return formatted string. When a mapping table hit occurs, the mapped value is returned as-is.
    */
   public static String format(DciValue value, boolean useMultipliers, TimeFormatter timeFormatter)
   {
      String mapped = lookup(value.getMappingTableId(), value.getValue());
      if (mapped != null)
         return mapped;

      String formatted = value.getFormattedValue(useMultipliers, timeFormatter);
      return applyEnumSubstitution(value, formatted);
   }

   /**
    * Format DCI value for display using a format string, applying mapping table lookup and MIB enum label
    * substitution when applicable.
    *
    * @param value DCI value to format
    * @param formatString format string
    * @param timeFormatter time formatter for duration/timestamp data types
    * @return formatted string. When a mapping table hit occurs, the mapped value is returned as-is.
    */
   public static String format(DciValue value, String formatString, TimeFormatter timeFormatter)
   {
      String mapped = lookup(value.getMappingTableId(), value.getValue());
      if (mapped != null)
         return mapped;

      Object formatted = value.getFormattedValue(formatString, timeFormatter);
      return applyEnumSubstitution(value, (formatted != null) ? formatted.toString() : null);
   }

   /**
    * Resolve a raw value through the mapping table with the given ID. Shared entry point for any view that needs to
    * render mapped DCI values (last-values lists, gauges, charts, etc.). Returns the mapped string, or {@code null}
    * when there is no mapping configured ({@code id == 0}), the raw value is empty, the table is not yet cached
    * (an async load is scheduled), or the lookup misses. Callers should fall back to their default formatting on null.
    *
    * @param id mapping table ID; 0 means no mapping
    * @param rawValue raw collected value as string
    * @return mapped string, or {@code null} when no mapping applies
    */
   public static String lookup(int id, String rawValue)
   {
      MappingTableCache cache = MappingTableCache.getInstance();
      return (cache != null) ? cache.lookup(id, rawValue) : null;
   }

   /**
    * Apply enum label substitution to an already-formatted value string.
    * Returns the input unchanged if the flag is off, source is not SNMP, the raw value
    * is not an integer, or the OID does not have a matching enum label.
    */
   private static String applyEnumSubstitution(DciValue value, String formatted)
   {
      if (formatted == null)
         return null;

      if ((value.getSource() != DataOrigin.SNMP) || ((value.getFlags() & DataCollectionItem.DCF_USE_SNMP_MIB_ENUM) == 0))
         return formatted;

      String raw = value.getValue();
      if ((raw == null) || raw.isEmpty())
         return formatted;

      int numeric;
      try
      {
         numeric = Integer.parseInt(raw.trim());
      }
      catch(NumberFormatException e)
      {
         return formatted;
      }

      MibObject mibObject = MibCache.findObject(value.getName(), false);
      if (mibObject == null)
         return formatted;

      String label = mibObject.resolveEnumValue(numeric);
      if (label == null)
         return formatted;

      return label + " (" + formatted + ")";
   }
}
