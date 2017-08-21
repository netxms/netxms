/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2017 RadenSolutions
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
package org.netxms.ui.eclipse.objecttools;

import org.eclipse.core.expressions.PropertyTester;
import org.netxms.client.objecttools.ObjectTool;

/**
 * Property tester for ObjectTool
 */
public class ObjectToolsPopertyTester extends PropertyTester
{
   @Override
   public boolean test(Object receiver, String property, Object[] args, Object expectedValue)
   {
      if (!(receiver instanceof ObjectTool) || !property.equals("hasObjectToolsType"))
         return false;

      ObjectTool objectTool = (ObjectTool)receiver;
      switch(objectTool.getToolType())
      {
         case ObjectTool.TYPE_TABLE_AGENT:
         case ObjectTool.TYPE_TABLE_SNMP:
            return true;
         default:
            return false;
      }
   }

}
