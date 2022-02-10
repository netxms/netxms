/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.alarms.editors;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.events.AlarmCategory;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;

public class AlarmCategoryEditor
{
   AlarmCategory category;
   private Runnable timer;
   private NXCSession session;
   
   /**
    * @param category
    */
   public AlarmCategoryEditor(AlarmCategory category)
   {
      this.category = category;
      session = Registry.getSession();
      timer = new Runnable() {
         @Override
         public void run()
         {
            doCategoryModification();
         }
      };
   }
   
   /**
    * Do category modification on server
    */
   private void doCategoryModification()
   {
      new Job("Update alarm category", null) {
         
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            synchronized(AlarmCategoryEditor.this)
            {
               long id = session.modifyAlarmCategory(category);
               if (id != 0)
                  category.setId(id);
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Failed to update alarm category";
         }
      }.start();      
   }
   
   /**
    * Schedule category modification
    */
   public void modify()
   {
      Display.getCurrent().timerExec(-1, timer);
      Display.getCurrent().timerExec(200, timer);
   }
   
   /**
    * @return category
    */
   public AlarmCategory getObjectAsItem()
   {
      return category;
   }
}
