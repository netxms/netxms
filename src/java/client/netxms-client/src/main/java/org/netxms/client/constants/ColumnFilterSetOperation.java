package org.netxms.client.constants;

import java.util.HashMap;
import java.util.Map;
import org.netxms.base.Logger;

public enum ColumnFilterSetOperation
{
   AND(0),
   OR(1),
   UNKNOWN(2);
   
   private int value;
   private static Map<Integer, ColumnFilterSetOperation> lookupTable = new HashMap<Integer, ColumnFilterSetOperation>();

   static
   {
      for(ColumnFilterSetOperation element : ColumnFilterSetOperation.values())
      {
         lookupTable.put(element.value, element);
      }
   }
   
   private ColumnFilterSetOperation(int value)
   {
      this.value = value;
   }
   
   public int getValue()
   {
      return value;
   }
   
   public static ColumnFilterSetOperation getByValue(int value)
   {
      final ColumnFilterSetOperation element = lookupTable.get(value);
      if (element == null)
      {
         Logger.warning(ColumnFilterSetOperation.class.getName(), "Unknown element " + value);
         return UNKNOWN; //fallback
      }
      return element;
   }
}
