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

import org.eclipse.jface.resource.ImageDescriptor;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;

/**
 * Base Object tool result view class
 */
public abstract class ObjectToolResultView extends AdHocObjectView
{
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
}
