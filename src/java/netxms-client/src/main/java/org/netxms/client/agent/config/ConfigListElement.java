/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.client.agent.config;

import org.netxms.base.NXCPMessage;


public class ConfigListElement implements Comparable
{
   private long id;
   private String name;
   private long sequenceNumber;
   
   /**
    * Create from message 
    * 
    * @param name
    */
   public ConfigListElement(long base,  NXCPMessage response)
   {
      this.id = response.getVariableAsInt64(base);
      this.name = response.getVariableAsString(base+1);
      this.sequenceNumber = response.getVariableAsInt64(base+2);
   }
   
   public ConfigListElement()
   {
      id = 0;
      name = "New config";
      sequenceNumber = -1;
   }
   
   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }
   
   /**
    * @param id the id to set
    */
   public void setId(long id)
   {
      this.id = id;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }
   
   @Override
   public String toString()
   {
      return name;
   }

   /**
    * @return the sequenceNumber
    */
   public long getSequenceNumber()
   {
      return sequenceNumber;
   }

   /**
    * @param sequenceNumber the sequenceNumber to set
    */
   public void setSequenceNumber(long sequenceNumber)
   {
      this.sequenceNumber = sequenceNumber;
   } 

   @Override
   public int compareTo(Object arg0)
   {
      long otherValue = ((ConfigListElement)arg0).getSequenceNumber();
      int result = new Long(sequenceNumber).compareTo(new Long(otherValue));
      return result;
   }
   
}
