/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2024 Raden Soultions
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
package org.netxms.nxmc.modules.objecttools.widgets;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.netxms.nxmc.modules.objects.ObjectContext;

/**
 * Empty command executor for code compatibility with desktop version.
 */
public class LocalCommandExecutor extends AbstractObjectToolExecutor
{
   public LocalCommandExecutor(Composite resultArea, ObjectContext objectContext, ActionSet actionSet, CommonContext objectToolInfo)
   {
      super(resultArea, objectContext, actionSet, objectToolInfo);
   }

   /**
    * @see org.netxms.nxmc.modules.objecttools.widgets.AbstractObjectToolExecutor#executeInternal(org.eclipse.swt.widgets.Display)
    */
   @Override
   protected void executeInternal(Display display) throws Exception
   {
   }
}
