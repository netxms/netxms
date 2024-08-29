/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.resource.ImageDescriptor;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.modules.objecttools.ObjectToolsCache;
import org.xnap.commons.i18n.I18n;

/**
 * Base Object tool result view class
 */
public abstract class ObjectToolResultView extends AdHocObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectToolResultView.class);
   
   protected ObjectTool tool;
   protected ObjectContext object;

   /**
    * Get View name
    * 
    * @param object object
    * @param tool object tool
    * @return view name
    */
   protected static String getViewName(AbstractObject object, ObjectTool tool)
   {
      return object.getObjectName() + " - " + tool.getDisplayName();
   }

   /**
    * Get View id
    * 
    * @param object object
    * @param tool object tool
    * @return view id
    */
   protected static String generateViewId(AbstractObject object, ObjectTool tool)
   {
      return "object-tools.results." + object.getObjectId() + "." + tool.getId();
   }

   /**
    * Create result view.
    *
    * @param object object this tool executed at
    * @param tool object tool definition
    * @param image view icon
    * @param hasFilter true if view should have filter
    */
   public ObjectToolResultView(ObjectContext object, ObjectTool tool, ImageDescriptor image, boolean hasFilter)
   {
      super(getViewName(object.object, tool), image, generateViewId(object.object, tool), object.object.getObjectId(), object.contextId, hasFilter);
      this.tool = tool;
      this.object = object;
   }

   /**
    * Clone constructor
    */
   protected ObjectToolResultView()
   {
      super(null, null, null, 0, 0, false);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      ObjectToolResultView view = (ObjectToolResultView)super.cloneView();
      view.tool = tool;
      view.object = object;
      return view;
   } 

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);      
      memento.set("tool", tool.getId());  
      memento.set("contextObject.alarm", object.getAlarmId());
      memento.set("contextObject.object", object.object.getObjectId());
      memento.set("contextObject.contextId", object.contextId);
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      long id = memento.getAsLong("tool", 0);
      long alarm = memento.getAsLong("contextObject.alarm", 0);
      long objectId = memento.getAsLong("contextObject.object", 0);
      long contextId = memento.getAsLong("object.contextId", 0);
      
      this.tool = ObjectToolsCache.getInstance().findTool(id);
      if (this.tool == null)
         throw(new ViewNotRestoredException(i18n.tr("Invalid tool id")));
      
      Job job = new Job(i18n.tr("Find alarm id"), this) {
         
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            object = new ObjectContext(session.findObjectById(objectId), alarm > 0 ? session.getAlarm(alarm) : null, contextId);
            runInUIThread(() -> setName(getViewName(object.object, tool)));
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Error on alarm search");
         }
      };
      job.setUser(false);
      job.start();
   }
}
