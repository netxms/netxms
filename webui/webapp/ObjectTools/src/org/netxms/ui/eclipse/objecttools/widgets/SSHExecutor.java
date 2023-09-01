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
package org.netxms.ui.eclipse.objecttools.widgets;

import java.util.List;
import java.util.Map;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.objects.ObjectContext;
import org.netxms.ui.eclipse.objecttools.Messages;

/**
 * Action executor widget to run an action and display it's result
 */
public class SSHExecutor extends AbstractObjectToolExecutor
{
   private String executionString;
   protected long nodeId;
   private List<String> maskedFields;
   private Map<String, String> lastInputValues = null;

   /**
    * Constructor for action execution
    * 
    * @param parent parent control
    * @param viewPart parent view
    * @param ctx execution context
    * @param actionSet action set
    * @param tool object tool to execute
    */
   public SSHExecutor(Composite parent, ViewPart viewPart, ObjectContext ctx, ActionSet actionSet, ObjectTool tool, Map<String, String> inputValues, List<String> maskedFields)
   {
      super(parent, viewPart, ctx, actionSet);
      nodeId = ctx.object.getObjectId();
      this.executionString = tool.getData();
      lastInputValues = inputValues;
      this.maskedFields = maskedFields;
   }

   /**
    * @see org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor#executeInternal(org.eclipse.swt.widgets.Display)
    */
   @Override
   protected void executeInternal(Display display) throws Exception
   {
      session.executeSshCommand(nodeId, objectContext.getAlarmId(), executionString, lastInputValues, maskedFields, true, getOutputListener(), null);
      out.write(Messages.get().LocalCommandResults_Terminated);
   }
}
