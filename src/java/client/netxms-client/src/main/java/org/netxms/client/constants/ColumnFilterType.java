package org.netxms.client.constants;

import java.util.HashMap;
import java.util.Map;
import org.netxms.base.Logger;

public enum ColumnFilterType
{
   EQUALS(0),
   RANGE(1),
   SET(2),
   LIKE(3),
   LESS(4),
   GREATER(5),
   CHILDOF(6),
   UNKNOWN(7);
   
   private int value;
   private static Map<Integer, ColumnFilterType> lookupTable = new HashMap<Integer, ColumnFilterType>();
   
   static
   {
      for(ColumnFilterType element : ColumnFilterType.values())
      {
         lookupTable.put(element.value, element);
      }
   }
   
   private ColumnFilterType(int value)
   {
      this.value = value;
   }
   
   public int getValue()
   {
      return value;
   }
   
   public static ColumnFilterType getByValue(int value)
   {
      final ColumnFilterType element = lookupTable.get(value);
      if (element == null)
      {
         Logger.warning(ColumnFilterType.class.getName(), "Unknown element " + value);
         return UNKNOWN; // fallback
      }
      return element;
   }
}
