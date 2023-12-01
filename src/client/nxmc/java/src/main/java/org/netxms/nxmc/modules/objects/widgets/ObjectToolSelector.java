/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.modules.objecttools.ObjectToolsCache;
import org.netxms.nxmc.modules.objecttools.dialogs.ObjectToolSelectionDialog;

/**
 * Object tool selector
 */
public class ObjectToolSelector extends AbstractSelector
{
   private long toolId;
   
   /**
    * @param parent
    * @param style
    */
   public ObjectToolSelector(Composite parent, int style)
   {
      super(parent, style);
      toolId = 0;
   }

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractSelector#selectionButtonHandler()
    */
   @Override
   protected void selectionButtonHandler()
   {
      ObjectToolSelectionDialog dlg = new ObjectToolSelectionDialog(getShell());
      if (dlg.open() == Window.OK)
      {
         toolId = dlg.getTool().getId();
         updateToolName();
      }
   }

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractSelector#clearButtonHandler()
    */
   @Override
   protected void clearButtonHandler()
   {
      toolId = 0;
      updateToolName();
   }

   /**
    * @return the toolId
    */
   public long getToolId()
   {
      return toolId;
   }

   /**
    * @param toolId the toolId to set
    */
   public void setToolId(long toolId)
   {
      this.toolId = toolId;
      updateToolName();
   }
   
   /**
    * Update tool name in selector
    */
   private void updateToolName()
   {
      if (toolId == 0)
      {
         setText("");
         return;
      }

      ObjectTool t = ObjectToolsCache.getInstance().findTool(toolId);
      if (t != null)
         setText(t.getName());
      else
         setText("[" + toolId + "]");
   }
}
