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

import java.io.IOException;
import java.util.List;
import java.util.Map;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.TextOutputListener;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.objects.ObjectContext;
import org.netxms.ui.eclipse.objecttools.Messages;

/**
 * Action executor widget to run an action and display it's result 
 */
public class ActionExecutor extends AbstractObjectToolExecutor implements TextOutputListener
{
   private String executionString;
   private long alarmId;
   private Map<String, String> inputValues;
   private List<String> maskedFields;
   protected long nodeId;
   
   /**
    * Constructor for action execution
    * 
    * @param parent parent control
    * @param viewPart parent view
    * @param ctx execution context
    * @param actionSet action set
    * @param tool object tool to execute
    * @param inputValues input values provided by user
    * @param maskedFields list of the fields that should be masked
    */
   public ActionExecutor(Composite parent, ViewPart viewPart, ObjectContext ctx, ActionSet actionSet, ObjectTool tool,
         Map<String, String> inputValues, List<String> maskedFields)
   {
      super(parent, viewPart, ctx, actionSet);
      alarmId = ctx.getAlarmId();
      nodeId = ctx.object.getObjectId();
      this.executionString = tool.getData();
      this.inputValues = inputValues;
      this.maskedFields = maskedFields;
   }

   /**
    * @see org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor#executeInternal(org.eclipse.swt.widgets.Display)
    */
   @Override
   protected void executeInternal(Display display) throws Exception
   {
      session.executeActionWithExpansion(nodeId, alarmId, executionString, true, inputValues, maskedFields, ActionExecutor.this, null);
      out.write(Messages.get(display).LocalCommandResults_Terminated);
   }

   /**
    * @see org.netxms.client.ActionExecutionListener#messageReceived(java.lang.String)
    */
   @Override
   public void messageReceived(String text)
   {
      try
      {
         if (out != null)
            out.write(text);
      }
      catch(IOException e)
      {
      }
   }

   /**
    * @see org.netxms.client.TextOutputListener#setStreamId(long)
    */
   @Override
   public void setStreamId(long streamId)
   {      
   }

   /**
    * @see org.netxms.client.TextOutputListener#onError()
    */
   @Override
   public void onError()
   {
   }
}
