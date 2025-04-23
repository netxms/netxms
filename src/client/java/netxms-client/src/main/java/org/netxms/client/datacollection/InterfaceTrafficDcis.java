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

/**
 * Interface data 
 */
public class InterfaceTrafficDcis
{   
   private Long[] dciList;
   private String[] unitNames;

   /**
    * Interface data constructor
    */
   public InterfaceTrafficDcis(Long[] dciList, String[] unitNames)
   {
      this.dciList = dciList;
      this.unitNames = unitNames;
   }

   /**
    * Interface id's
    * 
    * @return
    */
   public Long[] getDciList()
   {
      return dciList;
   }

   /**
    * Interface id units
    * 
    * @return
    */
   public String[] getUnitNames()
   {
      return unitNames;
   }
   
}
