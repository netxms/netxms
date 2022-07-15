package org.netxms.client.datacollection;

import org.netxms.base.NXCPMessage;

public class DciInfo
{
   private boolean binaryUnit;
   private String unitName;
   private int multipierPower;
   
   public DciInfo(NXCPMessage msg, long base)
   {
      unitName = msg.getFieldAsString(base++);
      multipierPower = msg.getFieldAsInt32(base++);
      binaryUnit = unitName.contains(" (IEC)");
   }   
   
   /**
    * @return the binaryUnit
    */
   public boolean isBinaryUnit()
   {
      return binaryUnit;
   }
   /**
    * @return the unitName
    */
   public String getUnitName()
   {
      return unitName;
   }
   /**
    * @return the multipierPower
    */
   public int getMultipierPower()
   {
      return multipierPower;
   }

}
