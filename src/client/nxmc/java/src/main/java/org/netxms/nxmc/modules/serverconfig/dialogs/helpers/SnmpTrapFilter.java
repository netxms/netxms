/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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
package org.netxms.nxmc.modules.serverconfig.dialogs.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.NXCSession;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.nxmc.Registry;



/**
 * Filter for Snmp Traps
 */
public class SnmpTrapFilter extends ViewerFilter
{
   private String filterString = null;

   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if (filterString == null || (filterString.isEmpty()))
         return true;
      if (objectIdMatch(element))
         return true;
      else if (eventMatch(element))
         return true;
      else if (descriptionMatch(element))
         return true;
      return false;
   }
   
   /**
    * Checks for a match in description
    * @param element
    * @return
    */
   public boolean descriptionMatch(Object element)
   {
      if(((SnmpTrap)element).getDescription().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   /**
    * Checks for a match in Event
    * @param element
    * @return
    */
   public boolean eventMatch(Object element)
   {
      NXCSession session = Registry.getSession();
      if(session.findEventTemplateByCode(((SnmpTrap)element).getEventCode()).toString().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   /**
    * Checks for a match in ObjectID
    * @param element
    * @return
    */
   public boolean objectIdMatch(Object element)
   {
      if (((SnmpTrap)element).getObjectId().toString().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   /**
    * Set filter string
    */
   public void setFilterString(final String filterString)
   {
      this.filterString = filterString.toLowerCase();
   }
}