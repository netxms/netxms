/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2022 Raden Soultions
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
 * Server script executor and output provider widget
 */
public class ServerScriptExecutor extends AbstractObjectToolExecutor
{

   /**
    * Constructor
    * 
    * @param resultArea parent composite for result
    * @param viewPart parent view
    * @param ctx execution context
    * @param actionSet action set
    * @param tool object tool to execute
    * @param inputValues input values for execution
    */
   public ServerScriptExecutor(Composite resultArea, final ObjectContext ctx, final ActionSet actionSet, final CommonContext objectToolInfo)
   {
      super(resultArea, ctx, actionSet, objectToolInfo);
   }

   /**
    * @see org.netxms.nxmc.modules.objecttools.widgets.AbstractObjectToolExecutor#executeInternal(org.eclipse.swt.widgets.Display)
    */
   @Override
   protected void executeInternal(Display display) throws Exception
   {
      session.executeLibraryScript(objectContext.getObjectId(), objectContext.getAlarmId(), objectToolInfo.tool.getData(), objectToolInfo.inputValues, objectToolInfo.maskedFields, getOutputListener());
   }
}
