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
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.xnap.commons.i18n.I18n;

/**
 * Action executor widget to run an action and display it's result
 */
public class SSHExecutor extends AbstractObjectToolExecutor
{
   private I18n i18n = LocalizationHelper.getI18n(SSHExecutor.class);

   /**
    * Constructor for action execution
    * 
    * @param parent parent control
    * @param viewPart parent view
    * @param ctx execution context
    * @param actionSet action set
    * @param tool object tool to execute
    */
   public SSHExecutor(Composite parent,  ObjectContext ctx, ActionSet actionSet, final CommonContext objectToolInfo)
   {
      super(parent, ctx, actionSet, objectToolInfo);
   }

   /**
    * @see org.netxms.nxmc.modules.objecttools.widgets.AbstractObjectToolExecutor#executeInternal(org.eclipse.swt.widgets.Display)
    */
   @Override
   protected void executeInternal(Display display) throws Exception
   {
      session.executeSshCommand(objectContext.getObjectId(), objectContext.getAlarmId(), objectToolInfo.tool.getData(), objectToolInfo.inputValues, objectToolInfo.maskedFields, true, getOutputListener(), null);
      writeOutput(i18n.tr("\n\n*** TERMINATED ***\n\n\n"));
   }
}

