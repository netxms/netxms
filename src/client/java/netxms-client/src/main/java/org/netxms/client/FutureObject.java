/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Raden Solutions
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

import org.netxms.client.objects.AbstractObject;

/**
 * Class that to wait for object 
 */
class FutureObject
{
   private AbstractObject object;
   
   /**
    * Object constructor
    */
   public FutureObject()
   {
      object = null;
   }
   
   /**
    * Object constructor
    * 
    * @param object object inherited form AbstractObject
    */
   public FutureObject(AbstractObject object)
   {
      this.object = object;
   }
   
   /**
    * If object is set
    * 
    * @return true if object is set
    */
   public boolean hasObject()
   {
      return object != null;
   }
   
   /**
    * Set object
    * 
    * @param object object inherited form AbstractObject
    */
   public void setObject(AbstractObject object)
   {
      this.object = object;
   }
   
   /**
    * Get object
    * 
    * @return object inherited form AbstractObject
    */
   public AbstractObject getObject()
   {
      return object;
   }
}
