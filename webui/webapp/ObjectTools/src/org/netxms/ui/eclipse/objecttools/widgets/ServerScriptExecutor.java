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

/**
 * Server script executor and output provider widget
 */
public class ServerScriptExecutor extends AbstractObjectToolExecutor
{
   private String script = null;
   private Map<String, String> inputValues = null;
   private List<String> maskedFields;
   private long alarmId;
   private long nodeId;

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
   public ServerScriptExecutor(Composite resultArea, ViewPart viewPart, ObjectContext ctx, ActionSet actionSet, ObjectTool tool, Map<String, String> inputValues, List<String> maskedFields)
   {
      super(resultArea, viewPart, ctx, actionSet);
      script = tool.getData();
      alarmId = ctx.alarm != null ? ctx.alarm.getId() : 0;
      nodeId = ctx.object.getObjectId();
      this.inputValues = inputValues;
      this.maskedFields = maskedFields;
   }

   /**
    * @see org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor#executeInternal(org.eclipse.swt.widgets.Display)
    */
   @Override
   protected void executeInternal(Display display) throws Exception
   {
      session.executeLibraryScript(nodeId, alarmId, script, inputValues, maskedFields, getOutputListener());
   }
}
