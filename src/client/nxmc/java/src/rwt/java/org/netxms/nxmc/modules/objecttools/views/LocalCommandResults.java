/**
 * NetXMS - open source network management system
 * Copyright (C) 2022 Raden Soultions
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
package org.netxms.nxmc.modules.objecttools.views;

import java.util.List;
import java.util.Map;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.modules.objects.ObjectContext;

/**
 * Empty view placeholder for code compatibility with desktop version
 */
public class LocalCommandResults extends AbstractCommandResultView
{
   public LocalCommandResults(ObjectContext node, ObjectTool tool, final Map<String, String> inputValues, final List<String> maskedFields)
   {
      super(node, tool, inputValues, maskedFields);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.AbstractCommandResultView#execute()
    */
   @Override
   protected void execute()
   {
   }
}
