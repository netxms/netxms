/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.perfview;

import org.eclipse.core.expressions.PropertyTester;
import org.netxms.client.datacollection.GraphSettings;

/**
 * Property tester for server components
 */
public class GraphTemplatePropertyTester extends PropertyTester
{
   /* (non-Javadoc)
    * @see org.eclipse.core.expressions.IPropertyTester#test(java.lang.Object, java.lang.String, java.lang.Object[], java.lang.Object)
    */
   @Override
   public boolean test(Object receiver, String property, Object[] args, Object expectedValue)
   {
      if (!(receiver instanceof GraphSettings))
         return false;
      
      if (property.equals("isTemplateGraph")) //$NON-NLS-1$
      {
         return ((GraphSettings)receiver).isTemplate();
      }
      
      return false;
   }
}
