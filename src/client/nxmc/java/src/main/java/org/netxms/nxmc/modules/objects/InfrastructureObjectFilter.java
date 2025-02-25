/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Soultions
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
package org.netxms.nxmc.modules.objects;

import java.util.Set;
import org.netxms.client.ObjectFilter;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.modules.objects.views.ObjectBrowser;

/**
 * Object filter for Infrastructure object tree
 */
public class InfrastructureObjectFilter implements ObjectFilter
{

   private static final Set<Integer> classFilterInfrastructure = ObjectBrowser.calculateClassFilter(SubtreeType.INFRASTRUCTURE);
   private static final Set<Integer> classFilterNetwork = ObjectBrowser.calculateClassFilter(SubtreeType.NETWORK);
   static
   {
      classFilterInfrastructure.remove(AbstractObject.OBJECT_SUBNET);
   }
   
   /**
    * @see org.netxms.client.ObjectFilter#accept(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean accept(AbstractObject o)
   {
      if (!o.hasParents() || (o.getObjectClass() == AbstractObject.OBJECT_CONTAINER) || (o.getObjectClass() == AbstractObject.OBJECT_COLLECTOR) ||
            (o.getObjectClass() == AbstractObject.OBJECT_CIRCUIT) || (o.getObjectClass() == AbstractObject.OBJECT_WIRELESSDOMAIN) || 
            (o.getObjectClass() == AbstractObject.OBJECT_CONDITION))
         return true;
      if (o.getObjectClass() == AbstractObject.OBJECT_INTERFACE || o.getObjectClass() == AbstractObject.OBJECT_VPNCONNECTOR || o.getObjectClass() == AbstractObject.OBJECT_NETWORKSERVICE)
         return o.hasAccessibleViewParents(classFilterInfrastructure, this);
      if (o.getObjectClass() == AbstractObject.OBJECT_SUBNET)
         return o.hasAccessibleParents(classFilterInfrastructure);
      return o.hasAccessibleParents(classFilterInfrastructure) || !o.hasAccessibleParents(classFilterNetwork);
   }      
}
